#include "libmpr.h"


/**
    string.c - String routines safe for embedded programming

    This module provides safe replacements for the standard string library.
    Most routines in this file are not thread-safe. It is the callers responsibility to perform all thread synchronization.

 */

/********************************** Includes **********************************/



/*********************************** Locals ***********************************/

#define HASH_PRIME 0x01000193

/************************************ Code ************************************/

PUBLIC char *itos(int64 value)
{
    return itosradix(value, 10);
}


/*
    Format a number as a string. Support radix 10 and 16.
 */
PUBLIC char *itosradix(int64 value, int radix)
{
    char    numBuf[32];
    char    *cp;
    char    digits[] = "0123456789ABCDEF";
    int     negative;

    if (radix != 10 && radix != 16) {
        return 0;
    }
    cp = &numBuf[sizeof(numBuf)];
    *--cp = '\0';

    if (value < 0) {
        negative = 1;
        value = -value;
    } else {
        negative = 0;
    }
    do {
        *--cp = digits[value % radix];
        value /= radix;
    } while (value > 0);

    if (negative) {
        *--cp = '-';
    }
    return sclone(cp);
}


PUBLIC char *itosbuf(char *buf, ssize size, int64 value, int radix)
{
    char    *cp, *end;
    char    digits[] = "0123456789ABCDEF";
    int     negative;

    if ((radix != 10 && radix != 16) || size < 2) {
        return 0;
    }
    end = cp = &buf[size];
    *--cp = '\0';

    if (value < 0) {
        negative = 1;
        value = -value;
        size--;
    } else {
        negative = 0;
    }
    do {
        *--cp = digits[value % radix];
        value /= radix;
    } while (value > 0 && cp > buf);

    if (negative) {
        if (cp <= buf) {
            return 0;
        }
        *--cp = '-';
    }
    if (buf < cp) {
        /* Move the null too */
        memmove(buf, cp, end - cp);
    }
    return buf;
}


PUBLIC char *scamel(cchar *str)
{
    char    *ptr;
    ssize   size, len;

    if (str == 0) {
        str = "";
    }
    len = slen(str);
    size = len + 1;
    if ((ptr = mprAlloc(size)) != 0) {
        memcpy(ptr, str, len);
        ptr[len] = '\0';
        ptr[0] = (char) tolower((uchar) ptr[0]);
    }
    return ptr;
}


/*
    Case insensitive string comparison. Limited by length
 */
PUBLIC int scaselesscmp(cchar *s1, cchar *s2)
{
    if (s1 == 0) {
        return -1;
    } else if (s2 == 0) {
        return 1;
    }
    return sncaselesscmp(s1, s2, max(slen(s1), slen(s2)));
}


PUBLIC bool scaselessmatch(cchar *s1, cchar *s2)
{
    return scaselesscmp(s1, s2) == 0;
}


PUBLIC char *schr(cchar *s, int c)
{
    if (s == 0) {
        return 0;
    }
    return strchr(s, c);
}


PUBLIC char *sncontains(cchar *str, cchar *pattern, ssize limit)
{
    cchar   *cp, *s1, *s2;
    ssize   lim;

    if (limit < 0) {
        limit = MAXINT;
    }
    if (str == 0) {
        return 0;
    }
    if (pattern == 0 || *pattern == '\0') {
        return 0;
    }
    for (cp = str; limit > 0 && *cp; cp++, limit--) {
        s1 = cp;
        s2 = pattern;
        for (lim = limit; lim > 0 && *s1 && *s2 && (*s1 == *s2); lim--) {
            s1++;
            s2++;
        }
        if (*s2 == '\0') {
            return (char*) cp;
        }
    }
    return 0;
}


PUBLIC char *scontains(cchar *str, cchar *pattern)
{
    return sncontains(str, pattern, -1);
}


PUBLIC char *sncaselesscontains(cchar *str, cchar *pattern, ssize limit)
{
    cchar   *cp, *s1, *s2;
    ssize   lim;

    if (limit < 0) {
        limit = MAXINT;
    }
    if (str == 0) {
        return 0;
    }
    if (pattern == 0 || *pattern == '\0') {
        return 0;
    }
    for (cp = str; limit > 0 && *cp; cp++, limit--) {
        s1 = cp;
        s2 = pattern;
        for (lim = limit; lim > 0 && *s1 && *s2 && (tolower((uchar) *s1) == tolower((uchar) *s2)); lim--) {
            s1++;
            s2++;
        }
        if (*s2 == '\0') {
            return (char*) cp;
        }
    }
    return 0;
}


PUBLIC char *scaselesscontains(cchar *str, cchar *pattern)
{
    return sncaselesscontains(str, pattern, -1);
}

/*
    Copy a string into a buffer. Always ensure it is null terminated
 */
PUBLIC ssize scopy(char *dest, ssize destMax, cchar *src)
{
    ssize      len;

    assert(src);
    assert(dest);
    if (!src || !dest) {
        return 0;
    }
    assert(0 < destMax && destMax < MAXINT);

    len = slen(src);
    /* Must ensure room for null */
    if (destMax <= len) {
        assert(!MPR_ERR_WONT_FIT);
        if (destMax > 0) {
            *dest = '\0';
        }
        return MPR_ERR_WONT_FIT;
    }
    strcpy(dest, src);
    return len;
}


PUBLIC char *sclone(cchar *str)
{
    char    *ptr;
    ssize   size, len;

    if (str == 0) {
        str = "";
    }
    len = slen(str);
    size = len + 1;
    if ((ptr = mprAlloc(size)) != 0) {
        memcpy(ptr, str, len);
        ptr[len] = '\0';
    }
    return ptr;
}


PUBLIC int scmp(cchar *s1, cchar *s2)
{
    if (s1 == s2) {
        return 0;
    } else if (s1 == 0) {
        return -1;
    } else if (s2 == 0) {
        return 1;
    }
    return sncmp(s1, s2, max(slen(s1), slen(s2)));
}


PUBLIC cchar *sends(cchar *str, cchar *suffix)
{
    if (str == 0 || suffix == 0) {
        return 0;
    }
    if (slen(str) < slen(suffix)) {
        return 0;
    }
    if (strcmp(&str[slen(str) - slen(suffix)], suffix) == 0) {
        return &str[slen(str) - slen(suffix)];
    }
    return 0;
}


PUBLIC char *sfmt(cchar *format, ...)
{
    va_list     ap;
    char        *buf;

    if (format == 0) {
        format = "%s";
    }
    va_start(ap, format);
    buf = mprPrintfCore(NULL, -1, format, ap);
    va_end(ap);
    return buf;
}


PUBLIC char *sfmtv(cchar *format, va_list arg)
{
    return mprPrintfCore(NULL, -1, format, arg);
}


PUBLIC uint shash(cchar *cname, ssize len)
{
    uint    hash;

    assert(cname);
    assert(0 <= len && len < MAXINT);

    if (cname == 0) {
        return 0;
    }
    hash = (uint) len;
    while (len-- > 0) {
        hash ^= *cname++;
        hash *= HASH_PRIME;
    }
    return hash;
}


/*
    Hash the lower case name
 */
PUBLIC uint shashlower(cchar *cname, ssize len)
{
    uint    hash;

    assert(cname);
    assert(0 <= len && len < MAXINT);

    if (cname == 0) {
        return 0;
    }
    hash = (uint) len;
    while (len-- > 0) {
        hash ^= tolower((uchar) *cname++);
        hash *= HASH_PRIME;
    }
    return hash;
}


PUBLIC char *sjoin(cchar *str, ...)
{
    va_list     ap;
    char        *result;

    va_start(ap, str);
    result = sjoinv(str, ap);
    va_end(ap);
    return result;
}


PUBLIC char *sjoinv(cchar *buf, va_list args)
{
    va_list     ap;
    char        *dest, *str, *dp;
    ssize       required;

    va_copy(ap, args);
    required = 1;
    if (buf) {
        required += slen(buf);
    }
    str = va_arg(ap, char*);
    while (str) {
        required += slen(str);
        str = va_arg(ap, char*);
    }
    if ((dest = mprAlloc(required)) == 0) {
        va_end(ap);
        return 0;
    }
    dp = dest;
    if (buf) {
        strcpy(dp, buf);
        dp += slen(buf);
    }
    va_copy(ap, args);
    str = va_arg(ap, char*);
    while (str) {
        strcpy(dp, str);
        dp += slen(str);
        str = va_arg(ap, char*);
    }
    *dp = '\0';
    va_end(ap);
    return dest;
}


PUBLIC ssize slen(cchar *s)
{
    return s ? strlen(s) : 0;
}


/*
    Map a string to lower case. Allocates a new string.
 */
PUBLIC char *slower(cchar *str)
{
    char    *cp, *s;

    if (str) {
        s = sclone(str);
        for (cp = s; *cp; cp++) {
            if (isupper((uchar) *cp)) {
                *cp = (char) tolower((uchar) *cp);
            }
        }
        str = s;
    }
    return (char*) str;
}


PUBLIC bool smatch(cchar *s1, cchar *s2)
{
    return scmp(s1, s2) == 0;
}


/*
    Secure constant time comparison
*/
PUBLIC bool smatchsec(cchar *s1, cchar *s2)
{
    ssize   i, len1, len2;
    uchar   c;

    len1 = slen(s1);
    len2 = slen(s2);
    if (len1 != len2) {
        return 0;
    }
    for (i = 0, c = 0; i < len1; i++) {
        c |= (uchar) s1[i] ^ (uchar) s2[i];
    }
    return !c;
}


PUBLIC int sncaselesscmp(cchar *s1, cchar *s2, ssize n)
{
    int     rc;

    assert(0 <= n && n < MAXINT);

    if (s1 == 0) {
        return -1;
    } else if (s2 == 0) {
        return 1;
    }
    for (rc = 0; n > 0 && *s1 && rc == 0; s1++, s2++, n--) {
        rc = tolower((uchar) *s1) - tolower((uchar) *s2);
    }
    if (rc) {
        return (rc > 0) ? 1 : -1;
    } else if (n == 0) {
        return 0;
    } else if (*s1 == '\0' && *s2 == '\0') {
        return 0;
    } else if (*s1 == '\0') {
        return -1;
    } else if (*s2 == '\0') {
        return 1;
    }
    return 0;
}


/*
    Clone a sub-string of a specified length. The null is added after the length. The given len can be longer than the
    source string.
 */
PUBLIC char *snclone(cchar *str, ssize len)
{
    char    *ptr;
    ssize   size, l;

    if (str == 0) {
        str = "";
    }
    l = slen(str);
    len = min(l, len);
    size = len + 1;
    if ((ptr = mprAlloc(size)) != 0) {
        memcpy(ptr, str, len);
        ptr[len] = '\0';
    }
    return ptr;
}


/*
    Case sensitive string comparison. Limited by length
 */
PUBLIC int sncmp(cchar *s1, cchar *s2, ssize n)
{
    int     rc;

    assert(0 <= n && n < MAXINT);

    if (s1 == 0 && s2 == 0) {
        return 0;
    } else if (s1 == 0) {
        return -1;
    } else if (s2 == 0) {
        return 1;
    }
    for (rc = 0; n > 0 && *s1 && rc == 0; s1++, s2++, n--) {
        rc = *s1 - *s2;
    }
    if (rc) {
        return (rc > 0) ? 1 : -1;
    } else if (n == 0) {
        return 0;
    } else if (*s1 == '\0' && *s2 == '\0') {
        return 0;
    } else if (*s1 == '\0') {
        return -1;
    } else if (*s2 == '\0') {
        return 1;
    }
    return 0;
}


/*
    This routine copies at most "count" characters from a string. It ensures the result is always null terminated and
    the buffer does not overflow. Returns MPR_ERR_WONT_FIT if the buffer is too small.
 */
PUBLIC ssize sncopy(char *dest, ssize destMax, cchar *src, ssize count)
{
    ssize      len;

    assert(dest);
    assert(src);
    assert(src != dest);
    assert(0 <= count && count < MAXINT);
    assert(0 < destMax && destMax < MAXINT);

    //  OPT use strnlen(src, count);
    len = slen(src);
    len = min(len, count);
    if (destMax <= len) {
        if (destMax > 0) {
            *dest = '\0';
        }
        assert(!MPR_ERR_WONT_FIT);
        return MPR_ERR_WONT_FIT;
    }
    if (len > 0) {
        memcpy(dest, src, len);
        dest[len] = '\0';
    } else {
        *dest = '\0';
        len = 0;
    }
    return len;
}


PUBLIC bool snumber(cchar *s)
{
    if (!s) {
        return 0;
    }
    if (*s == '-' || *s == '+') {
        s++;
    }
    return s && *s && strspn(s, "1234567890") == strlen(s);
}


PUBLIC bool sspace(cchar *s)
{
    if (!s) {
        return 1;
    }
    while (isspace((uchar) *s)) {
        s++;
    }
    if (*s == '\0') {
        return 1;
    }
    return 0;
}


/*
    Hex
 */
PUBLIC bool shnumber(cchar *s)
{
    return s && *s && strspn(s, "1234567890abcdefABCDEFxX") == strlen(s);
}


/*
    Floating point
    Float:      [DIGITS].[DIGITS][(e|E)[+|-]DIGITS]
 */
PUBLIC bool sfnumber(cchar *s)
{
    cchar   *cp;
    int     dots, valid;

    valid = s && *s && strspn(s, "1234567890.+-eE") == strlen(s) && strspn(s, "1234567890") > 0;
    if (valid) {
        /*
            Some extra checks
         */
        for (cp = s, dots = 0; *cp; cp++) {
            if (*cp == '.') {
                if (dots++ > 0) {
                    valid = 0;
                    break;
                }
            }
        }
    }
    return valid;
}


PUBLIC char *stitle(cchar *str)
{
    char    *ptr;
    ssize   size, len;

    if (str == 0) {
        str = "";
    }
    len = slen(str);
    size = len + 1;
    if ((ptr = mprAlloc(size)) != 0) {
        memcpy(ptr, str, len);
        ptr[len] = '\0';
        ptr[0] = (char) toupper((uchar) ptr[0]);
    }
    return ptr;
}


PUBLIC char *spbrk(cchar *str, cchar *set)
{
    cchar       *sp;
    int         count;

    if (str == 0 || set == 0) {
        return 0;
    }
    for (count = 0; *str; count++, str++) {
        for (sp = set; *sp; sp++) {
            if (*str == *sp) {
                return (char*) str;
            }
        }
    }
    return 0;
}


PUBLIC char *srchr(cchar *s, int c)
{
    if (s == 0) {
        return 0;
    }
    return strrchr(s, c);
}


PUBLIC char *srejoin(char *buf, ...)
{
    va_list     args;
    char        *result;

    va_start(args, buf);
    result = srejoinv(buf, args);
    va_end(args);
    return result;
}


PUBLIC char *srejoinv(char *buf, va_list args)
{
    va_list     ap;
    char        *dest, *str, *dp;
    ssize       len, required;

    va_copy(ap, args);
    len = slen(buf);
    required = len + 1;
    str = va_arg(ap, char*);
    while (str) {
        required += slen(str);
        str = va_arg(ap, char*);
    }
    if ((dest = mprRealloc(buf, required)) == 0) {
        va_end(ap);
        return 0;
    }
    dp = &dest[len];
    va_copy(ap, args);
    str = va_arg(ap, char*);
    while (str) {
        strcpy(dp, str);
        dp += slen(str);
        str = va_arg(ap, char*);
    }
    *dp = '\0';
    va_end(ap);
    return dest;
}


PUBLIC char *sreplace(cchar *str, cchar *pattern, cchar *replacement)
{
    MprBuf      *buf;
    cchar       *s;
    ssize       plen;

    if (!pattern || pattern[0] == '\0') {
        return sclone(str);
    }
    buf = mprCreateBuf(-1, -1);
    plen = slen(pattern);
    for (s = str; *s; s++) {
        if (sncmp(s, pattern, plen) == 0) {
            if (replacement) {
                mprPutStringToBuf(buf, replacement);
            }
            s += plen - 1;
        } else {
            mprPutCharToBuf(buf, *s);
        }
    }
    mprAddNullToBuf(buf);
    return sclone(mprGetBufStart(buf));
}


/*
    Split a string at a substring and return the parts.
    This differs from stok in that it never returns null. Also, stok eats leading deliminators, whereas
    ssplit will return an empty string if there are leading deliminators.
    Note: Modifies the original string and returns the string for chaining.
 */
PUBLIC char *ssplit(char *str, cchar *delim, char **last)
{
    char    *end;

    if (last) {
        *last = MPR->emptyString;
    }
    if (str == 0) {
        return MPR->emptyString;
    }
    if (delim == 0 || *delim == '\0') {
        return str;
    }
    if ((end = strpbrk(str, delim)) != 0) {
        *end++ = '\0';
        end += strspn(end, delim);
    } else {
        end = MPR->emptyString;
    }
    if (last) {
        *last = end;
    }
    return str;
}


PUBLIC ssize sspn(cchar *str, cchar *set)
{
    if (str == 0 || set == 0) {
        return 0;
    }
    return strspn(str, set);
}


PUBLIC bool sstarts(cchar *str, cchar *prefix)
{
    if (str == 0 || prefix == 0) {
        return 0;
    }
    if (strncmp(str, prefix, slen(prefix)) == 0) {
        return 1;
    }
    return 0;
}


PUBLIC int64 stoi(cchar *str)
{
    return stoiradix(str, 10, NULL);
}


PUBLIC double stof(cchar *str)
{
    if (str == 0 || *str == 0) {
        return 0.0;
    }
    return atof(str);
}


/*
    Parse a number and check for parse errors. Supports radix 8, 10 or 16.
    If radix is <= 0, then the radix is sleuthed from the input.
    Supports formats:
        [(+|-)][0][OCTAL_DIGITS]
        [(+|-)][0][(x|X)][HEX_DIGITS]
        [(+|-)][DIGITS]
 */
PUBLIC int64 stoiradix(cchar *str, int radix, int *err)
{
    cchar   *start;
    int64   val;
    int     n, c, negative;

    if (err) {
        *err = 0;
    }
    if (str == 0) {
        if (err) {
            *err = MPR_ERR_BAD_SYNTAX;
        }
        return 0;
    }
    while (isspace((uchar) *str)) {
        str++;
    }
    val = 0;
    if (*str == '-') {
        negative = 1;
        str++;
    } else {
        negative = 0;
    }
    start = str;
    if (radix <= 0) {
        radix = 10;
        if (*str == '0') {
            if (tolower((uchar) str[1]) == 'x') {
                radix = 16;
                str += 2;
            } else {
                radix = 8;
                str++;
            }
        }

    } else if (radix == 16) {
        if (*str == '0' && tolower((uchar) str[1]) == 'x') {
            str += 2;
        }

    } else if (radix > 10) {
        radix = 10;
    }
    if (radix == 16) {
        while (*str) {
            c = tolower((uchar) *str);
            if (isdigit((uchar) c)) {
                val = (val * radix) + c - '0';
            } else if (c >= 'a' && c <= 'f') {
                val = (val * radix) + c - 'a' + 10;
            } else {
                break;
            }
            str++;
        }
    } else {
        while (*str && isdigit((uchar) *str)) {
            n = *str - '0';
            if (n >= radix) {
                break;
            }
            val = (val * radix) + n;
            str++;
        }
    }
    if (str == start) {
        /* No data */
        if (err) {
            *err = MPR_ERR_BAD_SYNTAX;
        }
        return 0;
    }
    return (negative) ? -val: val;
}


/*
    Note "str" is modifed as per strtok()
    WARNING:  this does not allocate
 */
PUBLIC char *stok(char *str, cchar *delim, char **last)
{
    char    *start, *end;
    ssize   i;

    assert(delim);
    start = (str || !last) ? str : *last;
    if (start == 0) {
        if (last) {
            *last = 0;
        }
        return 0;
    }
    i = strspn(start, delim);
    start += i;
    if (*start == '\0') {
        if (last) {
            *last = 0;
        }
        return 0;
    }
    end = strpbrk(start, delim);
    if (end) {
        *end++ = '\0';
        i = strspn(end, delim);
        end += i;
    }
    if (last) {
        *last = end;
    }
    return start;
}


/*
    Tokenize a string at a pattern and return the parts. The delimiter is a string not a set of characters.
    If the pattern is not found, last is set to null.
    Note: Modifies the original string and returns the string for chaining.
 */
PUBLIC char *sptok(char *str, cchar *pattern, char **last)
{
    char    *cp, *end;

    if (last) {
        *last = MPR->emptyString;
    }
    if (str == 0) {
        return 0;
    }
    if (pattern == 0 || *pattern == '\0') {
        return str;
    }
    if ((cp = strstr(str, pattern)) != 0) {
        *cp = '\0';
        end = &cp[slen(pattern)];
    } else {
        end = 0;
    }
    if (last) {
        *last = end;
    }
    return str;
}


PUBLIC char *ssub(cchar *str, ssize offset, ssize len)
{
    char    *result;
    ssize   size;

    assert(str);
    assert(offset >= 0);
    assert(0 <= len && len < MAXINT);

    if (str == 0) {
        return 0;
    }
    size = len + 1;
    if ((result = mprAlloc(size)) == 0) {
        return 0;
    }
    sncopy(result, size, &str[offset], len);
    return result;
}


/*
    Trim characters from the given set. Returns a newly allocated string.
 */
PUBLIC char *strim(cchar *str, cchar *set, int where)
{
    char    *s;
    ssize   len, i;

    if (str == 0 || set == 0) {
        return 0;
    }
    if (where == 0) {
        where = MPR_TRIM_START | MPR_TRIM_END;
    }
    if (where & MPR_TRIM_START) {
        i = strspn(str, set);
    } else {
        i = 0;
    }
    s = sclone(&str[i]);
    if (where & MPR_TRIM_END) {
        len = slen(s);
        while (len > 0 && strspn(&s[len - 1], set) > 0) {
            s[len - 1] = '\0';
            len--;
        }
    }
    return s;
}


/*
    Map a string to upper case
 */
PUBLIC char *supper(cchar *str)
{
    char    *cp, *s;

    if (str) {
        s = sclone(str);
        for (cp = s; *cp; cp++) {
            if (islower((uchar) *cp)) {
                *cp = (char) toupper((uchar) *cp);
            }
        }
        str = s;
    }
    return (char*) str;
}


/*
    Expand ${token} references in a path or string.
 */
static char *stemplateInner(cchar *str, void *keys, int json)
{
    MprBuf      *buf;
    cchar       *value;
    char        *src, *result, *cp, *tok;

    if (str) {
        if (schr(str, '$') == 0) {
            return sclone(str);
        }
        buf = mprCreateBuf(0, 0);
        for (src = (char*) str; *src; ) {
            if (*src == '$') {
                if (*++src == '{') {
                    for (cp = ++src; *cp && *cp != '}'; cp++) ;
                    tok = snclone(src, cp - src);
                } else {
                    for (cp = src; *cp && (isalnum((uchar) *cp) || *cp == '_'); cp++) ;
                    tok = snclone(src, cp - src);
                }
                if (json) {
                    value = mprGetJson(keys, tok);
                } else {
                    value = mprLookupKey(keys, tok);
                }
                if (value != 0) {
                    mprPutStringToBuf(buf, value);
                    if (src > str && src[-1] == '{') {
                        src = cp + 1;
                    } else {
                        src = cp;
                    }
                } else {
                    mprPutCharToBuf(buf, '$');
                    if (src > str && src[-1] == '{') {
                        mprPutCharToBuf(buf, '{');
                    }
                    mprPutCharToBuf(buf, *src++);
                }
            } else {
                mprPutCharToBuf(buf, *src++);
            }
        }
        mprAddNullToBuf(buf);
        result = sclone(mprGetBufStart(buf));
    } else {
        result = MPR->emptyString;
    }
    return result;
}


PUBLIC char *stemplate(cchar *str, MprHash *keys)
{
    return stemplateInner(str, keys, 0);
}

PUBLIC char *stemplateJson(cchar *str, MprJson *obj)
{
    return stemplateInner(str, obj, 1);
}


/*
    String to list. This parses the string into space separated arguments. Single and double quotes are supported.
    This returns a stable list.
 */
PUBLIC MprList *stolist(cchar *src)
{
    MprList     *list;
    cchar       *start;
    int         quote;

    list = mprCreateList(0, MPR_LIST_STABLE);
    while (src && *src != '\0') {
        while (isspace((uchar) *src)) {
            src++;
        }
        if (*src == '\0')  {
            break;
        }
        for (quote = 0, start = src; *src; src++) {
            if (*src == '\\') {
                src++;
            } else if (*src == '"' || *src == '\'') {
                if (*src == quote) {
                    src++;
                    break;
                } else if (quote == 0) {
                    quote = *src;
                }
            } else if (isspace((uchar) *src) && !quote) {
                break;
            }
        }
        mprAddItem(list, snclone(start, src - start));
    }
    return list;
}


PUBLIC cchar *sjoinArgs(int argc, cchar **argv, cchar *sep)
{
    MprBuf  *buf;
    int     i;

    if (sep == 0) {
        sep = "";
    }
    buf = mprCreateBuf(0, 0);
    for (i = 0; i < argc; i++) {
        mprPutToBuf(buf, "%s%s", argv[i], sep);
    }
    if (argc > 0) {
        mprAdjustBufEnd(buf, -1);
    }
    return mprBufToString(buf);
}


PUBLIC void serase(char *str)
{
    char    *cp;

    for (cp = str; cp && *cp; ) {
        *cp++ = '\0';
    }
}




