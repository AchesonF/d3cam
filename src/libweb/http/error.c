#include "libhttp.h"

/*
    error.c -- Http error handling
 */

/********************************* Includes ***********************************/



/********************************** Forwards **********************************/

static void errorv(HttpStream *stream, int flags, cchar *fmt, va_list args);
static cchar *formatErrorv(HttpStream *stream, int status, cchar *fmt, va_list args);

/*********************************** Code *************************************/

PUBLIC void httpNetError(HttpNet *net, cchar *fmt, ...)
{
    va_list     args;
    HttpStream  *stream;
    cchar       *msg;
    int         next;

    if (net == 0 || fmt == 0) {
        return;
    }
    va_start(args, fmt);
    if (!net->error) {
        httpSetNetError(net);
        net->errorMsg = msg = sfmtv(fmt, args);
#if CONFIG_HTTP_HTTP2
        if (net->protocol >= 2 && !net->eof) {
            httpSendGoAway(net, HTTP2_INTERNAL_ERROR, "%s", msg);
        }
#endif
        if (httpIsServer(net)) {
            for (ITERATE_ITEMS(net->streams, stream, next)) {
                httpFinalizeConnector(stream);
                httpError(stream, HTTP_ABORT | HTTP_CODE_COMMS_ERROR, "%s", msg);
            }
            httpMonitorNetEvent(net, HTTP_COUNTER_BAD_REQUEST_ERRORS, 1);
        }
    }
    va_end(args);
}


PUBLIC void httpBadRequestError(HttpStream *stream, int flags, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    if (httpServerStream(stream)) {
        httpMonitorEvent(stream, HTTP_COUNTER_BAD_REQUEST_ERRORS, 1);
    }
    errorv(stream, flags, fmt, args);
    va_end(args);
}


PUBLIC void httpLimitError(HttpStream *stream, int flags, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    if (httpServerStream(stream)) {
        httpMonitorEvent(stream, HTTP_COUNTER_LIMIT_ERRORS, 1);
    }
    errorv(stream, flags, fmt, args);
    va_end(args);
}


PUBLIC void httpError(HttpStream *stream, int flags, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    errorv(stream, flags, fmt, args);
    va_end(args);
}


static void errorRedirect(HttpStream *stream, cchar *uri)
{
    HttpTx      *tx;

    tx = stream->tx;
    if (sstarts(uri, "http") || (tx->flags & HTTP_TX_HEADERS_CREATED)) {
        httpRedirect(stream, HTTP_CODE_MOVED_PERMANENTLY, uri);
    } else {
        tx->errorDocument = httpLinkAbs(stream, uri);
        //  Cap input and output until prepErrorDoc can reconstitute the request
        tx->finalized = tx->finalizedInput = tx->finalizedOutput = 1;
        httpSetState(stream, HTTP_STATE_FINALIZED);
    }
}


static void makeAltBody(HttpStream *stream, int status)
{
    HttpRx      *rx;
    HttpTx      *tx;
    cchar       *statusMsg, *msg;

    rx = stream->rx;
    tx = stream->tx;
    assert(rx && tx);

    statusMsg = httpLookupStatus(status);
    msg = (rx && rx->route && rx->route->flags & HTTP_ROUTE_SHOW_ERRORS) ?  stream->errorMsg : "";
    if (rx && scmp(rx->accept, "text/plain") == 0) {
        tx->altBody = sfmt("Access Error: %d -- %s\r\n%s\r\n", status, statusMsg, msg);
    } else {
        httpSetContentType(stream, "text/html");
        tx->altBody = sfmt("<!DOCTYPE html>\r\n"
            "<head>\r\n"
            "    <title>%s</title>\r\n"
            "    <link rel=\"shortcut icon\" href=\"data:image/x-icon;,\" type=\"image/x-icon\">\r\n"
            "</head>\r\n"
            "<body>\r\n<h2>Access Error: %d -- %s</h2>\r\n<pre>%s</pre>\r\n</body>\r\n</html>\r\n",
            statusMsg, status, statusMsg, mprEscapeHtml(msg));
    }
    tx->length = slen(tx->altBody);
}


/*
    The current request has an error and cannot complete as normal. This call sets the Http response status and
    overrides the normal output with an alternate error message. If the output has alread started (headers sent), then
    the connection MUST be closed so the client can get some indication the request failed.
 */
static void errorv(HttpStream *stream, int flags, cchar *fmt, va_list args)
{
    HttpRx      *rx;
    HttpTx      *tx;
    cchar       *uri;
    int         redirected, status;

    if (stream == 0 || fmt == 0) {
        return;
    }
    rx = stream->rx;
    tx = stream->tx;

    status = flags & HTTP_CODE_MASK;
    if (flags & (HTTP_ABORT | HTTP_CLOSE)) {
        stream->keepAliveCount = 0;
        httpSetEof(stream);
    }
    if (!stream->error) {
        stream->error = 1;
        if (status) {
            httpOmitBody(stream);
            stream->errorMsg = formatErrorv(stream, status, fmt, args);
            httpLog(stream->trace, "error", "error", "msg:%s", stream->errorMsg);
            if (httpServerStream(stream)) {
                if (status == HTTP_CODE_NOT_FOUND) {
                    httpMonitorEvent(stream, HTTP_COUNTER_NOT_FOUND_ERRORS, 1);
                }
                httpMonitorEvent(stream, HTTP_COUNTER_ERRORS, 1);
            }
        }
        HTTP_NOTIFY(stream, HTTP_EVENT_ERROR, 0);
        httpSetHeaderString(stream, "Cache-Control", "no-cache");

        if (httpServerStream(stream) && tx && rx) {
            if (tx->flags & HTTP_TX_HEADERS_CREATED) {
                /*
                    If the response headers have been sent, must let the other side of the failure ... aborting
                    the request is the only way as the status has been sent.
                 */
                flags |= HTTP_ABORT;
            } else {
                redirected = 0;
                if (rx->route) {
                    if ((uri = httpLookupRouteErrorDocument(rx->route, tx->status)) != 0) {
                        uri = httpExpandVars(stream, uri);
                    }
                    if (rx->route->callback) {
                        rx->route->callback(stream, HTTP_ROUTE_HOOK_ERROR, &uri);
                    }
                    if (uri && !smatch(uri, rx->uri)) {
                        errorRedirect(stream, uri);
                        redirected = 1;
                    }
                }
                if (status && !redirected) {
                    makeAltBody(stream, status);
                }
            }
        }
        if (flags & HTTP_ABORT) {
            stream->disconnect = 1;
            httpDiscardData(stream, HTTP_QUEUE_RX);
        }
        if (flags & (HTTP_ABORT | HTTP_CLOSE)) {
            httpFinalize(stream);
        } else {
            //  Still need to consume the input
            httpFinalizeOutput(stream);
        }
    }
    if (stream->disconnect && stream->net->protocol < 2) {
        httpDisconnectStream(stream);
    }
}


/*
    Just format stream->errorMsg and set status - nothing more
    NOTE: this is an internal API. Users should use httpError()
 */
static cchar *formatErrorv(HttpStream *stream, int status, cchar *fmt, va_list args)
{
    if (stream->errorMsg == 0) {
        stream->errorMsg = sfmtv(fmt, args);
        if (status) {
            if (status < 0) {
                status = HTTP_CODE_INTERNAL_SERVER_ERROR;
            }
            if (httpServerStream(stream) && stream->tx) {
                stream->tx->status = status;
            } else if (stream->rx) {
                stream->rx->status = status;
            }
        }
    }
    return stream->errorMsg;
}


PUBLIC cchar *httpGetError(HttpStream *stream)
{
    if (stream->errorMsg) {
        return stream->errorMsg;
    } else if (stream->state >= HTTP_STATE_FIRST) {
        return httpLookupStatus(stream->rx->status);
    } else {
        return "";
    }
}


PUBLIC void httpMemoryError(HttpStream *stream)
{
    httpError(stream, HTTP_CODE_INTERNAL_SERVER_ERROR, "Memory allocation error");
}




