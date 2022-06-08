#include "libhttp.h"

/*
    trace.c -- Trace data

    HTTP trace is configured per route.

    Event types and trace levels:
    1: debug, error
    2: request, result
    3: headers
    4: context
    5: packet
    6: detail
 */

/********************************* Includes ***********************************/



/********************************* Forwards ***********************************/

static void emitTraceValues(MprBuf *buf, char *str);
static void flushTrace(HttpTrace *trace);
static void formatTrace(HttpTrace *trace, cchar *event, cchar *type, int flags, cchar *buf, ssize len, cchar *fmt, ...);
static cchar *getTraceTag(cchar *event, cchar *type);

static void traceData(HttpTrace *trace, cchar *data, ssize len, int flags);
static void traceQueues(HttpStream *stream, MprBuf *buf);

/*********************************** Code *************************************/

static void manageTrace(HttpTrace *trace, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(trace->buf);
        mprMark(trace->events);
        mprMark(trace->file);
        mprMark(trace->format);
        mprMark(trace->lastTime);
        mprMark(trace->mutex);
        mprMark(trace->parent);
        mprMark(trace->path);
    }
}

/*
    Parent may be null
 */
PUBLIC HttpTrace *httpCreateTrace(HttpTrace *parent)
{
    HttpTrace   *trace;

    if ((trace = mprAllocObj(HttpTrace, manageTrace)) == 0) {
        return 0;
    }
    if (parent) {
        *trace = *parent;
        trace->parent = parent;
    } else {
        if ((trace->events = mprCreateHash(0, MPR_HASH_STATIC_VALUES)) == 0) {
            return 0;
        }
        mprAddKey(trace->events, "debug", ITOP(1));
        mprAddKey(trace->events, "error", ITOP(1));
        mprAddKey(trace->events, "request", ITOP(2));
        mprAddKey(trace->events, "result", ITOP(2));
        mprAddKey(trace->events, "headers", ITOP(3));
        mprAddKey(trace->events, "context", ITOP(4));
        mprAddKey(trace->events, "packet", ITOP(5));
        mprAddKey(trace->events, "detail", ITOP(6));

        /*
            Max log file size
         */
        trace->size = HTTP_TRACE_MAX_SIZE;
        trace->maxContent = MAXINT;
        trace->formatter = httpPrettyFormatter;
        trace->logger = httpWriteTraceLogFile;
        trace->mutex = mprCreateLock();
    }
    return trace;
}


PUBLIC void httpSetTraceContentSize(HttpTrace *trace, ssize size)
{
    trace->maxContent = size;
}


PUBLIC void httpSetTraceEventLevel(HttpTrace *trace, cchar *type, int level)
{
    assert(trace);
    mprAddKey(trace->events, type, ITOP(level));
}


PUBLIC int httpGetTraceLevel(HttpTrace *trace)
{
    return trace->level;
}


PUBLIC void httpSetTraceFormat(HttpTrace *trace, cchar *format)
{
    trace->format = sclone(format);
}


PUBLIC HttpTraceFormatter httpSetTraceFormatter(HttpTrace *trace, HttpTraceFormatter callback)
{
    HttpTraceFormatter  prior;

    prior = trace->formatter;
    trace->formatter = callback;
    return prior;
}


PUBLIC void httpSetTraceFormatterName(HttpTrace *trace, cchar *name)
{
    HttpTraceFormatter  formatter;

    if (name && smatch(name, "common")) {
        if ((trace->events = mprCreateHash(0, MPR_HASH_STATIC_VALUES)) == 0) {
            return;
        }
        mprAddKey(trace->events, "result", ITOP(0));
        formatter = httpCommonFormatter;

    } else if (smatch(name, "pretty")) {
       formatter = httpPrettyFormatter;

    } else if (smatch(name, "detail")) {
       formatter = httpDetailFormatter;
    } else {
        mprLog("trace error", 0, "Unknown trace formatter %s", name)
       formatter = httpDetailFormatter;
    }
    httpSetTraceFormatter(trace, formatter);
}


PUBLIC void httpSetTraceLevel(HttpTrace *trace, int level)
{
    if (level < 0) {
        level = 0;
    } else if (level > 10) {
        level = 10;
    }
    trace->level = level;
}


PUBLIC void httpSetTraceLogger(HttpTrace *trace, HttpTraceLogger callback)
{
    trace->logger = callback;
}


/*
    Inner routine for httpLog()
 */
PUBLIC void httpLogProc(HttpTrace *trace, cchar *event, cchar *type, int flags, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    httpFormatTrace(trace, event, type, flags, NULL, 0, fmt, args);
    va_end(args);
}


PUBLIC void httpLogRxPacket(HttpNet *net, cchar *buf, ssize len)
{
    formatTrace(net->trace, "rx.net", "packet", HTTP_TRACE_HEX, buf, len, "length:%lld", len);
}


PUBLIC void httpLogTxPacket(HttpNet *net, ssize len)
{
    HttpTrace   *trace;
    HttpQueue   *q;
    ssize       bytes;
    int         flags, i;

    q = net->socketq;
    flags = HTTP_TRACE_CONT;
    trace = net->trace;

    if (!net->skipTrace && net->bytesWritten >= trace->maxContent) {
        httpLog(trace, "tx.http1", "packet", "msg:Abbreviating packet trace");
        net->skipTrace = 1;
    }
    if (net->ioFile) {
        formatTrace(trace, "tx.net", "packet", flags, NULL, 0, "path:%s, length:%zd, written:%d\n",
            net->ioFile->path, net->ioCount, len);
    } else {
        formatTrace(trace, "tx.net", "packet", flags, NULL, 0, "length:%zd, written:%d\n", net->ioCount, len);
    }
    flags |= HTTP_TRACE_HEX;
    for (i = 0; i < net->ioIndex; i++) {
        bytes = min(len, net->iovec[i].len);
        formatTrace(trace, "tx.net", "packet", flags, net->iovec[i].start, bytes, NULL);
        len -= bytes;
    }
    if (net->ioFile) {
        //  MOB - need to read file data here
        formatTrace(trace, "tx.net", "packet", flags, "FILE", 4, NULL);
    }
    flushTrace(trace);
}


PUBLIC void httpLogCompleteRequest(HttpStream *stream)
{
    HttpRx      *rx;
    HttpTx      *tx;
    MprTicks    elapsed;
    MprOff      received;

    rx = stream->rx;
    tx = stream->tx;

    elapsed = mprGetTicks() - stream->started;
    if (httpTracing(stream->net)) {
        if (httpShouldTrace(stream->trace, "context")) {
            received = httpGetPacketLength(rx->headerPacket) + rx->bytesRead;
#if MPR_HIGH_RES_TIMER
            formatTrace(stream->trace, "tx.complete", "context", 0, (void*) stream, 0,
                "elapsed:%llu, ticks:%llu, received:%lld, sent:%lld",
                elapsed, mprGetHiResTicks() - stream->startMark, received, tx->bytesWritten);
#else
            formatTrace(stream->trace, "tx.complete", "context", 0, (void*) stream, 0,
                "elapsed:%llu, received:%lld, sent:%lld", elapsed, received, tx->bytesWritten);
#endif
        }
    }
}


/*
    Format and emit trace
 */
PUBLIC void httpFormatTrace(HttpTrace *trace, cchar *event, cchar *type, int flags, cchar *buf, ssize len, cchar *fmt, va_list args)
{
    (trace->formatter)(trace, event, type, flags, buf, len, fmt, args);
}


/*
    Internal format end emit trace with literal args
 */
static void formatTrace(HttpTrace *trace, cchar *event, cchar *type, int flags, cchar *buf, ssize len, cchar *fmt, ...)
{
    va_list    args;

    va_start(args, fmt);
    (trace->formatter)(trace, event, type, flags, buf, len, fmt, args);
    va_end(args);
}


/*
    Low-level write routine to be used only by formatters
 */
PUBLIC void httpWriteTrace(HttpTrace *trace, cchar *buf, ssize len)
{
    (trace->logger)(trace, buf, len);
}


/*
    Format a detailed request message
 */
PUBLIC void httpDetailFormatter(HttpTrace *trace, cchar *event, cchar *type, int flags,
    cchar *data, ssize len, cchar *fmt, va_list args)
{
    MprBuf      *buf;
    MprTime     now;
    char        *msg;

    assert(trace);
    lock(trace);

    if (!trace->buf) {
        trace->buf = mprCreateBuf(ME_PACKET_SIZE, 0);
    }
    buf = trace->buf;

    if (fmt) {
        now = mprGetTime();
        if (trace->lastMark < (now + TPS) || trace->lastTime == 0) {
            trace->lastTime = mprGetDate("%D %T");
            trace->lastMark = now;
        }
        mprPutToBuf(buf, "%s %s event=%s type=%s\n", trace->lastTime, getTraceTag(event, type), event, type);
    }
    if (fmt) {
        msg = sfmtv(fmt, args);
        if (*msg == '@') {
            mprPutStringToBuf(buf, &msg[1]);
        } else {
            mprPutStringToBuf(buf, msg);
        }
        mprPutStringToBuf(buf, "\n");
    }
    if (data && len > 0) {
        traceData(trace, data, len, flags);
        mprPutStringToBuf(buf, "\n");
    }
    flushTrace(trace);
    unlock(trace);
}


PUBLIC void httpPrettyFormatter(HttpTrace *trace, cchar *event, cchar *type, int flags,
        cchar *data, ssize len, cchar *fmt, va_list args)
{
    MprBuf      *buf;
    MprTicks    now;
    char        *msg;

    assert(trace);
    assert(event);
    assert(type);

    lock(trace);
    if (!trace->buf) {
        trace->buf = mprCreateBuf(ME_PACKET_SIZE, 0);
    }
    buf = trace->buf;

    if (fmt) {
        now = mprGetTime();
        if (trace->lastMark < (now + TPS) || trace->lastTime == 0) {
            trace->lastTime = mprGetDate("%D %T");
            trace->lastMark = now;
        }
        mprPutToBuf(buf, "\n%s %s (%s)\n", trace->lastTime, getTraceTag(event, type), event);
        if (fmt) {
            mprPutCharToBuf(buf, '\n');
            msg = sfmtv(fmt, args);
            if (flags & HTTP_TRACE_RAW) {
                mprPutStringToBuf(buf, msg);
            } else if (*msg == '@') {
                mprPutStringToBuf(buf, &msg[1]);
            } else {
                emitTraceValues(buf, msg);
            }
        }
        if (data && len > 0) {
            mprPutCharToBuf(buf, '\n');
        }
    }
    if (data && len > 0) {
        traceData(trace, data, len, flags);
    }
    if (!(flags & HTTP_TRACE_CONT)) {
        flushTrace(trace);
    }
    unlock(trace);
}


/*
    Trace a data buffer. Will emit in hex if flags & HTTP_TRACE_HEX.
 */
static void traceData(HttpTrace *trace, cchar *data, ssize len, int flags)
{
    MprBuf  *buf;
    cchar   *cp, *digits, *sol;
    char    *end, *ep;
    ssize   bsize, lines, need, space;
    int     i, j;

    buf = trace->buf;

    if (flags & HTTP_TRACE_HEX) {
        /*
            Round up lines, 4 chars per byte plush 3 chars per line (||\n)
         */
        lines = len / 16 + 1;
        bsize = ((lines * 16) * 4) + (lines * 5) + 2;
        space = mprGetBufSpace(buf);
        if (bsize > space) {
            need = bsize - space;
            need = max(need, ME_PACKET_SIZE);
            mprGrowBuf(buf, need);
        }
        end = mprGetBufEnd(buf);
        digits = "0123456789ABCDEF";

        for (i = 0, cp = data, ep = end; cp < &data[len]; ) {
            sol = cp;
            for (j = 0; j < 16 && cp < &data[len]; j++, cp++) {
                *ep++ = digits[(*cp >> 4) & 0x0f];
                *ep++ = digits[*cp & 0x0f];
                *ep++ = ' ';
            }

            for (; j < 16; j++) {
                *ep++ = ' '; *ep++ = ' '; *ep++ = ' ';
            }
            *ep++ = ' '; *ep++ = ' '; *ep++ = '|';
            for (j = 0, cp = sol; j < 16 && cp < &data[len]; j++, cp++) {
                *ep++ = isprint(*cp) ? *cp : '.';
            }
            for (; j < 16; j++) {
                *ep++ = ' ';
            }
            *ep++ = '|';
            *ep++ = '\n';
            assert((ep - end) <= bsize);
        }
        *ep = '\0';
        mprAdjustBufEnd(buf, ep - end);

    } else {
        mprPutBlockToBuf(buf, data, len);
    }
}


static void flushTrace(HttpTrace *trace)
{
    MprBuf      *buf;

    buf = trace->buf;
    mprPutStringToBuf(buf, "\n");
    httpWriteTrace(trace, mprGetBufStart(buf), mprGetBufLength(buf));
    mprFlushBuf(buf);
}


static void emitTraceValues(MprBuf *buf, char *str)
{
    char    *key, *tok, *value;

    for (key = stok(str, ",", &tok); key; key = stok(0, ",", &tok)) {
        if (key[0] != ':' && schr(key, ':')) {
            key = stok(key, ":", &value);
        } else if (schr(key, '=')) {
            key = stok(key, "=", &value);
        } else {
            value = NULL;
        }
        key = strim(key, " \t'", MPR_TRIM_BOTH);
        if (value) {
            value = strim(value, "\t'", MPR_TRIM_BOTH);
        } else {
            value = "";
        }
        if (smatch(key, "msg")) {
            mprPutToBuf(buf, "    %14s:  %s\n", "message", value);
        } else {
            mprPutToBuf(buf, "    %14s:  %s\n", key, value);
        }
    }
}


static cchar *getTraceTag(cchar *event, cchar *type)
{
    if (sstarts(event, "tx")) {
        return "OUTGOING";
    } else if (sstarts(event, "rx")) {
        return "INCOMING";
    }
    return type;
}

/*
    Common Log Formatter (NCSA)
    This formatter only emits messages only for connections at their complete event.
 */
PUBLIC void httpCommonFormatter(HttpTrace *trace, cchar *event, cchar *type, int flags, cchar *data, ssize len,
        cchar *msg, va_list args)
{
    HttpStream  *stream;
    HttpRx      *rx;
    HttpTx      *tx;
    MprBuf      *buf;
    cchar       *fmt, *cp, *qualifier, *timeText, *value;
    char        c, keyBuf[256];

    assert(trace);
    assert(type && *type);
    assert(event && *event);

    stream = (HttpStream*) data;
    if (!stream) {
        return;
    }
    if (!smatch(event, "rx.complete")) {
        return;
    }
    rx = stream->rx;
    tx = stream->tx;
    fmt = trace->format;
    if (fmt == 0 || fmt[0] == '\0') {
        fmt = ME_HTTP_LOG_FORMAT;
    }
    if (!trace->buf) {
        trace->buf = mprCreateBuf(ME_PACKET_SIZE, 0);
    }
    buf = trace->buf;

    while ((c = *fmt++) != '\0') {
        if (c != '%' || (c = *fmt++) == '%') {
            mprPutCharToBuf(buf, c);
            continue;
        }
        switch (c) {
        case 'a':                           /* Remote IP */
            mprPutStringToBuf(buf, stream->ip);
            break;

        case 'A':                           /* Local IP */
            mprPutStringToBuf(buf, stream->sock->listenSock->ip);
            break;

        case 'b':
            if (tx->bytesWritten == 0) {
                mprPutCharToBuf(buf, '-');
            } else {
                mprPutIntToBuf(buf, tx->bytesWritten);
            }
            break;

        case 'B':                           /* Bytes written (minus headers) */
            mprPutIntToBuf(buf, (tx->bytesWritten - tx->headerSize));
            break;

        case 'h':                           /* Remote host */
            mprPutStringToBuf(buf, stream->ip);
            break;

        case 'l':                           /* user identity - unknown */
            mprPutCharToBuf(buf, '-');
            break;

        case 'n':                           /* Local host */
            mprPutStringToBuf(buf, rx->parsedUri->host);
            break;

        case 'O':                           /* Bytes written (including headers) */
            mprPutIntToBuf(buf, tx->bytesWritten);
            break;

        case 'r':                           /* First line of request */
            mprPutToBuf(buf, "%s %s %s", rx->method, rx->uri, httpGetProtocol(stream->net));
            break;

        case 's':                           /* Response code */
            mprPutIntToBuf(buf, tx->status);
            break;

        case 't':                           /* Time */
            mprPutCharToBuf(buf, '[');
            timeText = mprFormatLocalTime(MPR_DEFAULT_DATE, mprGetTime());
            mprPutStringToBuf(buf, timeText);
            mprPutCharToBuf(buf, ']');
            break;

        case 'u':                           /* Remote username */
            mprPutStringToBuf(buf, stream->username ? stream->username : "-");
            break;

        case '{':                           /* Header line "{header}i" */
            qualifier = fmt;
            if ((cp = schr(qualifier, '}')) != 0) {
                fmt = &cp[1];
                switch (*fmt++) {
                case 'i':
                    sncopy(keyBuf, sizeof(keyBuf), qualifier, cp - qualifier);
                    value = (char*) mprLookupKey(rx->headers, keyBuf);
                    mprPutStringToBuf(buf, value ? value : "-");
                    break;
                default:
                    mprPutSubStringToBuf(buf, qualifier, qualifier - cp);
                }

            } else {
                mprPutCharToBuf(buf, c);
            }
            break;

        case '>':
            if (*fmt == 's') {
                fmt++;
                mprPutIntToBuf(buf, tx->status);
            }
            break;

        default:
            mprPutCharToBuf(buf, c);
            break;
        }
    }
    mprPutCharToBuf(buf, '\n');
    flushTrace(trace);
}

/************************************** TraceLogFile **************************/

static int backupTraceLogFile(HttpTrace *trace)
{
    MprPath     info;

    assert(trace->path);

    if (trace->file == MPR->logFile) {
        return 0;
    }
    if (trace->backupCount > 0 || (trace->flags & MPR_LOG_ANEW)) {
        lock(trace);
        if (trace->path && trace->parent && smatch(trace->parent->path, trace->path)) {
            unlock(trace);
            return backupTraceLogFile(trace->parent);
        }
        mprGetPathInfo(trace->path, &info);
        if (info.valid && ((trace->flags & MPR_LOG_ANEW) || info.size > trace->size)) {
            if (trace->file) {
                mprCloseFile(trace->file);
                trace->file = 0;
            }
            if (trace->backupCount > 0) {
                mprBackupLog(trace->path, trace->backupCount);
            }
        }
        unlock(trace);
    }
    return 0;
}


/*
    Open the request log file
 */
PUBLIC int httpOpenTraceLogFile(HttpTrace *trace)
{
    MprFile     *file;
    int         mode;

    if (!trace->file && trace->path) {
        if (smatch(trace->path, "-")) {
            file = MPR->logFile;
        } else {
            backupTraceLogFile(trace);
            mode = O_CREAT | O_WRONLY | O_TEXT;
            mode |= (trace->flags & MPR_LOG_ANEW) ? O_TRUNC : O_APPEND;
            if (smatch(trace->path, "stdout")) {
                file = MPR->stdOutput;
            } else if (smatch(trace->path, "stderr")) {
                file = MPR->stdError;
            } else if ((file = mprOpenFile(trace->path, mode, 0664)) == 0) {
                mprLog("error http trace", 0, "Cannot open log file %s", trace->path);
                return MPR_ERR_CANT_OPEN;
            }
        }
        trace->file = file;
        trace->flags &= ~MPR_LOG_ANEW;
    }
    return 0;
}


/*
    Start tracing when instructed via a command line option.
 */
PUBLIC int httpStartTracing(cchar *traceSpec)
{
    HttpTrace   *trace;
    char        *lspec;

    if (HTTP == 0 || HTTP->trace == 0 || traceSpec == 0 || *traceSpec == '\0') {
        assert(HTTP);
        return MPR_ERR_BAD_STATE;
    }
    trace = HTTP->trace;
    trace->flags = MPR_LOG_ANEW | MPR_LOG_CMDLINE;
    trace->path = stok(sclone(traceSpec), ":", &lspec);
    trace->level = (int) stoi(lspec);
    return httpOpenTraceLogFile(trace);
}


/*
    Configure the trace log file
 */
PUBLIC int httpSetTraceLogFile(HttpTrace *trace, cchar *path, ssize size, int backup, cchar *format, int flags)
{
    assert(trace);
    assert(path && *path);

    if (format == NULL || *format == '\0') {
        format = ME_HTTP_LOG_FORMAT;
    }
    trace->backupCount = backup;
    trace->flags = flags;
    trace->format = sclone(format);
    trace->size = size;
    trace->path = sclone(path);
    trace->file = 0;
    return httpOpenTraceLogFile(trace);
}


/*
    Write a message to the trace log
 */
PUBLIC void httpWriteTraceLogFile(HttpTrace *trace, cchar *buf, ssize len)
{
    static int  skipCheck = 0;

    lock(trace);
    if (trace->backupCount > 0) {
        if ((++skipCheck % 50) == 0) {
            backupTraceLogFile(trace);
        }
    }
    if (!trace->file && trace->path && httpOpenTraceLogFile(trace) < 0) {
        unlock(trace);
        return;
    }
	if (trace->file) {
		mprWriteFile(trace->file, buf, len);
	}
    unlock(trace);
}


PUBLIC bool httpShouldTrace(HttpTrace *trace, cchar *type)
{
    int         level;

    assert(type && *type);
    level = PTOI(mprLookupKey(trace->events, type));
    if (level >= 0 && level <= trace->level) {
        return 1;
    }
    return 0;
}


PUBLIC void httpTraceQueues(HttpStream *stream)
{
    HttpNet     *net;
    HttpQueue   *q;
    HttpTrace   *trace;
    cchar       *pipeline;

    net = stream->net;
    trace = stream->trace;
    if (!httpShouldTrace(trace, "detail")) {
        return;
    }
    pipeline = stream->rxHead->name;
    for (q = stream->rxHead->prevQ; q != stream->rxHead; q = q->prevQ) {
        pipeline = sjoin(pipeline, " < ", q->name, NULL);
    }
    pipeline = sjoin(pipeline, " < ", net->inputq->name, " < ", net->socketq->name, NULL);
    httpLog(trace, "stream.queues", "detail", "msg:read-pipeline: %s ", pipeline);

    pipeline = stream->txHead->name;
    for (q = stream->txHead->nextQ; q != stream->txHead; q = q->nextQ) {
        pipeline = sjoin(pipeline, " > ", q->name, NULL);
    }
    pipeline = sjoin(pipeline, " > ", net->outputq->name, NULL);
    for (q = net->outputq->nextQ; q != net->outputq; q = q->prevQ) {
        pipeline = sjoin(pipeline, " > ", q->name, NULL);
    }
    httpLog(trace, "stream.queues", "detail", "msg:write-pipeline: %s ", pipeline);
    httpLog(trace, "stream.queues", "detail", "readq: %s, writeq: %s, inputq: %s, outputq: %s",
        stream->readq->name, stream->writeq->name, stream->inputq->name, stream->outputq->name);
}


PUBLIC char *httpStatsReport(int flags)
{
    Http                *http;
    HttpNet             *net;
    HttpStream          *stream;
    HttpRx              *rx;
    HttpTx              *tx;
    MprTime             now;
    MprBuf              *buf;
    HttpStats           s;
    double              elapsed, mb;
    static MprTime      lastTime = 0;
    static HttpStats    last;
    int                 nextNet, nextStream;

    mb = 1024.0 * 1024;
    now = mprGetTime();
    if (lastTime == 0) {
        lastTime = MPR->start;
    }
    elapsed = (now - lastTime) / 1000.0;
    httpGetStats(&s);
    buf = mprCreateBuf(ME_PACKET_SIZE, 0);

    mprPutToBuf(buf, "\nHttp Report: at %s\n\n", mprGetDate("%D %T"));
    if (flags & HTTP_STATS_MEMORY) {
        mprPutToBuf(buf, "Memory       %8.1f MB, %5.1f%% max\n", s.mem / mb, s.mem / (double) s.memMax * 100.0);
        mprPutToBuf(buf, "Heap         %8.1f MB, %5.1f%% mem\n", s.heap / mb, s.heap / (double) s.mem * 100.0);
        mprPutToBuf(buf, "Heap-peak    %8.1f MB\n", s.heapPeak / mb);
        mprPutToBuf(buf, "Heap-used    %8.1f MB, %5.1f%% used\n", s.heapUsed / mb, s.heapUsed / (double) s.heap * 100.0);
        mprPutToBuf(buf, "Heap-free    %8.1f MB, %5.1f%% free\n", s.heapFree / mb, s.heapFree / (double) s.heap * 100.0);

        if (s.memMax == (size_t) -1) {
            mprPutToBuf(buf, "Heap limit          -\n");
            mprPutToBuf(buf, "Heap readline       -\n");
        } else {
            mprPutToBuf(buf, "Heap limit   %8.1f MB\n", s.memMax / mb);
            mprPutToBuf(buf, "Heap redline %8.1f MB\n", s.memRedline / mb);
        }
    }

    mprPutToBuf(buf, "Connections  %8.1f per/sec\n", (s.totalConnections - last.totalConnections) / elapsed);
    mprPutToBuf(buf, "Requests     %8.1f per/sec\n", (s.totalRequests - last.totalRequests) / elapsed);
    mprPutToBuf(buf, "Sweeps       %8.1f per/sec\n", (s.totalSweeps - last.totalSweeps) / elapsed);
    mprPutCharToBuf(buf, '\n');

    mprPutToBuf(buf, "Clients      %8d active\n", s.activeClients);
    mprPutToBuf(buf, "Connections  %8d active\n", s.activeConnections);
    mprPutToBuf(buf, "Processes    %8d active\n", s.activeProcesses);
    mprPutToBuf(buf, "Requests     %8d active\n", s.activeRequests);
    mprPutToBuf(buf, "Sessions     %8d active\n", s.activeSessions);
    mprPutToBuf(buf, "Workers      %8d busy - %d yielded, %d idle, %d max\n",
        s.workersBusy, s.workersYielded, s.workersIdle, s.workersMax);
    mprPutToBuf(buf, "Sessions     %8.1f MB\n", s.memSessions / mb);
    mprPutCharToBuf(buf, '\n');
    last = s;

    http = HTTP;
    mprPutToBuf(buf, "\nActive Streams:\n");
    lock(http);
    for (ITERATE_ITEMS(http->networks, net, nextNet)) {
        for (ITERATE_ITEMS(net->streams, stream, nextStream)) {
            rx = stream->rx;
            tx = stream->tx;
            mprPutToBuf(buf,
                "State %d (%d), error %d, eof %d, finalized input %d, output %d, connector %d, seqno %lld, net mask %x, net error %d, net eof %d, destroyed %d, uri %s\n",
                stream->state, stream->h2State, stream->error, rx->eof, tx->finalizedInput, tx->finalizedOutput,
                tx->finalizedConnector, stream->seqno, net->eventMask, net->error, (int) net->eof, net->destroyed, rx->uri);
            traceQueues(stream, buf);
        }
    }
    unlock(http);
    mprPutCharToBuf(buf, '\n');

    lastTime = now;
    mprAddNullToBuf(buf);
    return sclone(mprGetBufStart(buf));
}


static void traceQueues(HttpStream *stream, MprBuf *buf)
{
    HttpNet     *net;
    HttpQueue   *q;
    HttpTrace   *trace;

    net = stream->net;
    trace = stream->trace;

    mprPutToBuf(buf, "  Rx: ");
    mprPutToBuf(buf, "%s[%d, %d, 0x%x] < ",
        stream->rxHead->name, (int) stream->rxHead->count, (int) stream->rxHead->window, stream->rxHead->flags);
    for (q = stream->rxHead->prevQ; q != stream->rxHead; q = q->prevQ) {
        mprPutToBuf(buf, "%s[%d, %d, 0x%x] < ", q->name, (int) q->count, (int) q->window, q->flags);
    }
    mprPutToBuf(buf, "%s[%d, %d, 0x%x] < ",
        net->inputq->name, (int) net->inputq->count, (int) net->inputq->window, net->inputq->flags);
    mprPutToBuf(buf, "%s[%d, 0x%x]\n", net->socketq->name, (int) net->socketq->count, net->socketq->flags);

    mprPutToBuf(buf, "  Tx: ");
    mprPutToBuf(buf, "%s[%d, %d, 0x%x] > ",
        stream->txHead->name, (int) stream->txHead->count, (int) stream->txHead->window, stream->txHead->flags);
    for (q = stream->txHead->nextQ; q != stream->txHead; q = q->nextQ) {
        mprPutToBuf(buf, "%s[%d, %d, 0x%x] > ", q->name, (int) q->count, (int) q->window, q->flags);
    }
    mprPutToBuf(buf, "%s[%d, %d, 0x%x] > ",
        net->outputq->name, (int) net->outputq->count, (int) net->outputq->window, net->outputq->flags);
    for (q = net->outputq->nextQ; q != net->outputq; q = q->prevQ) {
        mprPutToBuf(buf, "%s[%d, %d, 0x%x]\n\n", q->name, (int) q->count, (int) q->window, q->flags);
    }
}




