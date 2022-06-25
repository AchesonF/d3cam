#include "libhttp.h"

/*
    http1Filter.c - HTTP/1 protocol handling.

    The HttpFilter is after the TailFilter and before the NetConnector.

 */

/********************************* Includes ***********************************/



/*********************************** Locals ***********************************/

#define TOKEN_HEADER_KEY        0x1     /* Validate token as a header key */
#define TOKEN_HEADER_VALUE      0x2     /* Validate token as a header value */
#define TOKEN_URI               0x4     /* Validate token as a URI value */
#define TOKEN_NUMBER            0x8     /* Validate token as a number */
#define TOKEN_WORD              0x10    /* Validate token as single word with no spaces */
#define TOKEN_LINE              0x20    /* Validate token as line with no newlines */

/********************************** Forwards **********************************/

static cchar *eatBlankLines(HttpPacket *packet);
static HttpStream *findStream1(HttpQueue *q);
static char *getToken(HttpPacket *packet, cchar *delim, int validation);
static bool gotHeaders(HttpQueue *q, HttpPacket *packet);
static void incomingHttp1(HttpQueue *q, HttpPacket *packet);
static void incomingHttp1Service(HttpQueue *q);
static void outgoingHttp1(HttpQueue *q, HttpPacket *packet);
static void outgoingHttp1Service(HttpQueue *q);
static void parseFields(HttpQueue *q, HttpPacket *packet);
static bool parseHeaders1(HttpQueue *q, HttpPacket *packet);
static void parseRequestLine(HttpQueue *q, HttpPacket *packet);
static void parseResponseLine(HttpQueue *q, HttpPacket *packet);
static char *validateToken(char *token, char *endToken, int validation);

/*********************************** Code *************************************/
/*
   Loadable module initialization
 */
PUBLIC int httpOpenHttp1Filter()
{
    HttpStage     *filter;

    if ((filter = httpCreateConnector("Http1Filter", NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    HTTP->http1Filter = filter;
    filter->incoming = incomingHttp1;
    filter->outgoing = outgoingHttp1;
    filter->incomingService = incomingHttp1Service;
    filter->outgoingService = outgoingHttp1Service;
    return 0;
}


static void incomingHttp1(HttpQueue *q, HttpPacket *packet)
{
    httpPutForService(q, packet, HTTP_SCHEDULE_QUEUE);
}


/*
    The queue is the net->inputq == netHttp-rx
 */
static void incomingHttp1Service(HttpQueue *q)
{
    HttpStream  *stream;
    HttpPacket  *packet;

    stream = findStream1(q);

    if (stream->state < HTTP_STATE_PARSED) {
        httpJoinPackets(q, -1);
    }
    for (packet = httpGetPacket(q); packet; packet = httpGetPacket(q)) {
        if (stream->state < HTTP_STATE_PARSED) {
            if (!parseHeaders1(q, packet)) {
                httpPutBackPacket(q, packet);
                return;
            }
            if (httpGetPacketLength(packet) == 0) {
                continue;
            }
        }
        if (!httpWillQueueAcceptPacket(q, stream->inputq, packet)) {
            httpPutBackPacket(q, packet);
            //  Re-enabled in tailFilter
            return;
        }
        httpPutPacket(stream->inputq, packet);
    }
    if (stream->error && packet && packet->flags & HTTP_PACKET_END) {
        httpFinalizeInput(stream);
    }
}


static void outgoingHttp1(HttpQueue *q, HttpPacket *packet)
{
    httpPutForService(q, packet, 1);
}


static void outgoingHttp1Service(HttpQueue *q)
{
    HttpStream  *stream;
    HttpPacket  *packet;

    stream = q->stream;
    stream->lastActivity = stream->net->lastActivity = stream->http->now;

    for (packet = httpGetPacket(q); packet; packet = httpGetPacket(q)) {
        if (!httpWillQueueAcceptPacket(q, q->net->socketq, packet)) {
            httpPutBackPacket(q, packet);
            return;
        }
        /*
            Mutliplex directly onto the net connector and not use q->nextQ
         */
        httpPutPacket(q->net->socketq, packet);
    }
    if (stream && q->count <= q->low && (stream->outputq->flags & HTTP_QUEUE_SUSPENDED)) {
        httpResumeQueue(stream->outputq, 0);
    }
}


/*
    After parsing headers, return a packet with body data.
    If headers are not yet fully read, return the packet.
 */
static bool parseHeaders1(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;
    HttpRx      *rx;

    stream = q->stream;
    assert(stream->rx);
    assert(stream->tx);
    rx = stream->rx;

    if (! gotHeaders(q, packet)) {
        return 0;
    }
    rx->headerPacket = packet;
    if (httpServerStream(stream)) {
        parseRequestLine(q, packet);
    } else {
        parseResponseLine(q, packet);
    }
    parseFields(q, packet);
    return 1;
}


static bool gotHeaders(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;
    HttpLimits  *limits;
    cchar       *end, *start;
    ssize       len;

    stream = q->stream;
    limits = stream->limits;
    start = eatBlankLines(packet);
    len = httpGetPacketLength(packet);

    if ((end = sncontains(start, "\r\n\r\n", len)) != 0 || (end = sncontains(start, "\n\n", len)) != 0) {
        len = end - start;
    }
    if (len >= limits->headerSize || len >= q->max) {
        httpLimitError(stream, HTTP_CLOSE | HTTP_CODE_REQUEST_TOO_LARGE,
            "Header too big. Length %zd vs limit %d", len, limits->headerSize);
        return 0;
    }
    return end ? 1 : 0;
}


/*
    Parse the first line of a http request. Return true if the first line parsed. This is only called once all the headers
    have been read and buffered. Requests look like: METHOD URL HTTP/1.X.
 */
static void parseRequestLine(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;
    HttpRx      *rx;
    HttpLimits  *limits;
    char        *method, *uri, *protocol;
    ssize       len;

    stream = q->stream;
    rx = stream->rx;
    limits = stream->limits;

    method = getToken(packet, NULL, TOKEN_WORD);
    if (method == NULL || *method == '\0') {
        httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad HTTP request. Empty method");
        return;
    }
    rx->originalMethod = rx->method = supper(method);
    httpParseMethod(stream);

    uri = getToken(packet, NULL, TOKEN_URI);
    if (uri == NULL || *uri == '\0') {
        httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad HTTP request. Invalid URI");
        return;
    }
    len = slen(uri);
    if (len >= limits->uriSize) {
        httpLimitError(stream, HTTP_CLOSE | HTTP_CODE_REQUEST_URL_TOO_LARGE,
            "Bad request. URI too long. Length %zd vs limit %d", len, limits->uriSize);
        return;
    }
    rx->uri = sclone(uri);
    if (!rx->originalUri) {
        rx->originalUri = rx->uri;
    }
    protocol = getToken(packet, "\r\n", TOKEN_WORD);
    if (protocol == NULL || *protocol == '\0') {
        httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad HTTP request. Empty protocol");
        return;
    }
    rx->protocol = protocol = supper(protocol);
    if (smatch(protocol, "HTTP/1.0") || *protocol == 0) {
        if (rx->flags & (HTTP_POST|HTTP_PUT)) {
            rx->remainingContent = HTTP_UNLIMITED;
            rx->needInputPipeline = 1;
        }
        stream->net->protocol = 0;
    } else if (strcmp(protocol, "HTTP/1.1") != 0) {
        httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_NOT_ACCEPTABLE, "Unsupported HTTP protocol");
        return;
    } else {
        stream->net->protocol = 1;
    }
    httpSetState(stream, HTTP_STATE_FIRST);
}


/*
    Parse the first line of a http response. Return true if the first line parsed. This is only called once all the headers
    have been read and buffered. Response status lines look like: HTTP/1.X CODE Message
 */
static void parseResponseLine(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpStream  *stream;
    HttpRx      *rx;
    HttpTx      *tx;
    char        *message, *protocol, *status;
    ssize       len;

    net = q->net;
    stream = q->stream;
    rx = stream->rx;
    tx = stream->tx;

    protocol = getToken(packet, NULL, TOKEN_WORD);
    if (protocol == NULL || *protocol == '\0') {
        httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_NOT_ACCEPTABLE, "Bad response protocol");
        return;
    }
    protocol = supper(protocol);
    if (strcmp(protocol, "HTTP/1.0") == 0) {
        net->protocol = 0;
        if (!scaselessmatch(tx->method, "HEAD")) {
            rx->remainingContent = HTTP_UNLIMITED;
        }
    } else if (strcmp(protocol, "HTTP/1.1") != 0) {
        httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_NOT_ACCEPTABLE, "Unsupported HTTP protocol");
        return;
    }
    rx->protocol = supper(protocol);
    status = getToken(packet, NULL, TOKEN_NUMBER);
    if (status == NULL || *status == '\0') {
        httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_NOT_ACCEPTABLE, "Bad response status code");
        return;
    }
    rx->status = atoi(status);

    message = getToken(packet, "\r\n", TOKEN_LINE);
    if (message == NULL || *message == '\0') {
        httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_NOT_ACCEPTABLE, "Bad response status message");
        return;
    }
    rx->statusMessage = sclone(message);

    len = slen(rx->statusMessage);
    if (len >= stream->limits->uriSize) {
        httpLimitError(stream, HTTP_CLOSE | HTTP_CODE_REQUEST_URL_TOO_LARGE,
            "Bad response. Status message too long. Length %zd vs limit %d", len, stream->limits->uriSize);
        return;
    }
}


/*
    Parse the header fields and return a following body packet if present.
    Return zero on errors.
 */
static void parseFields(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;
    HttpRx      *rx;
    HttpLimits  *limits;
    char        *key, *value;
    int         count;

    stream = q->stream;
    rx = stream->rx;

    limits = stream->limits;

    for (count = 0; mprGetBufLength(packet->content) > 0 && packet->content->start[0] != '\r' && !stream->error; count++) {
        if (count >= limits->headerMax) {
            httpLimitError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Too many headers");
            return;
        }
        key = getToken(packet, ":", TOKEN_HEADER_KEY);
        if (key == NULL || *key == '\0' || mprGetBufLength(packet->content) == 0) {
            httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad header format");
            return;
        }
        value = getToken(packet, "\r\n", TOKEN_HEADER_VALUE);
        if (value == NULL || mprGetBufLength(packet->content) == 0 || packet->content->start[0] == '\0') {
            httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad header value");
            return;
        }
        if (scaselessmatch(key, "set-cookie")) {
            mprAddDuplicateKey(rx->headers, key, sclone(value));
        } else {
            mprAddKey(rx->headers, key, sclone(value));
        }
    }
    /*
        If there were no headers, there will be no trailing ...
     */
    if (mprGetBufLength(packet->content) < 2) {
        httpBadRequestError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad header format");
        return;
    }
    /*
        Split the headers and retain the data for later. Step over "\r\n" after headers except if chunked
        so chunking can parse a single chunk delimiter of "\r\nSIZE ...\r\n"
     */
    if (smatch(httpGetHeader(stream, "transfer-encoding"), "chunked")) {
        httpInitChunking(stream);
    } else {
        mprAdjustBufStart(packet->content, 2);
    }
    if (rx->status != HTTP_CODE_CONTINUE) {
        httpSetState(stream, HTTP_STATE_PARSED);
    }
}


/*
    Get the next input token. The content buffer is advanced to the next token.
    The delimiter is a string to match and not a set of characters.
    If the delimeter null, it means use white space (space or tab) as a delimiter.
 */
static char *getToken(HttpPacket *packet, cchar *delim, int validation)
{
    MprBuf  *buf;
    char    *token, *endToken;

    buf = packet->content;
    /* Already null terminated but for safety */
    mprAddNullToBuf(buf);
    token = mprGetBufStart(buf);
    endToken = mprGetBufEnd(buf);

    /*
        Eat white space before token
     */
    for (; token < endToken && (*token == ' ' || *token == '\t'); token++) {}

    if (delim) {
        if ((endToken = sncontains(token, delim, endToken - token)) == NULL) {
            return NULL;
        }
        /* Only eat one occurence of the delimiter */
        buf->start = endToken + strlen(delim);
        *endToken = '\0';

    } else {
        delim = " \t";
        if ((endToken = strpbrk(token, delim)) == NULL) {
            return NULL;
        }
        buf->start = endToken + strspn(endToken, delim);
        *endToken = '\0';
    }
    token = validateToken(token, endToken, validation);
    return token;
}


static char *validateToken(char *token, char *endToken, int validation)
{
    char   *t;

    if (validation == TOKEN_HEADER_KEY) {
        if (*token == '\0') {
            return NULL;
        }
        if (strpbrk(token, "\"\\/ \t\r\n(),:;<=>?@[]{}")) {
            return NULL;
        }
        for (t = token; t < endToken && *t; t++) {
            if (!isprint(*t)) {
                return NULL;
            }
        }
    } else if (validation == TOKEN_HEADER_VALUE) {
        if (token < endToken) {
            /* Trim white space */
            for (t = endToken - 1; t >= token; t--) {
                if (isspace((uchar) *t)) {
                    *t = '\0';
                } else {
                    break;
                }
            }
        }
        while (isspace((uchar) *token)) {
            token++;
        }
        for (t = token; *t; t++) {
            if (!isprint(*t)) {
                return NULL;
            }
        }
    } else if (validation == TOKEN_URI) {
        if (!httpValidUriChars(token)) {
            return NULL;
        }
    } else if (validation == TOKEN_NUMBER) {
        if (!snumber(token)) {
            return NULL;
        }
    } else if (validation == TOKEN_WORD) {
        if (strpbrk(token, " \t\r\n") != NULL) {
            return NULL;
        }
    } else {
        if (strpbrk(token, "\r\n") != NULL) {
            return NULL;
        }
    }
    return token;
}


PUBLIC void httpCreateHeaders1(HttpQueue *q, HttpPacket *packet)
{
    Http        *http;
    HttpStream  *stream;
    HttpTx      *tx;
    HttpUri     *parsedUri;
    MprKey      *kp;
    MprBuf      *buf;

    stream = q->stream;
    http = stream->http;
    tx = stream->tx;
    buf = packet->content;

    tx->responded = 1;

    if (tx->chunkSize <= 0 && q->count > 0 && tx->length < 0) {
        /* No content length and there appears to be output data -- must close connection to signify EOF */
        stream->keepAliveCount = 0;
    }
    if ((tx->flags & HTTP_TX_USE_OWN_HEADERS) && !stream->error) {
        /* Cannot count on content length */
        stream->keepAliveCount = 0;
        return;
    }
    httpPrepareHeaders(stream);

    if (httpServerStream(stream)) {
        mprPutStringToBuf(buf, httpGetProtocol(stream->net));
        mprPutCharToBuf(buf, ' ');
        mprPutIntToBuf(buf, tx->status);
        mprPutCharToBuf(buf, ' ');
        mprPutStringToBuf(buf, httpLookupStatus(tx->status));
        /* Server tracing of status happens in the "complete" event */

    } else {
        mprPutStringToBuf(buf, tx->method);
        mprPutCharToBuf(buf, ' ');
        parsedUri = tx->parsedUri;
        if (http->proxyHost && *http->proxyHost) {
            if (parsedUri->query && *parsedUri->query) {
                mprPutToBuf(buf, "http://%s:%d%s?%s %s", http->proxyHost, http->proxyPort,
                    parsedUri->path, parsedUri->query, httpGetProtocol(stream->net));
            } else {
                mprPutToBuf(buf, "http://%s:%d%s %s", http->proxyHost, http->proxyPort, parsedUri->path,
                    httpGetProtocol(stream->net));
            }
        } else {
            if (parsedUri->query && *parsedUri->query) {
                mprPutToBuf(buf, "%s?%s %s", parsedUri->path, parsedUri->query, httpGetProtocol(stream->net));
            } else {
                mprPutStringToBuf(buf, parsedUri->path);
                mprPutCharToBuf(buf, ' ');
                mprPutStringToBuf(buf, httpGetProtocol(stream->net));
            }
        }
    }
    mprPutStringToBuf(buf, "\r\n");

    if (httpTracing(q->net)) {
        if (httpServerStream(stream)) {
            httpLog(stream->trace, "tx.http.status", "result", "@%s %d %s",
                httpGetProtocol(stream->net), tx->status, httpLookupStatus(tx->status));
        } else {
            if (parsedUri->query && *parsedUri->query) {
                httpLog(stream->trace, "tx.http.status", "request", "@%s %s?%s %s", tx->method, tx->parsedUri->path,
                    tx->parsedUri->query, httpGetProtocol(stream->net));
            } else {
                httpLog(stream->trace, "tx.http.status", "request", "@%s %s %s", tx->method, tx->parsedUri->path, httpGetProtocol(stream->net));
            }
        }
        httpLog(stream->trace, "tx.http.headers", "headers", "@%s", httpTraceHeaders(tx->headers));
    }
    /*
        Output headers
     */
    kp = mprGetFirstKey(stream->tx->headers);
    while (kp) {
        mprPutStringToBuf(packet->content, kp->key);
        mprPutStringToBuf(packet->content, ": ");
        if (kp->data) {
            mprPutStringToBuf(packet->content, kp->data);
        }
        mprPutStringToBuf(packet->content, "\r\n");
        kp = mprGetNextKey(stream->tx->headers, kp);
    }
    /*
        By omitting the "\r\n" delimiter after the headers, chunks can emit "\r\nSize\r\n" as a single chunk delimiter
     */
    if (tx->chunkSize <= 0) {
        mprPutStringToBuf(buf, "\r\n");
    }
    tx->headerSize = mprGetBufLength(buf);
    tx->flags |= HTTP_TX_HEADERS_CREATED;
}


static HttpStream *findStream1(HttpQueue *q)
{
    HttpStream  *stream;

    if (!q->stream) {
        if ((stream = httpCreateStream(q->net, 1)) == 0) {
            //  Memory error - centrally reported
            return 0;
        }
        q->stream = stream;
        q->pair->stream = stream;
    } else {
        stream = q->stream;
    }
    stream->lastActivity = stream->net->lastActivity;
    return stream;
}


static cchar *eatBlankLines(HttpPacket *packet)
{
    MprBuf  *content;
    cchar   *start;

    content = packet->content;
    while (mprGetBufLength(content) > 0) {
        start = mprGetBufStart(content);
        if (*start != '\r' && *start != '\n') {
            break;
        }
        mprGetCharFromBuf(packet->content);
    }
    return mprGetBufStart(content);
}




