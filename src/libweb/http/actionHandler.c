#include "libhttp.h"

/*
    actionHandler.c -- Action handler

    This handler maps URIs to actions that are C functions that have been registered via httpDefineAction.
 */

/********************************* Includes ***********************************/



/*********************************** Code *************************************/

static void startAction(HttpQueue *q)
{
    HttpStream  *stream;
    HttpAction  action;
    cchar       *name;

    stream = q->stream;
    assert(!stream->error);
    assert(!stream->tx->finalized);

    name = stream->rx->pathInfo;
    if ((action = mprLookupKey(stream->tx->handler->stageData, name)) == 0) {
        httpError(stream, HTTP_CODE_NOT_FOUND, "Cannot find action: %s", name);
    } else {
        (*action)(stream);
    }
}


PUBLIC void httpDefineAction(cchar *name, HttpAction action)
{
    HttpStage   *stage;

    if ((stage = httpLookupStage("actionHandler")) == 0) {
        mprLog("error http action", 0, "Cannot find actionHandler");
        return;
    }
    mprAddKey(stage->stageData, name, action);
}


PUBLIC int httpOpenActionHandler()
{
    HttpStage     *stage;

    if ((stage = httpCreateHandler("actionHandler", NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    HTTP->actionHandler = stage;
    if ((stage->stageData = mprCreateHash(0, MPR_HASH_STATIC_VALUES)) == 0) {
        return MPR_ERR_MEMORY;
    }
    stage->start = startAction;
    return 0;
}



