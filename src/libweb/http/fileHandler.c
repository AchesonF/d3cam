#include "libhttp.h"

/*
    fileHandler.c -- Static file content handler

    This handler manages static file based content such as HTML, GIF /or JPEG pages. It supports all methods including:
    GET, PUT, DELETE, OPTIONS and TRACE. It is event based and does not use worker threads.

    The fileHandler also manages requests for directories that require redirection to an index or responding with
    a directory listing.

 */

/********************************* Includes ***********************************/



/***************************** Forward Declarations ***************************/

static void closeFileHandler(HttpQueue *q);
static void handleDeleteRequest(HttpQueue *q);
static void incomingFile(HttpQueue *q, HttpPacket *packet);
static int openFileHandler(HttpQueue *q);
static void outgoingFileService(HttpQueue *q);
static ssize readFileData(HttpQueue *q, HttpPacket *packet, MprOff pos, ssize size);
static void readyFileHandler(HttpQueue *q);
static int rewriteFileHandler(HttpStream *stream);
static void startFileHandler(HttpQueue *q);
static void startPutRequest(HttpQueue *q);

/*********************************** Code *************************************/
/*
    Loadable module initialization
 */
PUBLIC int httpOpenFileHandler()
{
    HttpStage     *handler;

    /*
        This handler serves requests without using thread workers.
     */
    if ((handler = httpCreateHandler("fileHandler", NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    handler->rewrite = rewriteFileHandler;
    handler->open = openFileHandler;
    handler->close = closeFileHandler;
    handler->start = startFileHandler;
    handler->ready = readyFileHandler;
    handler->outgoingService = outgoingFileService;
    handler->incoming = incomingFile;
    HTTP->fileHandler = handler;
    return 0;
}

/*
    Rewrite the request for directories, indexes and compressed content.
 */
static int rewriteFileHandler(HttpStream *stream)
{
    HttpRx      *rx;
    HttpTx      *tx;
    MprPath     *info;

    rx = stream->rx;
    tx = stream->tx;
    info = &tx->fileInfo;

    httpMapFile(stream);
    assert(info->checked);

    if (rx->flags & (HTTP_DELETE | HTTP_PUT)) {
        return HTTP_ROUTE_OK;
    }
    if (info->isDir) {
        return httpHandleDirectory(stream);
    }
    if (rx->flags & (HTTP_GET | HTTP_HEAD | HTTP_POST) && info->valid) {
        tx->entityLength = tx->fileInfo.size;
    }
    return HTTP_ROUTE_OK;
}


/*
    This is called after the headers are parsed
 */
static int openFileHandler(HttpQueue *q)
{
    HttpRx      *rx;
    HttpTx      *tx;
    HttpStream  *stream;
    MprPath     *info;
    char        *date, dbuf[16];
    MprHash     *dateCache;

    stream = q->stream;
    tx = stream->tx;
    rx = stream->rx;
    info = &tx->fileInfo;

    if (stream->error) {
        return MPR_ERR_CANT_OPEN;
    }
    if (rx->flags & (HTTP_GET | HTTP_HEAD | HTTP_POST)) {
        if (!(info->valid || info->isDir)) {
            httpError(stream, HTTP_CODE_NOT_FOUND, "Cannot find document");
            return 0;
        }
        if (!tx->etag) {
            /* Set the etag for caching in the client */
            tx->etag = itos(info->inode + info->size + info->mtime);
        }
        if (info->mtime) {
            dateCache = stream->http->dateCache;
            if ((date = mprLookupKey(dateCache, itosbuf(dbuf, sizeof(dbuf), (int64) info->mtime, 10))) == 0) {
                if (!dateCache || mprGetHashLength(dateCache) > 128) {
                    stream->http->dateCache = dateCache = mprCreateHash(0, 0);
                }
                date = httpGetDateString(&tx->fileInfo);
                mprAddKey(dateCache, itosbuf(dbuf, sizeof(dbuf), (int64) info->mtime, 10), date);
            }
            httpSetHeaderString(stream, "Last-Modified", date);
        }
        if (httpContentNotModified(stream)) {
            httpSetStatus(stream, HTTP_CODE_NOT_MODIFIED);
            httpRemoveHeader(stream, "Content-Encoding");
            httpOmitBody(stream);
            httpFinalizeOutput(stream);
        }
        if (!tx->fileInfo.isReg && !tx->fileInfo.isLink) {
            httpLog(stream->trace, "fileHandler", "error", "msg:Document is not a regular file, filename:%s", tx->filename);
            httpError(stream, HTTP_CODE_NOT_FOUND, "Cannot serve document");

        } else if (tx->fileInfo.size > stream->limits->txBodySize && stream->limits->txBodySize != HTTP_UNLIMITED) {
            httpError(stream, HTTP_CLOSE | HTTP_CODE_REQUEST_TOO_LARGE,
                "Http transmission aborted. File size exceeds max body of %lld bytes", stream->limits->txBodySize);

        } else {
            /*
                If using the net connector, open the file if a body must be sent with the response. The file will be
                automatically closed when the request completes.
             */
            if (!(tx->flags & HTTP_TX_NO_BODY)) {
                tx->file = mprOpenFile(tx->filename, O_RDONLY | O_BINARY, 0);
                if (tx->file == 0) {
                    if (rx->referrer && *rx->referrer) {
                        httpLog(stream->trace, "fileHandler", "error", "msg:Cannot open document, filename:%s, referrer:%s",
                            tx->filename, rx->referrer);
                    } else {
                        httpLog(stream->trace, "fileHandler", "error", "msg:Cannot open document, filename:%s", tx->filename);
                    }
                    httpError(stream, HTTP_CODE_NOT_FOUND, "Cannot open document");
                }
            }
        }

    } else if (rx->flags & HTTP_PUT) {
        startPutRequest(q);

    } else if (rx->flags & (HTTP_OPTIONS | HTTP_DELETE)) {
        //  Handle in startFileHandler

    } else {
        httpError(stream, HTTP_CODE_BAD_METHOD, "Unsupported method");
    }
    return 0;
}


/*
    Called when the request is complete
 */
static void closeFileHandler(HttpQueue *q)
{
    HttpTx  *tx;

    tx = q->stream->tx;
    if (tx->file) {
        mprCloseFile(tx->file);
        tx->file = 0;
    }
}


/*
    Called when all the body content has been received, but may not have yet been processed by our incoming()
 */
static void startFileHandler(HttpQueue *q)
{
    HttpStream      *stream;
    HttpTx          *tx;
    HttpRx          *rx;
    HttpPacket      *packet;
    HttpFillProc    fill;

    stream = q->stream;
    tx = stream->tx;
    rx = stream->rx;

    if (rx->flags & HTTP_DELETE) {
        handleDeleteRequest(q);
        httpFinalizeOutput(stream);

    } else if (stream->rx->flags & HTTP_HEAD) {
        tx->length = tx->entityLength;
        httpFinalizeOutput(stream);

    } else if (rx->flags & HTTP_OPTIONS) {
        httpHandleOptions(q->stream);
        httpFinalizeOutput(stream);

    } else if (stream->rx->flags & HTTP_PUT) {
        //  Handled in incoming

    } else if (stream->rx->flags & (HTTP_GET | HTTP_POST)) {
        if ((!(tx->flags & HTTP_TX_NO_BODY)) && (tx->entityLength >= 0 && !stream->error)) {
            /*
                Create a single dummy data packet based to hold the remaining data length and file seek possition.
                This is used to trigger the outgoing file service. It is not transmitted to the client.
             */
#if CONFIG_HTTP_SENDFILE
            if (tx->simplePipeline) {
                httpLog(stream->trace, "fileHandler", "detail", "msg:Using sendfile for %s", tx->filename);
                httpRemoveChunkFilter(stream->txHead);
                fill = NULL;
            } else {
                if (q->net->protocol < 2 && !q->net->secure) {
                    httpLog(stream->trace, "fileHandler", "detail", "msg:Using slower full pipeline for %s", tx->filename);
                }
                fill = readFileData;
            }
#else
            fill = readFileData;
#endif
            packet = httpCreateEntityPacket(0, tx->entityLength, fill);

            /*
                Set the content length if not chunking and not using ranges
             */
            if (!tx->outputRanges && tx->chunkSize < 0) {
                tx->length = tx->entityLength;
            }
            httpPutPacket(q, packet);
        }
    } else {
        httpFinalizeOutput(stream);
    }
}


/*
    The ready callback is invoked when all the input body data has been received
    The queue already contains a single data packet representing all the output data.
 */
static void readyFileHandler(HttpQueue *q)
{
    if (!q->stream->tx->finalized) {
        httpScheduleQueue(q);
    }
}


/*
    The incoming callback is invoked to receive body data
 */
static void incomingFile(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;
    HttpTx      *tx;
    HttpRx      *rx;
    HttpRange   *range;
    MprBuf      *buf;
    MprFile     *file;
    ssize       len;

    stream = q->stream;
    tx = stream->tx;
    rx = stream->rx;
    file = (MprFile*) q->queueData;

    if (packet->flags & HTTP_PACKET_END) {
        /* End of input */
        if (file) {
            mprCloseFile(file);
            q->queueData = 0;
        }
        if (!tx->etag) {
            /* Set the etag for caching in the client */
            mprGetPathInfo(tx->filename, &tx->fileInfo);
            tx->etag = itos(tx->fileInfo.inode + tx->fileInfo.size + tx->fileInfo.mtime);
        }
        httpFinalizeInput(stream);

        if (rx->flags & HTTP_PUT) {
            httpFinalizeOutput(stream);
        }

    } else if (file) {
        buf = packet->content;
        len = mprGetBufLength(buf);
        if (len > 0) {
            range = rx->inputRange;
            if (range && mprSeekFile(file, SEEK_SET, range->start) != range->start) {
                httpError(stream, HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot seek to range start to %lld", range->start);

            } else if (mprWriteFile(file, mprGetBufStart(buf), len) != len) {
                httpError(stream, HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot PUT to %s", tx->filename);
            }
        }
    }
}


/*
    The service callback will be invoked to service outgoing packets on the service queue. It will only be called
    once all incoming data has been received and then when the downstream queues drain sufficiently to absorb
    more data. This routine may flow control if the downstream stage cannot accept all the file data. It will
    then be re-called as required to send more data.
 */
static void outgoingFileService(HttpQueue *q)
{
    HttpStream  *stream;
    //HttpRx      *rx;
    HttpTx      *tx;
    HttpPacket  *data, *packet;
    ssize       size, nbytes;

    stream = q->stream;
    if (stream->state >= HTTP_STATE_COMPLETE) {
        return;
    }
    //rx = stream->rx;
    tx = stream->tx;

    /*
        The queue will contain a dummy entity packet which holds the position from which to read in the file.
        If the downstream queue is full, the data packet will be put onto the queue ahead of the entity packet.
        When EOF, and END packet will be added to the queue via httpFinalizeOutput which will then be sent.
     */
    for (packet = q->first; packet; packet = q->first) {
        if (packet->fill) {
            size = min(packet->esize, q->packetSize);
            size = min(size, q->nextQ->packetSize);
            if (size > 0) {
                data = httpCreateDataPacket(size);
                if ((nbytes = readFileData(q, data, tx->filePos, size)) < 0) {
                    httpError(stream, HTTP_CODE_NOT_FOUND, "Cannot read document");
                    return;
                }
                tx->filePos += nbytes;
                packet->epos += nbytes;
                packet->esize -= nbytes;
                if (packet->esize == 0) {
                    httpGetPacket(q);
                }
                /*
                    This may split the packet and put back the tail portion ahead of the just putback entity packet.
                 */
                if (!httpWillNextQueueAcceptPacket(q, data)) {
                    httpPutBackPacket(q, data);
                    return;
                }
                httpPutPacketToNext(q, data);
            } else {
                httpGetPacket(q);
            }
        } else {
            // Don't flow control as the packet is already consuming memory
            packet = httpGetPacket(q);
            httpPutPacketToNext(q, packet);
        }
        if (!tx->finalizedOutput && !q->first) {
            /*
                Optimize FinalizeOutput by putting the end packet manually. The helps the HTTP/2 protocol
                can see the END_DATA packet now and set an END_DATA flag on the last data frame rather than
                waiting for this outgoing to be rescheduled later.
             */
            tx->putEndPacket = 1;
            httpPutPacketToNext(q, httpCreateEndPacket());
            httpFinalizeOutput(stream);
        }
    }
    if (!tx->finalized && tx->finalizedOutput && tx->finalizedInput) {
        httpFinalize(stream);
    }
}


/*
    Populate a packet with file data. Return the number of bytes read or a negative error code.
 */
static ssize readFileData(HttpQueue *q, HttpPacket *packet, MprOff pos, ssize size)
{
    HttpStream  *stream;
    HttpTx      *tx;
    ssize       nbytes;

    stream = q->stream;
    tx = stream->tx;

    if (size <= 0) {
        return 0;
    }
    if (mprGetBufSpace(packet->content) < size) {
        size = mprGetBufSpace(packet->content);
    }
    if (pos >= 0) {
        mprSeekFile(tx->file, SEEK_SET, pos);
    }
    if ((nbytes = mprReadFile(tx->file, mprGetBufStart(packet->content), size)) != size) {
        /*
            As we may have sent some data already to the client, the only thing we can do is abort and hope the client
            notices the short data.
         */
        httpError(stream, HTTP_CODE_SERVICE_UNAVAILABLE, "Cannot read file %s", tx->filename);
        return MPR_ERR_CANT_READ;
    }
    mprAdjustBufEnd(packet->content, nbytes);
    return nbytes;
}


/*
    This is called to setup for a HTTP PUT request. It is called before receiving the post data via incomingFileData
 */
static void startPutRequest(HttpQueue *q)
{
    HttpStream  *stream;
    HttpTx      *tx;
    MprFile     *file;
    cchar       *path;

    assert(q->queueData == 0);

    stream = q->stream;
    tx = stream->tx;
    assert(tx->filename);
    assert(tx->fileInfo.checked);

    path = tx->filename;
    if (tx->outputRanges) {
        /*
            Open an existing file with fall-back to create
         */
        if ((file = mprOpenFile(path, O_BINARY | O_WRONLY, 0644)) == 0) {
            if ((file = mprOpenFile(path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, 0644)) == 0) {
                httpError(stream, HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot create the put URI");
                return;
            }
        } else {
            mprSeekFile(file, SEEK_SET, 0);
        }
    } else {
        if ((file = mprOpenFile(path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, 0644)) == 0) {
            httpError(stream, HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot create the put URI");
            return;
        }
    }
    if (!tx->fileInfo.isReg) {
        httpSetHeaderString(stream, "Location", stream->rx->uri);
    }
    /*
        These are both success returns. 204 means already existed.
     */
    httpSetStatus(stream, tx->fileInfo.isReg ? HTTP_CODE_NO_CONTENT : HTTP_CODE_CREATED);
    q->queueData = (void*) file;
}


static void handleDeleteRequest(HttpQueue *q)
{
    HttpStream  *stream;
    HttpTx      *tx;

    stream = q->stream;
    tx = stream->tx;
    assert(tx->filename);
    assert(tx->fileInfo.checked);

    if (!tx->fileInfo.isReg) {
        httpError(stream, HTTP_CODE_NOT_FOUND, "Document not found");
        return;
    }
    if (mprDeletePath(tx->filename) < 0) {
        httpError(stream, HTTP_CODE_NOT_FOUND, "Cannot remove document");
        return;
    }
    httpSetStatus(stream, HTTP_CODE_NO_CONTENT);
}


PUBLIC int httpHandleDirectory(HttpStream *stream)
{
    HttpRx      *rx;
    HttpTx      *tx;
    HttpRoute   *route;
    HttpUri     *req;
    cchar       *index, *pathInfo, *path;
    int         next;

    rx = stream->rx;
    tx = stream->tx;
    req = rx->parsedUri;
    route = rx->route;

    /*
        Manage requests for directories
     */
    if (!sends(req->path, "/")) {
        /*
           Append "/" and do an external redirect. Use the original request URI. Use httpFormatUri to preserve query.
         */
        httpRedirect(stream, HTTP_CODE_MOVED_PERMANENTLY,
            httpFormatUri(0, 0, 0, sjoin(req->path, "/", NULL), req->reference, req->query, 0));
        return HTTP_ROUTE_OK;
    }
    if (route->indexes) {
        /*
            Ends with a "/" so do internal redirection to an index file
         */
        path = 0;
        for (ITERATE_ITEMS(route->indexes, index, next)) {
            /*
                Internal directory redirections. Transparently append index. Test indexes in order.
             */
            path = mprJoinPath(tx->filename, index);
            if (mprPathExists(path, R_OK)) {
                break;
            }
            if (route->map && !(tx->flags & HTTP_TX_NO_MAP)) {
                path = httpMapContent(stream, path);
                if (mprPathExists(path, R_OK)) {
                    break;
                }
            }
            path = 0;
        }
        if (path) {
            pathInfo = sjoin(req->path, index, NULL);
            if (httpSetUri(stream, httpFormatUri(req->scheme, req->host, req->port, pathInfo, req->reference,
                    req->query, 0)) < 0) {
                mprLog("error http", 0, "Cannot handle directory \"%s\"", pathInfo);
                return HTTP_ROUTE_REJECT;
            }
            tx->filename = httpMapContent(stream, path);
            mprGetPathInfo(tx->filename, &tx->fileInfo);
            return HTTP_ROUTE_REROUTE;
        }
    }
#if CONFIG_HTTP_DIR
    /*
        Directory Listing. Test if a directory listing should be rendered. If so, delegate to the dirHandler.
        Must use the netConnector.
     */
    if (httpShouldRenderDirListing(stream)) {
        tx->handler = stream->http->dirHandler;
        tx->connector = stream->http->netConnector;
        return HTTP_ROUTE_OK;
    }
#endif
    return HTTP_ROUTE_OK;
}



