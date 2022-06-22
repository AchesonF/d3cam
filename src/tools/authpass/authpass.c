/* srcfile: authpass.c -- Authorization password management.

    authpass [--cipher md5|blowfish] [--file filename] [--password password] realm user roles...

    This file provides facilities for creating passwords. It supports the MD5 or Blowfish ciphers.
 */


#include    "webapp.h"


#define MAX_PASS    64

static cchar        *programName;

static char *getPassword(void)
{
    char    *password, *confirm;

    password = getpass("New password: ");
    confirm = getpass("Confirm password: ");
    if (smatch(password, confirm)) {
        return password;
    }
    mprLog("error authpass", 0, "Password not verified");
    return 0;
}


static void printUsage(cchar *programName)
{
    mprEprintf("usage: %s [--cipher cipher] [--file path] [--password password] realm user roles...\n"
        "Options:\n"
        "    --cipher md5|blowfish Select the encryption cipher. Defaults to md5\n"
        "    --file filename       Modify the password file\n"
        "    --password password   Use the specified password\n"
        "\n", programName);
}



int main(int argc, char *argv[])
{
    MprBuf      *buf;
    HttpRoute   *route;
    HttpAuth    *auth;
    char        *password, *authFile, *username, *encodedPassword, *realm, *cp, *roles, *cipher;
    int         i, errflg, nextArg;

    mprCreate(argc, argv, 0);
    programName = mprGetAppName();

    authFile = 0;
    username = 0;
    errflg = 0;
    password = 0;
    cipher = "blowfish";

    for (i = 1; i < argc && !errflg; i++) {
        if (argv[i][0] != '-') {
            break;
        }
        for (cp = &argv[i][1]; *cp && !errflg; cp++) {
            if (*cp == '-') {
                cp++;
            }
            if (smatch(cp, "cipher")) {
                if (++i == argc) {
                    errflg++;
                } else {
                    cipher = argv[i];
                    if (!smatch(cipher, "md5") && !smatch(cipher, "blowfish")) {
                        mprLog("error authpass", 0, "Unknown cipher \"%s\". Use \"md5\" or \"blowfish\".", cipher);
                    }
                    break;
                }

            } else if (smatch(cp, "file") || smatch(cp, "f")) {
                if (++i == argc) {
                    errflg++;
                } else {
                    authFile = argv[i];
                    break;
                }

            } else if (smatch(cp, "password") || smatch(cp, "p")) {
                if (++i == argc) {
                    errflg++;
                } else {
                    password = argv[i];
                    break;
                }

            } else {
                errflg++;
            }
        }
    }
    nextArg = i;

    if ((nextArg + 3) > argc) {
        errflg++;
    }
    if (errflg) {
        printUsage(programName);
        exit(2);
    }
    realm = argv[nextArg++];
    username = argv[nextArg++];

    buf = mprCreateBuf(0, 0);
    for (i = nextArg; i < argc; ) {
        mprPutStringToBuf(buf, argv[i]);
        if (++i < argc) {
            mprPutCharToBuf(buf, ' ');
        }
    }
    roles = sclone(mprGetBufStart(buf));

    if (httpCreate(HTTP_SERVER_SIDE) == 0) {
        exit(3);
    }
    route = httpGetDefaultRoute(NULL);
    auth = route->auth;

    if (authFile && mprPathExists(authFile, R_OK)) {
        if (maParseConfig(authFile) < 0) {
            exit(4);
        }
    }
    if (!password && (password = getPassword()) == 0) {
        exit(7);
    }
    if (smatch(cipher, "md5")) {
        encodedPassword = mprGetMD5(sfmt("%s:%s:%s", username, realm, password));
    } else {
        /* This uses the more secure blowfish cipher */
        encodedPassword = mprMakePassword(sfmt("%s:%s:%s", username, realm, password), 16, 128);
    }
    if (authFile) {
        httpRemoveUser(auth, username);
        if (httpAddUser(auth, username, encodedPassword, roles) == 0) {
            exit(8);
        }
        if (maWriteAuthFile(auth, authFile) < 0) {
            exit(9);
        }
    } else {
        printf("%s\n", encodedPassword);
    }
    mprDestroy();
    return 0;
}




