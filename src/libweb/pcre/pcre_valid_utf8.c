/* This module contains an internal function for validating UTF-8 character
strings. */


#include "me.h"
#include "libpcre.h"


#if CONFIG_COM_PCRE

#include "pcre_config.h"
#include "pcre_internal.h"
#include "ucp.h"
#include "ucpinternal.h"
#include "ucptable.h"



/*************************************************
*         Validate a UTF-8 string                *
*************************************************/

/* This function is called (optionally) at the start of compile or match, to
validate that a supposed UTF-8 string is actually valid. The early check means
that subsequent code can assume it is dealing with a valid string. The check
can be turned off for maximum performance, but the consequences of supplying
an invalid string are then undefined.

Originally, this function checked according to RFC 2279, allowing for values in
the range 0 to 0x7fffffff, up to 6 bytes long, but ensuring that they were in
the canonical format. Once somebody had pointed out RFC 3629 to me (it
obsoletes 2279), additional restrictions were applied. The values are now
limited to be between 0 and 0x0010ffff, no more than 4 bytes long, and the
subrange 0xd000 to 0xdfff is excluded.

Arguments:
  string       points to the string
  length       length of string, or -1 if the string is zero-terminated

Returns:       < 0    if the string is a valid UTF-8 string
               >= 0   otherwise; the value is the offset of the bad byte
*/

int
_pcre_valid_utf8(const uschar *string, int length)
{
#ifdef SUPPORT_UTF8
register const uschar *p;

if (length < 0)
  {
  for (p = string; *p != 0; p++);
  length = (int) (p - string);
  }

for (p = string; length-- > 0; p++)
  {
  register int ab;
  register int c = *p;
  if (c < 128) continue;
  if (c < 0xc0) return (int) (p - string);
  ab = _pcre_utf8_table4[c & 0x3f];     /* Number of additional bytes */
  if (length < ab || ab > 3) return (int) (p - string);
  length -= ab;

  /* Check top bits in the second byte */
  if ((*(++p) & 0xc0) != 0x80) return (int) (p - string);

  /* Check for overlong sequences for each different length, and for the
  excluded range 0xd000 to 0xdfff.  */

  switch (ab)
    {
    /* Check for xx00 000x (overlong sequence) */

    case 1:
    if ((c & 0x3e) == 0) return (int) (p - string);
    continue;   /* We know there aren't any more bytes to check */

    /* Check for 1110 0000, xx0x xxxx (overlong sequence) or
                 1110 1101, 1010 xxxx (0xd000 - 0xdfff) */

    case 2:
    if ((c == 0xe0 && (*p & 0x20) == 0) ||
        (c == 0xed && *p >= 0xa0))
      return (int) (p - string);
    break;

    /* Check for 1111 0000, xx00 xxxx (overlong sequence) or
       greater than 0x0010ffff (f4 8f bf bf) */

    case 3:
    if ((c == 0xf0 && (*p & 0x30) == 0) ||
        (c > 0xf4 ) ||
        (c == 0xf4 && *p > 0x8f))
      return (int) (p - string);
    break;

#if 0
    /* These cases can no longer occur, as we restrict to a maximum of four
    bytes nowadays. Leave the code here in case we ever want to add an option
    for longer sequences. */

    /* Check for 1111 1000, xx00 0xxx */
    case 4:
    if (c == 0xf8 && (*p & 0x38) == 0) return p - string;
    break;

    /* Check for leading 0xfe or 0xff, and then for 1111 1100, xx00 00xx */
    case 5:
    if (c == 0xfe || c == 0xff ||
       (c == 0xfc && (*p & 0x3c) == 0)) return p - string;
    break;
#endif

    }

  /* Check for valid bytes after the 2nd, if any; all must start 10 */
  while (--ab > 0)
    {
    if ((*(++p) & 0xc0) != 0x80) return (int) (p - string);
    }
  }
#endif

return -1;
}

/* End of pcre_valid_utf8.c */
#endif /* CONFIG_COM_PCRE */
