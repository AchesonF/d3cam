#include "libesp.h"

#if CONFIG_COM_ESP


/*
    espConfig.c -- ESP Configuration
 */

/*********************************** Includes *********************************/



/************************************* Locals *********************************/

#define ITERATE_CONFIG(route, obj, child, index) \
    index = 0, child = obj ? obj->children: 0; obj && index < obj->length && !route->error; child = child->next, index++
static void defineEnv(HttpRoute *route, cchar *key, cchar *value);

/************************************** Code **********************************/

static void loadApp(HttpRoute *parent, MprJson *prop)
{
    HttpRoute   *route;
    MprList     *files;
    cchar       *config, *prefix;
    int         next;

    if (prop->type & MPR_JSON_OBJ) {
        prefix = mprGetJson(prop, "prefix");
        config = mprGetJson(prop, "config");
        route = httpCreateInheritedRoute(parent);
        if (espInit(route, prefix, config) < 0) {
            httpParseError(route, "Cannot define ESP application at: %s", config);
            return;
        }
        httpFinalizeRoute(route);

    } else if (prop->type & MPR_JSON_STRING) {
        files = mprGlobPathFiles(".", prop->value, MPR_PATH_RELATIVE);
        for (ITERATE_ITEMS(files, config, next)) {
            route = httpCreateInheritedRoute(parent);
            prefix = mprGetPathBase(mprGetPathDir(mprGetAbsPath(config)));
            if (espInit(route, prefix, config) < 0) {
                httpParseError(route, "Cannot define ESP application at: %s", config);
                return;
            }
            httpFinalizeRoute(route);
        }
    }
}


static void parseEsp(HttpRoute *route, cchar *key, MprJson *prop)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    httpParseAll(route, key, prop);

    /*
        Fix ups
     */
    if (route->flags & HTTP_ROUTE_UTILITY) {
        eroute->compile = 1;
        eroute->update = 1;
    }
    if (eroute->app) {
        if (!mprLookupStringItem(route->indexes, "index.esp")) {
            httpAddRouteIndex(route, "index.esp");
        }
        if (!mprLookupStringItem(route->indexes, "index.html")) {
            httpAddRouteIndex(route, "index.html");
        }
    }
}

/*
    app: {
        source: [
            'patterns/ *.c',
        ],
        tokens: [
            CFLAGS: '-DMY=42'
        ],
    }
 */
static void parseApp(HttpRoute *route, cchar *key, MprJson *prop)
{
    EspRoute    *eroute;

    eroute = route->eroute;

    if (!(prop->type & MPR_JSON_OBJ)) {
        if (prop->type & MPR_JSON_TRUE) {
            eroute->app = 1;
        }
    } else {
        eroute->app = 1;
    }
    if (eroute->app) {
        /*
            Set some defaults before parsing "esp". This permits user overrides.
         */
        httpSetRouteXsrf(route, 1);
        httpAddRouteHandler(route, "espHandler", "");
        eroute->keep = smatch(route->mode, "release") == 0;
        espSetDefaultDirs(route, eroute->app);
    }
    if (prop->type & MPR_JSON_OBJ) {
        httpParseAll(route, key, prop);
    }
}


/*
    esp: {
        apps: 'myapp/esp.json',
        apps: [
            'apps/STAR/esp.json'
        ],
        apps: [
            { prefix: '/blog', config: 'blog/esp.json' }
        ],
    }
 */
static void parseApps(HttpRoute *route, cchar *key, MprJson *prop)
{
    MprJson     *child;
    int         ji;

    if (prop->type & MPR_JSON_STRING) {
        loadApp(route, prop);

    } else if (prop->type & MPR_JSON_OBJ) {
        loadApp(route, prop);

    } else if (prop->type & MPR_JSON_ARRAY) {
        for (ITERATE_CONFIG(route, prop, child, ji)) {
            loadApp(route, child);
        }
    }
}


static void parseCombine(HttpRoute *route, cchar *key, MprJson *prop)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    if (smatch(prop->value, "true")) {
        eroute->combine = 1;
    } else {
        eroute->combine = 0;
    }
}


static void parseCompile(HttpRoute *route, cchar *key, MprJson *prop)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    eroute->compile = (prop->type & MPR_JSON_TRUE) ? 1 : 0;
}


static void defineEnv(HttpRoute *route, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    MprJson     *child, *set;
    cchar       *arch;
    int         ji;

    eroute = route->eroute;
    if (smatch(key, "set")) {
        httpParsePlatform(HTTP->platform, NULL, &arch, NULL);

        if ((set = mprGetJsonObj(route->config, sfmt("esp.build.env.%s.default", value))) != 0) {
            for (ITERATE_CONFIG(route, set, child, ji)) {
                defineEnv(route, child->name, child->value);
            }
        }
        if ((set = mprGetJsonObj(route->config, sfmt("esp.build.env.%s.%s", value, arch))) != 0) {
            for (ITERATE_CONFIG(route, set, child, ji)) {
                defineEnv(route, child->name, child->value);
            }
        }

    } else {
        value = espExpandCommand(route, value, "", "");
        mprAddKey(eroute->env, key, value);
        if (scaselessmatch(key, "PATH")) {
            if (eroute->searchPath) {
                eroute->searchPath = sclone(value);
            } else {
                eroute->searchPath = sjoin(eroute->searchPath, MPR_SEARCH_SEP, value, NULL);
            }
        }
    }
}


static void parseBuild(HttpRoute *route, cchar *key, MprJson *prop)
{
    EspRoute    *eroute;
    MprJson     *child, *env, *rules;
    cchar       *buildType, *os, *rule, *stem;
    int         ji;

    eroute = route->eroute;
    buildType = HTTP->staticLink ? "static" : "dynamic";
    httpParsePlatform(HTTP->platform, &os, NULL, NULL);

    stem = sfmt("esp.build.rules.%s.%s", buildType, os);
    if ((rules = mprGetJsonObj(route->config, stem)) == 0) {
        stem = sfmt("esp.build.rules.%s.default", buildType);
        rules = mprGetJsonObj(route->config, stem);
    }
    if (rules) {
        if ((rule = mprGetJson(route->config, sfmt("%s.%s", stem, "compile"))) != 0) {
            eroute->compileCmd = rule;
        }
        if ((rule = mprGetJson(route->config, sfmt("%s.%s", stem, "link"))) != 0) {
            eroute->linkCmd = rule;
        }
        if ((env = mprGetJsonObj(route->config, sfmt("%s.%s", stem, "env"))) != 0) {
            if (eroute->env == 0) {
                eroute->env = mprCreateHash(-1, MPR_HASH_STABLE);
            }
            for (ITERATE_CONFIG(route, env, child, ji)) {
                /* WARNING: yields */
                defineEnv(route, child->name, child->value);
            }
        }
    } else {
        httpParseError(route, "Cannot find compile rules for O/S \"%s\"", os);
    }
}


static void parseKeep(HttpRoute *route, cchar *key, MprJson *prop)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    eroute->keep = (prop->type & MPR_JSON_TRUE) ? 1 : 0;
}


static void parseOptimize(HttpRoute *route, cchar *key, MprJson *prop)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    eroute->compileMode = smatch(prop->value, "true") ? ESP_COMPILE_OPTIMIZED : ESP_COMPILE_SYMBOLS;
}


static void parseUpdate(HttpRoute *route, cchar *key, MprJson *prop)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    eroute->update = (prop->type & MPR_JSON_TRUE) ? 1 : 0;
}


static void serverRouteSet(HttpRoute *route, cchar *set)
{
    httpAddRestfulRoute(route, "GET,POST", "/{action}(/)*$", "${action}", "{controller}");
}


static void restfulRouteSet(HttpRoute *route, cchar *set)
{
    httpAddResourceGroup(route, "{controller}");
}


static void postRouteSet(HttpRoute *route, cchar *set)
{
    httpAddPostGroup(route, "{controller}");
}

static void spaRouteSet(HttpRoute *route, cchar *set)
{
    httpAddSpaGroup(route, "{controller}");
}

#if DEPRECATED && REMOVE
static void legacyRouteSet(HttpRoute *route, cchar *set)
{
    restfulRouteSet(route, "restful");
}
#endif


PUBLIC int espInitParser()
{
    httpDefineRouteSet("esp-server", serverRouteSet);
    httpDefineRouteSet("esp-restful", restfulRouteSet);
    httpDefineRouteSet("esp-spa", spaRouteSet);
    httpDefineRouteSet("esp-post", postRouteSet);
#if DEPRECATE && REMOVE
    httpDefineRouteSet("esp-html-mvc", legacyRouteSet);
#endif
    httpAddConfig("esp", parseEsp);
    httpAddConfig("esp.app", parseApp);
    httpAddConfig("esp.apps", parseApps);
    httpAddConfig("esp.build", parseBuild);
    httpAddConfig("esp.combine", parseCombine);
    httpAddConfig("esp.compile", parseCompile);
    httpAddConfig("esp.keep", parseKeep);
    httpAddConfig("esp.optimize", parseOptimize);
    httpAddConfig("esp.update", parseUpdate);
    return 0;
}


#endif




