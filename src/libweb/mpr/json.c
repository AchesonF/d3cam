#include "libmpr.h"

/**
    json.c - A JSON parser, serializer and query language.

 */
/********************************** Includes **********************************/



/*********************************** Locals ***********************************/
/*
    JSON parse tokens
 */
#define JTOK_COLON      1
#define JTOK_COMMA      2
#define JTOK_EOF        3
#define JTOK_ERR        4
#define JTOK_FALSE      5
#define JTOK_LBRACE     6
#define JTOK_LBRACKET   7
#define JTOK_NULL       8
#define JTOK_NUMBER     9
#define JTOK_RBRACE     10
#define JTOK_RBRACKET   11
#define JTOK_REGEXP     12
#define JTOK_STRING     13
#define JTOK_TRUE       14
#define JTOK_UNDEFINED  15

#define JSON_EXPR_CHARS "<>=!~"

/*
    Remove matching properties
 */
#define JSON_REMOVE     0x1

/****************************** Forward Declarations **************************/

static void adoptChildren(MprJson *obj, MprJson *other);
static void appendItem(MprJson *obj, MprJson *child);
static void appendProperty(MprJson *obj, MprJson *child);
static int checkBlockCallback(MprJsonParser *parser, cchar *name, bool leave);
static int gettok(MprJsonParser *parser);
static MprJson *jsonParse(MprJsonParser *parser, MprJson *obj);
static void jsonErrorCallback(MprJsonParser *parser, cchar *msg);
static int peektok(MprJsonParser *parser);
static void puttok(MprJsonParser *parser);
static MprJson *queryCore(MprJson *obj, cchar *key, MprJson *value, int flags);
static MprJson *queryLeaf(MprJson *obj, cchar *property, MprJson *value, int flags);
static MprJson *setProperty(MprJson *obj, cchar *name, MprJson *child);
static void setValue(MprJson *obj, cchar *value, int type);
static int setValueCallback(MprJsonParser *parser, MprJson *obj, cchar *name, MprJson *child);
static void spaces(MprBuf *buf, int count);

/************************************ Code ************************************/

static void manageJson(MprJson *obj, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(obj->name);
        mprMark(obj->value);
        mprMark(obj->prev);
        mprMark(obj->next);
        mprMark(obj->children);
    }
}


/*
    If value is null, return null so query can detect "set" operations
 */
PUBLIC MprJson *mprCreateJsonValue(cchar *value, int type)
{
    MprJson  *obj;

    if ((obj = mprAllocObj(MprJson, manageJson)) == 0) {
        return 0;
    }
    setValue(obj, value, type);
    return obj;
}


PUBLIC MprJson *mprCreateJson(int type)
{
    MprJson  *obj;

    if ((obj = mprAllocObj(MprJson, manageJson)) == 0) {
        return 0;
    }
    obj->type = type ? type : MPR_JSON_OBJ;
    return obj;
}


static MprJson *createObjCallback(MprJsonParser *parser, int type)
{
    return mprCreateJson(type);
}


static void manageJsonParser(MprJsonParser *parser, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(parser->token);
        mprMark(parser->putback);
        mprMark(parser->path);
        mprMark(parser->errorMsg);
        mprMark(parser->buf);
    }
}


/*
    Extended parse. The str and data args are unmanged.
 */
PUBLIC MprJson *mprParseJsonEx(cchar *str, MprJsonCallback *callback, void *data, MprJson *obj, cchar **errorMsg)
{
    MprJsonParser   *parser;
    MprJson         *result, *child, *next;
    int             i;

    if ((parser = mprAllocObj(MprJsonParser, manageJsonParser)) == 0) {
        return 0;
    }
    parser->input = str ? str : "";
    if (callback) {
        parser->callback = *callback;
    }
    if (parser->callback.checkBlock == 0) {
        parser->callback.checkBlock = checkBlockCallback;
    }
    if (parser->callback.createObj == 0) {
        parser->callback.createObj = createObjCallback;
    }
    if (parser->callback.parseError == 0) {
        parser->callback.parseError = jsonErrorCallback;
    }
    if (parser->callback.setValue == 0) {
        parser->callback.setValue = setValueCallback;
    }
    parser->data = data;
    parser->state = MPR_JSON_STATE_VALUE;
    parser->tolerant = 1;
    parser->buf = mprCreateBuf(128, 0);
    parser->lineNumber = 1;

    if ((result = jsonParse(parser, 0)) == 0) {
        if (errorMsg) {
            *errorMsg = parser->errorMsg;
        }
        return 0;
    }
    if (obj) {
        for (i = 0, child = result->children; child && i < result->length; child = next, i++) {
            next = child->next;
            setProperty(obj, child->name, child);
        }
    } else {
        obj = result;
    }
    return obj;
}


PUBLIC MprJson *mprParseJsonInto(cchar *str, MprJson *obj)
{
    return mprParseJsonEx(str, 0, 0, obj, 0);
}


PUBLIC MprJson *mprParseJson(cchar *str)
{
    return mprParseJsonEx(str, 0, 0, 0, 0);
}


/*
    Inner parse routine. This is called recursively.
 */
static MprJson *jsonParse(MprJsonParser *parser, MprJson *obj)
{
    MprJson     *child;
    cchar       *name;
    int         tokid, type;

    name = 0;
    while (1) {
        tokid = gettok(parser);
        switch (parser->state) {
        case MPR_JSON_STATE_ERR:
            return 0;

        case MPR_JSON_STATE_EOF:
            return obj;

        case MPR_JSON_STATE_NAME:
            if (tokid == JTOK_RBRACE) {
                puttok(parser);
                return obj;
            } else if (tokid != JTOK_STRING) {
                mprSetJsonError(parser, "Expected property name");
                return 0;
            }
            name = sclone(parser->token);
            if (gettok(parser) != JTOK_COLON) {
                mprSetJsonError(parser, "Expected colon");
                return 0;
            }
            parser->state = MPR_JSON_STATE_VALUE;
            break;

        case MPR_JSON_STATE_VALUE:
            if (tokid == JTOK_LBRACE) {
                parser->state = MPR_JSON_STATE_NAME;
                if (name && parser->callback.checkBlock(parser, name, 0) < 0) {
                    return 0;
                }
                child = parser->callback.createObj(parser, MPR_JSON_OBJ);
                if (peektok(parser) != JTOK_RBRACE) {
                    child = jsonParse(parser, child);
                }
                if (gettok(parser) != JTOK_RBRACE) {
                    mprSetJsonError(parser, "Missing closing brace");
                    return 0;
                }
                if (name && parser->callback.checkBlock(parser, name, 1) < 0) {
                    return 0;
                }

            } else if (tokid == JTOK_LBRACKET) {
                if (parser->callback.checkBlock(parser, name, 0) < 0) {
                    return 0;
                }
                child = jsonParse(parser, parser->callback.createObj(parser, MPR_JSON_ARRAY));
                if (gettok(parser) != JTOK_RBRACKET) {
                    mprSetJsonError(parser, "Missing closing bracket");
                    return 0;
                }
                if (parser->callback.checkBlock(parser, name, 1) < 0) {
                    return 0;
                }

            } else if (tokid == JTOK_RBRACKET || tokid == JTOK_RBRACE) {
                puttok(parser);
                return obj;

            } else if (tokid == JTOK_EOF) {
                return obj;

            } else {
                switch (tokid) {
                case JTOK_FALSE:
                    type = MPR_JSON_FALSE;
                    break;
                case JTOK_NULL:
                    type = MPR_JSON_NULL;
                    break;
                case JTOK_NUMBER:
                    type = MPR_JSON_NUMBER;
                    break;
                case JTOK_REGEXP:
                    type = MPR_JSON_REGEXP;
                    break;
                case JTOK_TRUE:
                    type = MPR_JSON_TRUE;
                    break;
                case JTOK_UNDEFINED:
                    type = MPR_JSON_UNDEFINED;
                    break;
                case JTOK_STRING:
                    type = MPR_JSON_STRING;
                    break;
                default:
                    mprSetJsonError(parser, "Unexpected input");
                    return 0;
                    break;
                }
                child = mprCreateJsonValue(parser->token, type);
            }
            if (child == 0) {
                return 0;
            }
            if (obj) {
                if (parser->callback.setValue(parser, obj, name, child) < 0) {
                    return 0;
                }
            } else {
                /* Becomes root object */
                obj = child;
            }
            tokid = peektok(parser);
            if (tokid == JTOK_COMMA) {
                gettok(parser);
                if (parser->tolerant) {
                    tokid = peektok(parser);
                    if (tokid == JTOK_RBRACE || parser->tokid == JTOK_RBRACKET) {
                        return obj;
                    }
                }
                parser->state = (obj->type & MPR_JSON_OBJ) ? MPR_JSON_STATE_NAME : MPR_JSON_STATE_VALUE;
            } else if (tokid == JTOK_RBRACE || parser->tokid == JTOK_RBRACKET || tokid == JTOK_EOF) {
                return obj;
            } else {
                mprSetJsonError(parser, "Unexpected input. Missing comma.");
                return 0;
            }
            break;
        }
    }
}


static void eatRestOfComment(MprJsonParser *parser)
{
    cchar   *cp;
    int     startLine;

    startLine = parser->lineNumber;
    cp = parser->input;
    if (*cp == '/') {
        for (cp++; *cp && *cp != '\n'; cp++) {}
        parser->lineNumber++;

    } else if (*cp == '*') {
        for (cp++; cp[0] && (cp[0] != '*' || cp[1] != '/'); cp++) {
            if (*cp == '\n') {
                parser->lineNumber++;
            }
        }
        if (*cp) {
            cp += 2;
        } else {
            mprSetJsonError(parser, "Cannot find end of comment started on line %d", startLine);
        }
    }
    parser->input = cp;
}


/*
    Peek at the next token without consuming it
 */
static int peektok(MprJsonParser *parser)
{
    int     tokid;

    tokid = gettok(parser);
    puttok(parser);
    return tokid;
}


/*
    Put back the token so it can be refetched via gettok
 */
static void puttok(MprJsonParser *parser)
{
    parser->putid = parser->tokid;
    parser->putback = sclone(parser->token);
}


/*
    Get the next token. Returns the token ID and also stores it in parser->tokid.
    Residuals: parser->token set to the token text. parser->errorMsg for parse error diagnostics.
    Note: parser->token is a reference into the parse buffer and will be overwritten on the next call to gettok.
 */
static int gettok(MprJsonParser *parser)
{
    cchar   *cp, *value;
    ssize   len;
    int     c, d, i, val;

    assert(parser);
    if (!parser || !parser->input) {
        return JTOK_EOF;
    }
    assert(parser->input);
    mprFlushBuf(parser->buf);

    if (parser->state == MPR_JSON_STATE_EOF || parser->state == MPR_JSON_STATE_ERR) {
        return parser->tokid = JTOK_EOF;
    }
    if (parser->putid) {
        parser->tokid = parser->putid;
        parser->putid = 0;
        mprPutStringToBuf(parser->buf, parser->putback);

    } else {
        for (parser->tokid = 0; !parser->tokid; ) {
            c = *parser->input++;
            switch (c) {
            case '\0':
                parser->state = MPR_JSON_STATE_EOF;
                parser->tokid = JTOK_EOF;
                parser->input--;
                break;
            case ' ':
            case '\t':
            case '\r':
                break;
            case '\n':
                parser->lineNumber++;
                break;
            case '{':
                parser->tokid = JTOK_LBRACE;
                mprPutCharToBuf(parser->buf, c);
                break;
            case '}':
                parser->tokid = JTOK_RBRACE;
                mprPutCharToBuf(parser->buf, c);
                break;
            case '[':
                 parser->tokid = JTOK_LBRACKET;
                mprPutCharToBuf(parser->buf, c);
                 break;
            case ']':
                parser->tokid = JTOK_RBRACKET;
                mprPutCharToBuf(parser->buf, c);
                break;
            case ',':
                parser->tokid = JTOK_COMMA;
                mprPutCharToBuf(parser->buf, c);
                break;
            case ':':
                parser->tokid = JTOK_COLON;
                mprPutCharToBuf(parser->buf, c);
                break;
            case '/':
                c = *parser->input;
                if (c == '*' || c == '/') {
                    eatRestOfComment(parser);
                } else if (parser->state == MPR_JSON_STATE_NAME || parser->state == MPR_JSON_STATE_VALUE) {
                    for (cp = parser->input; *cp; cp++) {
                        if (*cp == '\\' && cp[1] == '/') {
                            mprPutCharToBuf(parser->buf, '/');
                        } else if (*cp == '/') {
                            parser->tokid = JTOK_REGEXP;
                            parser->input = cp + 1;
                            break;
                        } else {
                            mprPutCharToBuf(parser->buf, *cp);
                        }
                    }
                    if (*cp != '/') {
                        mprSetJsonError(parser, "Missing closing slash for regular expression");
                    }
                } else {
                    mprSetJsonError(parser, "Unexpected input");
                }
                break;

            case '\\':
                mprSetJsonError(parser, "Bad input state");
                break;

            case '"':
            case '\'':
            case '`':
                /*
                    Quoted strings: names or values
                    This parser is tolerant of embedded, unquoted control characters
                 */
                if (parser->state == MPR_JSON_STATE_NAME || parser->state == MPR_JSON_STATE_VALUE) {
                    for (cp = parser->input; *cp; cp++) {
                        if (*cp == '\\' && cp[1]) {
                            cp++;
                            if (*cp == '\\') {
                                mprPutCharToBuf(parser->buf, '\\');
                            } else if (*cp == '\'') {
                                mprPutCharToBuf(parser->buf, '\'');
                            } else if (*cp == '"') {
                                mprPutCharToBuf(parser->buf, '"');
                            } else if (*cp == '`') {
                                mprPutCharToBuf(parser->buf, '`');
                            } else if (*cp == '/') {
                                mprPutCharToBuf(parser->buf, '/');
                            } else if (*cp == '"') {
                                mprPutCharToBuf(parser->buf, '"');
                            } else if (*cp == 'b') {
                                mprPutCharToBuf(parser->buf, '\b');
                            } else if (*cp == 'f') {
                                mprPutCharToBuf(parser->buf, '\f');
                            } else if (*cp == 'n') {
                                mprPutCharToBuf(parser->buf, '\n');
                            } else if (*cp == 'r') {
                                mprPutCharToBuf(parser->buf, '\r');
                            } else if (*cp == 't') {
                                mprPutCharToBuf(parser->buf, '\t');
                            } else if (*cp == 'u') {
                                for (i = val = 0, ++cp; i < 4 && *cp; i++) {
                                    d = tolower((uchar) *cp);
                                    if (isdigit((uchar) d)) {
                                        val = (val * 16) + d - '0';
                                    } else if (d >= 'a' && d <= 'f') {
                                        val = (val * 16) + d - 'a' + 10;
                                    } else {
                                        mprSetJsonError(parser, "Unexpected hex characters");
                                        break;
                                    }
                                    cp++;
                                }
                                mprPutCharToBuf(parser->buf, val);
                                cp--;
                            } else {
                                mprSetJsonError(parser, "Unexpected input");
                                break;
                            }
                        } else if (*cp == c) {
                            parser->tokid = JTOK_STRING;
                            parser->input = cp + 1;
                            break;
                        } else {
                            mprPutCharToBuf(parser->buf, *cp);
                        }
                    }
                    if (*cp != c) {
                        mprSetJsonError(parser, "Missing closing quote");
                    }
                } else {
                    mprSetJsonError(parser, "Unexpected quote");
                }
                break;

            default:
                parser->input--;
                if (parser->state == MPR_JSON_STATE_NAME) {
                    if (parser->tolerant) {
                        /* Allow unquoted names */
                        for (cp = parser->input; *cp; cp++) {
                            c = *cp;
                            if (isspace((uchar) c) || c == ':') {
                                break;
                            }
                            mprPutCharToBuf(parser->buf, c);
                        }
                        parser->tokid = JTOK_STRING;
                        parser->input = cp;

                    } else {
                        mprSetJsonError(parser, "Unexpected input");
                    }

                } else if (parser->state == MPR_JSON_STATE_VALUE) {
                    if ((cp = strpbrk(parser->input, " \t\n\r:,}]")) == 0) {
                        cp = &parser->input[slen(parser->input)];
                    }
                    len = cp - parser->input;
                    value = mprGetBufStart(parser->buf);
                    mprPutBlockToBuf(parser->buf, parser->input, len);
                    mprAddNullToBuf(parser->buf);

                    if (scaselessmatch(value, "false")) {
                        parser->tokid = JTOK_FALSE;
                    } else if (scaselessmatch(value, "null")) {
                        parser->tokid = JTOK_NULL;
                    } else if (scaselessmatch(value, "true")) {
                        parser->tokid = JTOK_TRUE;
                    } else if (scaselessmatch(value, "undefined") && parser->tolerant) {
                        parser->tokid = JTOK_UNDEFINED;
                    } else if (sfnumber(value)) {
                        parser->tokid = JTOK_NUMBER;
                    } else {
                        parser->tokid = JTOK_STRING;
                    }
                    parser->input += len;

                } else {
                    mprSetJsonError(parser, "Unexpected input");
                }
                break;
            }
        }
    }
    mprAddNullToBuf(parser->buf);
    parser->token = mprGetBufStart(parser->buf);
    return parser->tokid;
}


/*
    Supports hashes where properties are strings or hashes of strings. N-level nest is supported.
 */
static char *objToString(MprBuf *buf, MprJson *obj, int indent, int flags)
{
    MprJson  *child;
    int     pretty, index;

    pretty = flags & MPR_JSON_PRETTY;

    if (obj->type & MPR_JSON_ARRAY) {
        mprPutCharToBuf(buf, '[');
        indent++;
        if (pretty) mprPutCharToBuf(buf, '\n');

        for (ITERATE_JSON(obj, child, index)) {
            if (pretty) spaces(buf, indent);
            objToString(buf, child, indent, flags);
            if (child->next != obj->children) {
                mprPutCharToBuf(buf, ',');
            }
            if (pretty) mprPutCharToBuf(buf, '\n');
        }
        if (pretty) spaces(buf, --indent);
        mprPutCharToBuf(buf, ']');

    } else if (obj->type & MPR_JSON_OBJ) {
        mprPutCharToBuf(buf, '{');
        indent++;
        if (pretty) mprPutCharToBuf(buf, '\n');
        for (ITERATE_JSON(obj, child, index)) {
            if (pretty) spaces(buf, indent);
            mprFormatJsonName(buf, child->name, flags);
            if (pretty) {
                mprPutStringToBuf(buf, ": ");
            } else {
                mprPutCharToBuf(buf, ':');
            }
            objToString(buf, child, indent, flags);
            if (child->next != obj->children) {
                mprPutCharToBuf(buf, ',');
            }
            if (pretty) mprPutCharToBuf(buf, '\n');
        }
        if (pretty) spaces(buf, --indent);
        mprPutCharToBuf(buf, '}');

    } else {
        mprFormatJsonValue(buf, obj->type, obj->value, flags);
    }
    return sclone(mprGetBufStart(buf));
}


/*
    Serialize into JSON format
 */
PUBLIC char *mprJsonToString(MprJson *obj, int flags)
{
    if (!obj) {
        return 0;
    }
    return objToString(mprCreateBuf(0, 0), obj, 0, flags);
}


static void setValue(MprJson *obj, cchar *value, int type)
{
    if (type == 0) {
        if (scaselessmatch(value, "false")) {
            type |= MPR_JSON_FALSE;
        } else if (value == 0 || scaselessmatch(value, "null")) {
            type |= MPR_JSON_NULL;
            value = 0;
        } else if (scaselessmatch(value, "true")) {
            type |= MPR_JSON_TRUE;
        } else if (scaselessmatch(value, "undefined")) {
            type |= MPR_JSON_UNDEFINED;
        } else if (sfnumber(value)) {
            type |= MPR_JSON_NUMBER;
        } else if (*value == '/' && value[slen(value) - 1] == '/') {
            type |= MPR_JSON_REGEXP;
        } else {
            type |= MPR_JSON_STRING;
        }
    }
    obj->value = sclone(value);
    obj->type = MPR_JSON_VALUE | type;
}


PUBLIC MprList *mprJsonToEnv(MprJson *json, cchar *prefix, MprList *list)
{
    MprJson     *jp;
    int         ji;

    if (!list) {
        list = mprCreateList(0, 0);
    }
    for (ITERATE_JSON(json, jp, ji)) {
        if (jp->type & MPR_JSON_VALUE) {
            mprAddItem(list, sfmt("%s_%s=%s", prefix, jp->name, jp->value));
        } else if (jp->type & (MPR_JSON_OBJ | MPR_JSON_ARRAY)) {
            list = mprJsonToEnv(jp, sfmt("%s_%s", prefix, jp->name), list);
        }
    }
    mprAddNullItem(list);
    return list;
}


PUBLIC void mprFormatJsonName(MprBuf *buf, cchar *name, int flags)
{
    cchar   *cp;
    int     quotes;

    quotes = flags & MPR_JSON_QUOTES;
    for (cp = name; *cp; cp++) {
        if (!isalnum((uchar) *cp) && *cp != '_') {
            quotes++;
            break;
        }
    }
    if (quotes) {
        mprPutCharToBuf(buf, '"');
    }
    for (cp = name; *cp; cp++) {
        if (*cp == '\"' || *cp == '\\') {
            mprPutCharToBuf(buf, '\\');
            mprPutCharToBuf(buf, *cp);
        } else if (*cp == '\b') {
            mprPutStringToBuf(buf, "\\b");
        } else if (*cp == '\f') {
            mprPutStringToBuf(buf, "\\f");
        } else if (*cp == '\n') {
            mprPutStringToBuf(buf, "\\n");
        } else if (*cp == '\r') {
            mprPutStringToBuf(buf, "\\r");
        } else if (*cp == '\t') {
            mprPutStringToBuf(buf, "\\t");
        } else if (iscntrl((uchar) *cp)) {
            mprPutToBuf(buf, "\\u%04x", *cp);
        } else {
            mprPutCharToBuf(buf, *cp);
        }
    }
    if (quotes) {
        mprPutCharToBuf(buf, '"');
    }
}


PUBLIC void mprFormatJsonString(MprBuf *buf, cchar *value)
{
    cchar   *cp;

    mprPutCharToBuf(buf, '"');
    for (cp = value; *cp; cp++) {
        if (*cp == '\"' || *cp == '\\') {
            mprPutCharToBuf(buf, '\\');
            mprPutCharToBuf(buf, *cp);
        } else if (*cp == '\b') {
            mprPutStringToBuf(buf, "\\b");
        } else if (*cp == '\f') {
            mprPutStringToBuf(buf, "\\f");
        } else if (*cp == '\n') {
            mprPutStringToBuf(buf, "\\n");
        } else if (*cp == '\r') {
            mprPutStringToBuf(buf, "\\r");
        } else if (*cp == '\t') {
            mprPutStringToBuf(buf, "\\t");
        } else if (iscntrl((uchar) *cp)) {
            mprPutToBuf(buf, "\\u%04x", *cp);
        } else {
            mprPutCharToBuf(buf, *cp);
        }
    }
    mprPutCharToBuf(buf, '"');
}


PUBLIC void mprFormatJsonValue(MprBuf *buf, int type, cchar *value, int flags)
{
    if (!(type & MPR_JSON_STRING) && !(flags & MPR_JSON_STRINGS)) {
        if (value == 0) {
            mprPutStringToBuf(buf, "null");
        } else if (type & MPR_JSON_REGEXP) {
            mprPutToBuf(buf, "/%s/", value);
        } else {
            mprPutStringToBuf(buf, value);
        }
        return;
    }
    switch (type & MPR_JSON_DATA_TYPE) {
    case MPR_JSON_FALSE:
    case MPR_JSON_NUMBER:
    case MPR_JSON_TRUE:
    case MPR_JSON_NULL:
    case MPR_JSON_UNDEFINED:
        mprPutStringToBuf(buf, value);
        break;
    case MPR_JSON_REGEXP:
        mprPutToBuf(buf, "/%s/", value);
        break;
    case MPR_JSON_STRING:
    default:
        mprFormatJsonString(buf, value);
    }
}


static void spaces(MprBuf *buf, int count)
{
    int     i;

    for (i = 0; i < count; i++) {
        mprPutStringToBuf(buf, "    ");
    }
}


static void jsonErrorCallback(MprJsonParser *parser, cchar *msg)
{
    if (!parser->errorMsg) {
        if (parser->path) {
            parser->errorMsg = sfmt("JSON Parse Error: %s\nIn file '%s' at line %d. Token \"%s\"",
                msg, parser->path, parser->lineNumber + 1, parser->token);
        } else {
            parser->errorMsg = sfmt("JSON Parse Error: %s\nAt line %d. Token \"%s\"",
                msg, parser->lineNumber + 1, parser->token);
        }
        mprDebug("mpr json", 4, "%s", parser->errorMsg);
    }
}


PUBLIC void mprSetJsonError(MprJsonParser *parser, cchar *fmt, ...)
{
    va_list     args;
    cchar       *msg;

    va_start(args, fmt);
    msg = sfmtv(fmt, args);
    (parser->callback.parseError)(parser, msg);
    va_end(args);
    parser->state = MPR_JSON_STATE_ERR;
    parser->tokid = JTOK_ERR;
}

/*
 **************** JSON object query API -- only works for MprJson implementations *****************
 */

PUBLIC int mprBlendJson(MprJson *dest, MprJson *src, int flags)
{
    MprJson     *dp, *sp, *child;
    cchar       *trimmedName;
    int         kind, si, pflags;

    if (dest == 0) {
        return MPR_ERR_BAD_ARGS;
    }
    if (src == 0) {
        return 0;
    }
    if ((MPR_JSON_OBJ_TYPE & dest->type) != (MPR_JSON_OBJ_TYPE & src->type)) {
        if (flags & (MPR_JSON_APPEND | MPR_JSON_REPLACE)) {
            return MPR_ERR_BAD_ARGS;
        }
    }
    if (src->type & MPR_JSON_OBJ) {
        if (flags & MPR_JSON_CREATE) {
            /* Already present */
        } else {
            /* Examine each property for: MPR_JSON_APPEND (default) | MPR_JSON_REPLACE) */
            for (ITERATE_JSON(src, sp, si)) {
                trimmedName = sp->name;
                pflags = flags;
                if (flags & MPR_JSON_COMBINE && sp->name) {
                    kind = sp->name[0];
                    if (kind == '+') {
                        pflags = MPR_JSON_APPEND | (flags & MPR_JSON_COMBINE);
                        trimmedName = &sp->name[1];
                    } else if (kind == '-') {
                        pflags = MPR_JSON_REPLACE | (flags & MPR_JSON_COMBINE);
                        trimmedName = &sp->name[1];
                    } else if (kind == '?') {
                        pflags = MPR_JSON_CREATE | (flags & MPR_JSON_COMBINE);
                        trimmedName = &sp->name[1];
                    } else if (kind == '=') {
                        pflags = MPR_JSON_OVERWRITE | (flags & MPR_JSON_COMBINE);
                        trimmedName = &sp->name[1];
                    }
                }
                if ((dp = mprReadJsonObj(dest, trimmedName)) == 0) {
                    /* Absent in destination */
                    if (pflags & MPR_JSON_COMBINE && sp->type == MPR_JSON_OBJ) {
                        dp = mprCreateJson(sp->type);
                        if (trimmedName == &sp->name[1]) {
                            trimmedName = sclone(trimmedName);
                        }
                        setProperty(dest, trimmedName, dp);
                        mprBlendJson(dp, sp, pflags);
                    } else if (!(pflags & MPR_JSON_REPLACE)) {
                        if (trimmedName == &sp->name[1]) {
                            trimmedName = sclone(trimmedName);
                        }
                        setProperty(dest, trimmedName, mprCloneJson(sp));
                    }
                } else if (!(pflags & MPR_JSON_CREATE)) {
                    /* Already present in destination */
                    if (sp->type & MPR_JSON_OBJ && (MPR_JSON_OBJ_TYPE & dp->type) != (MPR_JSON_OBJ_TYPE & sp->type)) {
                        dp = setProperty(dest, dp->name, mprCreateJson(sp->type));
                    }
                    mprBlendJson(dp, sp, pflags);

                    if (pflags & MPR_JSON_REPLACE &&
                            !(sp->type & (MPR_JSON_OBJ | MPR_JSON_ARRAY)) && sspace(dp->value)) {
                        mprRemoveJsonChild(dest, dp);
                    }
                }
            }
        }
    } else if (src->type & MPR_JSON_ARRAY) {
        if (flags & MPR_JSON_REPLACE) {
            for (ITERATE_JSON(src, sp, si)) {
                if ((child = mprReadJsonValue(dest, sp->value)) != 0) {
                    mprRemoveJsonChild(dest, child);
                }
            }
        } else if (flags & MPR_JSON_CREATE) {
            ;
        } else if (flags & MPR_JSON_APPEND) {
            for (ITERATE_JSON(src, sp, si)) {
                if ((child = mprReadJsonValue(dest, sp->value)) == 0) {
                    appendProperty(dest, mprCloneJson(sp));
                }
            }
        } else {
            /* Default is to MPR_JSON_OVERWRITE */
            if ((sp = mprCloneJson(src)) != 0) {
                adoptChildren(dest, sp);
            }
        }

    } else {
        /* Ordinary string value */
        if (src->value) {
            if (flags & MPR_JSON_APPEND) {
                setValue(dest, sjoin(dest->value, " ", src->value, NULL), src->type);
            } else if (flags & MPR_JSON_REPLACE) {
                setValue(dest, sreplace(dest->value, src->value, NULL), src->type);
            } else if (flags & MPR_JSON_CREATE) {
                /* Do nothing */
            } else {
                /* MPR_JSON_OVERWRITE (default) */
                dest->value = sclone(src->value);
            }
        }
    }
    return 0;
}


/*
    Simple one-level lookup. Returns the actual JSON object and not a clone.
 */
PUBLIC MprJson *mprReadJsonObj(MprJson *obj, cchar *name)
{
    MprJson      *child;
    int         i, index;

    if (!obj || !name) {
        return 0;
    }
    if (obj->type & MPR_JSON_OBJ) {
        for (ITERATE_JSON(obj, child, i)) {
            if (smatch(child->name, name)) {
                return child;
            }
        }
    } else if (obj->type & MPR_JSON_ARRAY) {
        /*
            Note this does a linear traversal counting array elements. Not the fastest.
            This code is not optimized for huge arrays.
         */
        if (*name == '$') {
            return 0;
        }
        index = (int) stoi(name);
        for (ITERATE_JSON(obj, child, i)) {
            if (i == index) {
                return child;
            }
        }
    }
    return 0;
}


PUBLIC cchar *mprReadJson(MprJson *obj, cchar *name)
{
    MprJson     *item;

    if ((item = mprReadJsonObj(obj, name)) != 0 && item->type & MPR_JSON_VALUE) {
        return item->value;
    }
    return 0;
}


PUBLIC MprJson *mprReadJsonValue(MprJson *obj, cchar *value)
{
    MprJson     *child;
    int         i;

    if (!obj || !value) {
        return 0;
    }
    for (ITERATE_JSON(obj, child, i)) {
        if (smatch(child->value, value)) {
            return child;
        }
    }
    return 0;
}


/*
    JSON expression operators
 */
#define JSON_OP_EQ          1
#define JSON_OP_NE          2
#define JSON_OP_LT          3
#define JSON_OP_LE          4
#define JSON_OP_GT          5
#define JSON_OP_GE          6
#define JSON_OP_MATCH       7
#define JSON_OP_NMATCH      8

#define JSON_PROP_CONTENTS  0x1         /* property has "@" for 'contains'. Only for array contents */
#define JSON_PROP_ELIPSIS   0x2         /* Property was after elipsis:  ..name */
#define JSON_PROP_EXPR      0x4         /* property has expression. Only for objects */
#define JSON_PROP_RANGE     0x8         /* Property is a range N:M */
#define JSON_PROP_WILD      0x10        /* Property is wildcard "*" */
#define JSON_PROP_COMPOUND  0xff        /* property is not just a simple string */
#define JSON_PROP_ARRAY     0x100       /* Hint that an array should be created */

/*
    Split a mulitpart property string and extract the token, deliminator and remaining portion.
    Format expected is: [delimitor] property [delimitor2] rest
    Delimitor characters are: . .. [ ]
    Properties may be simple expressions (field OP value)
    Returns the next property token.
 */
PUBLIC char *getNextTerm(MprJson *obj, char *str, char **rest, int *termType)
{
    char    *start, *end, *seps, *dot, *expr;
    ssize   i;

    assert(rest);

    if (!obj || !rest) {
        return 0;
    }
    /* Clang resports *rest as a false positive */
    start = str ? str : *rest;

    if (start == 0) {
        *rest = 0;
        return 0;
    }
    *termType = 0;
    seps = ".[]";

    while (isspace((int) *start)) start++;
    if (termType && *start == '.') {
        *termType |= JSON_PROP_ELIPSIS;
    }
    if ((i = strspn(start, seps)) > 0) {
        start += i;
    }
    if (*start == '\0') {
        *rest = 0;
        return 0;
    }
    if (*start == '*' && (start[1] == '\0' || start[1] == '.' || start[1] == ']')) {
        *termType |= JSON_PROP_WILD;
    } else if (*start == '@' && obj->type & MPR_JSON_ARRAY) {
        *termType |= JSON_PROP_CONTENTS;
    } else if (schr(start, ':') && obj->type & MPR_JSON_ARRAY) {
        *termType |= JSON_PROP_RANGE;
    }
    dot = strpbrk(start, ".[");
    expr = strpbrk(start, " \t]" JSON_EXPR_CHARS);

    if (expr && (!dot || (expr < dot))) {
        /* Assume in [FIELD OP VALUE] */
        end = strpbrk(start, "]");
    } else {
        end = strpbrk(start, seps);
    }
    if (end != 0) {
        if (*end == '[') {
            /* Hint that an array vs object should be created if required */
            *termType |= JSON_PROP_ARRAY;
            *end++ = '\0';
        } else if (*end == '.') {
            *end++ = '\0';
        } else {
            *end++ = '\0';
            i = strspn(end, seps);
            end += i;
            if (*end == '\0') {
                end = 0;
            }
        }
    }
    if (spbrk(start, JSON_EXPR_CHARS) && !(*termType & JSON_PROP_CONTENTS)) {
        *termType |= JSON_PROP_EXPR;
    }
    *rest = end;
    return start;
}


static char *splitExpression(char *property, int *operator, char **value)
{
    char    *seps, *op, *end, *vp;
    ssize   i;

    assert(property);
    assert(operator);
    assert(value);

    seps = JSON_EXPR_CHARS " \t";
    *value = 0;

    if ((op = spbrk(property, seps)) == 0) {
        return 0;
    }
    end = op;
    while (isspace((int) *op)) op++;
    if (end < op) {
        *end = '\0';
    }
    switch (op[0]) {
    case '<':
        *operator = (op[1] == '=') ? JSON_OP_LE: JSON_OP_LT;
        break;
    case '>':
        *operator = (op[1] == '=') ? JSON_OP_GE: JSON_OP_GT;
        break;
    case '=':
        *operator = JSON_OP_EQ;
        break;
    case '!':
        if (op[1] == '~') {
            *operator = JSON_OP_NMATCH;
        } else if (op[1] == '=') {
            *operator = JSON_OP_NE;
        } else {
            *operator = 0;
            return 0;
        }
        break;
    case '~':
        *operator = JSON_OP_MATCH;
        break;
    default:
        *operator = 0;
        return 0;
    }
    if ((vp = spbrk(op, "<>=! \t")) != 0) {
        *vp++ = '\0';
        i = sspn(vp, seps);
        vp += i;
        if (*vp == '\'' || *vp == '"' || *vp == '`') {
            for (end = &vp[1]; *end; end++) {
                if (*end == '\\' && end[1]) {
                    end++;
                } else if (*end == *vp) {
                    *end = '\0';
                    vp++;
                }
            }
        }
        *value = vp;
    }
    return property;
}


/*
    Note: value is modified
 */
static bool matchExpression(MprJson *obj, int operator, char *value)
{
    if (!(obj->type & MPR_JSON_VALUE)) {
        return 0;
    }
    if ((value = stok(value, "'\"", NULL)) == 0) {
        return 0;
    }
    switch (operator) {
    case JSON_OP_EQ:
        return smatch(obj->value, value);
    case JSON_OP_NE:
        return !smatch(obj->value, value);
    case JSON_OP_LT:
        return scmp(obj->value, value) < 0;
    case JSON_OP_LE:
        return scmp(obj->value, value) <= 0;
    case JSON_OP_GT:
        return scmp(obj->value, value) > 0;
    case JSON_OP_GE:
        return scmp(obj->value, value) >= 0;
    case JSON_OP_MATCH:
        return scontains(obj->value, value) != 0;
    case JSON_OP_NMATCH:
        return scontains(obj->value, value) == 0;
    default:
        return 0;
    }
}


static void appendProperty(MprJson *obj, MprJson *child)
{
    if (child) {
        setProperty(obj, child->name, child);
    }
}


static void appendItem(MprJson *obj, MprJson *child)
{
    if (child) {
        setProperty(obj, 0, child);
    }
}


/*
    WARNING: this steals properties from items
 */
static void appendItems(MprJson *obj, MprJson *items)
{
    MprJson  *child, *next;
    int     index;

    for (index = 0, child = items ? items->children: 0; items && index < items->length; child = next, index++) {
        next = child->next;
        appendItem(obj, child);
    }
}


/*
    Search all descendants down multiple levels: ".."
 */
static MprJson *queryElipsis(MprJson *obj, cchar *property, cchar *rest, MprJson *value, int flags)
{
    MprJson     *child, *result;
    cchar       *subkey;
    int         index;

    result = mprCreateJson(MPR_JSON_ARRAY);
    for (ITERATE_JSON(obj, child, index)) {
        if (smatch(child->name, property)) {
            if (rest == 0) {
                appendItem(result, queryLeaf(obj, property, value, flags));
            } else {
                appendItems(result, queryCore(child, rest, value, flags));
            }
        } else if (child->type & (MPR_JSON_ARRAY | MPR_JSON_OBJ)) {
            if (rest) {
                subkey = sjoin("..", property, ".", rest, NULL);
            } else {
                subkey = sjoin("..", property, NULL);
            }
            appendItems(result, queryCore(child, subkey, value, flags));
        }
    }
    return result;
}


/*
    Search wildcard values: "*"
 */
static MprJson *queryWild(MprJson *obj, cchar *property, cchar *rest, MprJson *value, int flags)
{
    MprJson     *child, *result;
    int         index;

    result = mprCreateJson(MPR_JSON_ARRAY);
    for (ITERATE_JSON(obj, child, index)) {
        if (rest == 0) {
            appendItem(result, queryLeaf(obj, child->name, value, flags));
        } else {
            appendItems(result, queryCore(child, rest, value, flags));
        }
    }
    return result;
}


/*
    Array contents match: [@ EXPR value]
 */
static MprJson *queryContents(MprJson *obj, char *property, cchar *rest, MprJson *value, int flags)
{
    MprJson     *child, *result;
    char        *v, ibuf[16];
    int         operator, index;

    result = mprCreateJson(MPR_JSON_ARRAY);
    if (!(obj->type & MPR_JSON_ARRAY)) {
        /* Cannot get here */
        return result;
    }
    if (splitExpression(property, &operator, &v) == 0) {
        return result;
    }
    for (ITERATE_JSON(obj, child, index)) {
        if (matchExpression(child, operator, v)) {
            if (rest == 0) {
                if (flags & JSON_REMOVE) {
                    appendItem(result, mprRemoveJsonChild(obj, child));
                } else {
                    appendItem(result, queryLeaf(obj, itosbuf(ibuf, sizeof(ibuf), index, 10), value, flags));
                }
            } else {
                /*  Should never get here as this means the array has objects instead of simple values */
                appendItems(result, queryCore(child, rest, value, flags));
            }
        }
    }
    return result;
}


/*
    Array range of elements
 */
static MprJson *queryRange(MprJson *obj, char *property, cchar *rest, MprJson *value, int flags)
{
    MprJson     *child, *result;
    ssize       start, end;
    char        *e, *s, ibuf[16];
    int         index;

    result = mprCreateJson(MPR_JSON_ARRAY);
    if (!(obj->type & MPR_JSON_ARRAY)) {
        return result;
    }
    if ((s = stok(property, ": \t", &e)) == 0) {
        return result;
    }
    start = (ssize) stoi(s);
    end = (ssize) stoi(e);
    if (start < 0) {
        start = obj->length + start;
    }
    if (end < 0) {
        end = obj->length + end;
    }
    for (ITERATE_JSON(obj, child, index)) {
        if (index < start) continue;
        if (index > end) break;
        if (rest == 0) {
            if (flags & JSON_REMOVE) {
                appendItem(result, mprRemoveJsonChild(obj, child));
            } else {
                appendItem(result, queryLeaf(obj, itosbuf(ibuf, sizeof(ibuf), index, 10), value, flags));
            }
        } else {
            appendItems(result, queryCore(child, rest, value, flags));
        }
    }
    return result;
}


/*
    Object property match: property EXPR value
 */
static MprJson *queryExpr(MprJson *obj, char *property, cchar *rest, MprJson *value, int flags)
{
    MprJson     *child, *result, *prop;
    char        *v;
    int         index, operator, pi;

    result = mprCreateJson(MPR_JSON_ARRAY);
    if ((property = splitExpression(property, &operator, &v)) == 0) {
        /* Expression does not parse and so does not match */
        return result;
    }
    for (ITERATE_JSON(obj, child, index)) {
        for (ITERATE_JSON(child, prop, pi)) {
            if (matchExpression(prop, operator, v)) {
                if (rest == 0) {
                    if (flags & JSON_REMOVE) {
                        appendItem(result, mprRemoveJsonChild(obj, child));
                    } else {
                        appendItem(result, queryLeaf(obj, property, value, flags));
                    }
                } else {
                    appendItems(result, queryCore(child, rest, value, flags));
                }
            }
        }
    }
    return result;
}


static MprJson *queryCompound(MprJson *obj, char *property, cchar *rest, MprJson *value, int flags, int termType)
{
    if (termType & JSON_PROP_ELIPSIS) {
        return queryElipsis(obj, property, rest, value, flags);

    } else if (termType & JSON_PROP_WILD) {
        return queryWild(obj, property, rest, value, flags);

    } else if (termType & JSON_PROP_CONTENTS) {
        return queryContents(obj, property, rest, value, flags);

    } else if (termType & JSON_PROP_RANGE) {
        return queryRange(obj, property, rest, value, flags);

    } else if (termType & JSON_PROP_EXPR) {
        return queryExpr(obj, property, rest, value, flags);
    }
    return 0;
}


/*
    Property must be a managed reference
    Value must be cloned so it can be freely linked
 */
static MprJson *queryLeaf(MprJson *obj, cchar *property, MprJson *value, int flags)
{
    MprJson     *child;

    assert(obj);
    assert(property && *property);

    if (value) {
        setProperty(obj, sclone(property), value);
        return 0;

    } else if (flags & JSON_REMOVE) {
        if ((child = mprReadJsonObj(obj, property)) != 0) {
            return mprRemoveJsonChild(obj, child);
        }
        return 0;

    } else {
        return mprCloneJson(mprReadJsonObj(obj, property));
    }
}


/*
    Query a JSON object for a property key path and execute the given command.
    The object may be a string, array or object.
    The path is a multipart property. Examples are:
        user.name
        user['name']
        users[2]
        users[2:4]
        users[-4:-1]                //  Range from end of array
        users[name == 'john']
        users[age >= 50]
        users[phone ~ ^206]         //  Starts with 206
        colors[@ != 'red']          //  Array element not 'red'
        people..[name == 'john']    //  Elipsis descends down multiple levels

    If a value is provided, the property described by the keyPath is set to the value.
    If flags is set to JSON_REMOVE, the property described by the keyPath is removed.
    If doing a get, the properties described by the keyPath are cloned and returned as the result.
    If doing a set, ....
    If doing a remove, the removed properties are returned.

    This routine recurses for query expressions. Normal property references are handled without recursion.

    For get, returns list of matching properties. These are cloned.
    For set, returns empty list if successful, else null.

    For remove, returns list of removed elements
 */
static MprJson *queryCore(MprJson *obj, cchar *key, MprJson *value, int flags)
{
    MprJson     *result, *child;
    char        *property, *rest;
    int         termType;

    if (obj == 0 || key == 0 || *key == '\0' || obj->type & MPR_JSON_VALUE) {
        return 0;
    }
    result = 0;
    for (property = getNextTerm(obj, sclone(key), &rest, &termType); property; ) {
        if (termType & JSON_PROP_COMPOUND) {
            result = queryCompound(obj, property, rest, value, flags, termType);
            break;

        } else if (rest == 0) {
            if (!result && !value) {
                result = mprCreateJson(MPR_JSON_ARRAY);
            }
            appendItem(result, queryLeaf(obj, property, value, flags));
            break;

        } else if ((child = mprReadJsonObj(obj, property)) == 0) {
            if (value) {
                child = mprCreateJson(termType & JSON_PROP_ARRAY ? MPR_JSON_ARRAY : MPR_JSON_OBJ);
                setProperty(obj, sclone(property), child);
            } else {
                break;
            }
        }
        obj = (MprJson*) child;
        property = getNextTerm(obj, 0, &rest, &termType);
    }
    return result ? result : mprCreateJson(MPR_JSON_ARRAY);
}


PUBLIC MprJson *mprQueryJson(MprJson *obj, cchar *key, cchar *value, int type)
{
    return queryCore(obj, key, value ? mprCreateJsonValue(value, type) : 0, 0);
}


PUBLIC MprJson *mprGetJsonObj(MprJson *obj, cchar *key)
{
    MprJson      *result;

    if (key && !strpbrk(key, ".[]*")) {
        return mprReadJsonObj(obj, key);
    }
    if ((result = mprQueryJson(obj, key, 0, 0)) != 0 && result->children) {
        return result->children;
    }
    return 0;
}


PUBLIC cchar *mprGetJson(MprJson *obj, cchar *key)
{
    MprJson      *result;

    if (key && !strpbrk(key, ".[]*")) {
        return mprReadJson(obj, key);
    }
    if ((result = mprQueryJson(obj, key, 0, 0)) != 0) {
        if (result->length == 1 && result->children->type & MPR_JSON_VALUE) {
            return result->children->value;
        } else if (result->length >= 1) {
            return mprJsonToString(result, 0);
        }
    }
    return 0;
}


PUBLIC int mprSetJsonObj(MprJson *obj, cchar *key, MprJson *value)
{
    if (key && !strpbrk(key, ".[]*")) {
        if (setProperty(obj, sclone(key), value) == 0) {
            return MPR_ERR_CANT_WRITE;
        }
    } else if (queryCore(obj, key, value, 0) == 0) {
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}


PUBLIC int mprSetJson(MprJson *obj, cchar *key, cchar *value, int type)
{
    if (key && !strpbrk(key, ".[]*")) {
        if (setProperty(obj, sclone(key), mprCreateJsonValue(value, type)) == 0) {
            return MPR_ERR_CANT_WRITE;
        }
    } else if (queryCore(obj, key, mprCreateJsonValue(value, type), 0) == 0) {
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}


PUBLIC MprJson *mprRemoveJson(MprJson *obj, cchar *key)
{
    return queryCore(obj, key, 0, JSON_REMOVE);
}


MprJson *mprLoadJson(cchar *path)
{
    char    *str;

    if ((str = mprReadPathContents(path, NULL)) != 0) {
        return mprParseJson(str);
    }
    return 0;
}


PUBLIC int mprSaveJson(MprJson *obj, cchar *path, int flags)
{
    MprFile     *file;
    ssize       len;
    char        *buf;

    if (flags == 0) {
        flags = MPR_JSON_PRETTY | MPR_JSON_QUOTES;
    }
    if ((buf = mprJsonToString(obj, flags)) == 0) {
        return MPR_ERR_BAD_FORMAT;
    }
    len = slen(buf);
    if ((file = mprOpenFile(path, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, 0644)) == 0) {
        return MPR_ERR_CANT_OPEN;
    }
    if (mprWriteFile(file, buf, len) != len) {
        mprCloseFile(file);
        return MPR_ERR_CANT_WRITE;
    }
    mprWriteFileString(file, "\n");
    mprCloseFile(file);
    return 0;
}


PUBLIC void mprLogJson(int level, MprJson *obj, cchar *fmt, ...)
{
    va_list     ap;
    char        *msg;

    va_start(ap, fmt);
    msg = sfmtv(fmt, ap);
    va_end(ap);
    mprLog("info mpr json", level, "%s: %s", msg, mprJsonToString(obj, MPR_JSON_PRETTY));
}


/*
    Add the child as property in the given object. The child is not cloned and is dedicated to this object.
    NOTE: name must be a managed reference. For arrays, name can be a string index value. If name is null or empty,
    then the property will be appended. This is the typical pattern for appending to an array.
 */
static MprJson *setProperty(MprJson *obj, cchar *name, MprJson *child)
{
    MprJson      *prior, *existing;

    if (!obj || !child) {
        return 0;
    }
    if ((existing = mprReadJsonObj(obj, name)) != 0) {
        existing->value = child->value;
        existing->children = child->children;
        existing->type = child->type;
        existing->length = child->length;
        return existing;
    }
    if (obj->children) {
        prior = obj->children->prev;
        child->next = obj->children;
        child->prev = prior;
        prior->next->prev = child;
        prior->next = child;
    } else {
        child->next = child->prev = child;
        obj->children = child;
    }
    child->name = name;
    obj->length++;
    return child;
}


static void adoptChildren(MprJson *obj, MprJson *other)
{
    if (obj && other) {
        obj->children = other->children;
        obj->length = other->length;
    }
}


static int checkBlockCallback(MprJsonParser *parser, cchar *name, bool leave)
{
    return 0;
}


/*
    Note: name is allocated
 */
static int setValueCallback(MprJsonParser *parser, MprJson *obj, cchar *name, MprJson *child)
{
    return setProperty(obj, name, child) != 0;
}


PUBLIC MprJson *mprRemoveJsonChild(MprJson *obj, MprJson *child)
{
    MprJson      *dep;
    int         index;

    for (ITERATE_JSON(obj, dep, index)) {
        if (dep == child) {
            if (--obj->length == 0) {
                obj->children = 0;
            } else if (obj->children == dep) {
                if (dep->next == dep) {
                    obj->children = 0;
                } else {
                    obj->children = dep->next;
                }
            }
            dep->prev->next = dep->next;
            dep->next->prev = dep->prev;
            child->next = child->prev = 0;
            return child;
        }
    }
    return 0;
}


/*
    Deep copy of an object
 */
PUBLIC MprJson *mprCloneJson(MprJson *obj)
{
    MprJson     *result, *child;
    int         index;

    if (obj == 0) {
        return 0;
    }
    result = mprCreateJson(obj->type);
    result->name = obj->name;
    result->value = obj->value;
    result->type = obj->type;
    for (ITERATE_JSON(obj, child, index)) {
        setProperty(result, child->name, mprCloneJson(child));
    }
    return result;
}


PUBLIC ssize mprGetJsonLength(MprJson *obj)
{
    if (!obj) {
        return 0;
    }
    return obj->length;
}


PUBLIC MprHash *mprDeserializeInto(cchar *str, MprHash *hash)
{
    MprJson     *obj, *child;
    int         index;

    obj = mprParseJson(str);
    for (ITERATE_JSON(obj, child, index)) {
        mprAddKey(hash, child->name, child->value);
    }
    return hash;
}


PUBLIC MprHash *mprDeserialize(cchar *str)
{
    return mprDeserializeInto(str, mprCreateHash(0, 0));
}


PUBLIC char *mprSerialize(MprHash *hash, int flags)
{
    MprJson  *obj;
    MprKey   *kp;
    cchar    *key;

    obj = mprCreateJson(MPR_JSON_OBJ);
    for (ITERATE_KEYS(hash, kp)) {
        key = (hash->flags & MPR_HASH_STATIC_KEYS) ? sclone(kp->key) : kp->key;
        setProperty(obj, key, mprCreateJsonValue(kp->data, 0));
    }
    return mprJsonToString(obj, flags);
}


PUBLIC MprJson *mprHashToJson(MprHash *hash)
{
    MprJson     *obj;
    MprKey      *kp;
    cchar       *key;

    obj = mprCreateJson(0);
    for (ITERATE_KEYS(hash, kp)) {
        key = (hash->flags & MPR_HASH_STATIC_KEYS) ? sclone(kp->key) : kp->key;
        setProperty(obj, key, mprCreateJsonValue(kp->data, 0));
    }
    return obj;
}


PUBLIC MprHash *mprJsonToHash(MprJson *json)
{
    MprHash     *hash;
    MprJson     *obj;
    int         index;

    hash = mprCreateHash(0, 0);
    for (ITERATE_JSON(json, obj, index)) {
        if (obj->type & MPR_JSON_VALUE) {
            mprAddKey(hash, obj->name, obj->value);
        }
    }
    return hash;
}


PUBLIC int mprWriteJson(MprJson *obj, cchar *key, cchar *value, int type)
{
    if (setProperty(obj, sclone(key), mprCreateJsonValue(value, type)) == 0) {
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}


PUBLIC int mprWriteJsonObj(MprJson *obj, cchar *key, MprJson *value)
{
    if (setProperty(obj, sclone(key), value) == 0) {
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}



