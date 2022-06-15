/* This module contains code for searching the table of Unicode character
properties. */

#include "me.h"
#include "libpcre.h"


#if CONFIG_COM_PCRE

#include "pcre_config.h"
#include "pcre_internal.h"
#include "ucp.h"
#include "ucpinternal.h"
#include "ucptable.h"



/* Table to translate from particular type value to the general value. */

static const int ucp_gentype[] = {
  ucp_C, ucp_C, ucp_C, ucp_C, ucp_C,  /* Cc, Cf, Cn, Co, Cs */
  ucp_L, ucp_L, ucp_L, ucp_L, ucp_L,  /* Ll, Lu, Lm, Lo, Lt */
  ucp_M, ucp_M, ucp_M,                /* Mc, Me, Mn */
  ucp_N, ucp_N, ucp_N,                /* Nd, Nl, No */
  ucp_P, ucp_P, ucp_P, ucp_P, ucp_P,  /* Pc, Pd, Pe, Pf, Pi */
  ucp_P, ucp_P,                       /* Ps, Po */
  ucp_S, ucp_S, ucp_S, ucp_S,         /* Sc, Sk, Sm, So */
  ucp_Z, ucp_Z, ucp_Z                 /* Zl, Zp, Zs */
};



/*************************************************
*         Search table and return type           *
*************************************************/

/* Three values are returned: the category is ucp_C, ucp_L, etc. The detailed
character type is ucp_Lu, ucp_Nd, etc. The script is ucp_Latin, etc.

Arguments:
  c           the character value
  type_ptr    the detailed character type is returned here
  script_ptr  the script is returned here

Returns:      the character type category
*/

int
_pcre_ucp_findprop(const unsigned int c, int *type_ptr, int *script_ptr)
{
int bot = 0;
int top = sizeof(ucp_table)/sizeof(cnode);
int mid;

/* The table is searched using a binary chop. You might think that using
intermediate variables to hold some of the common expressions would speed
things up, but tests with gcc 3.4.4 on Linux showed that, on the contrary, it
makes things a lot slower. */

for (;;)
  {
  if (top <= bot)
    {
    *type_ptr = ucp_Cn;
    *script_ptr = ucp_Common;
    return ucp_C;
    }
  mid = (bot + top) >> 1;
  if (c == (ucp_table[mid].f0 & f0_charmask)) break;
  if (c < (ucp_table[mid].f0 & f0_charmask)) top = mid;
  else
    {
    if ((ucp_table[mid].f0 & f0_rangeflag) != 0 &&
        c <= (ucp_table[mid].f0 & f0_charmask) +
             (ucp_table[mid].f1 & f1_rangemask)) break;
    bot = mid + 1;
    }
  }

/* Found an entry in the table. Set the script and detailed type values, and
return the general type. */

*script_ptr = (ucp_table[mid].f0 & f0_scriptmask) >> f0_scriptshift;
*type_ptr = (ucp_table[mid].f1 & f1_typemask) >> f1_typeshift;

return ucp_gentype[*type_ptr];
}



/*************************************************
*       Search table and return other case       *
*************************************************/

/* If the given character is a letter, and there is another case for the
letter, return the other case. Otherwise, return -1.

Arguments:
  c           the character value

Returns:      the other case or NOTACHAR if none
*/

unsigned int
_pcre_ucp_othercase(const unsigned int c)
{
int bot = 0;
int top = sizeof(ucp_table)/sizeof(cnode);
int mid, offset;

/* The table is searched using a binary chop. You might think that using
intermediate variables to hold some of the common expressions would speed
things up, but tests with gcc 3.4.4 on Linux showed that, on the contrary, it
makes things a lot slower. */

for (;;)
  {
  if (top <= bot) return -1;
  mid = (bot + top) >> 1;
  if (c == (ucp_table[mid].f0 & f0_charmask)) break;
  if (c < (ucp_table[mid].f0 & f0_charmask)) top = mid;
  else
    {
    if ((ucp_table[mid].f0 & f0_rangeflag) != 0 &&
        c <= (ucp_table[mid].f0 & f0_charmask) +
             (ucp_table[mid].f1 & f1_rangemask)) break;
    bot = mid + 1;
    }
  }

/* Found an entry in the table. Return NOTACHAR for a range entry. Otherwise
return the other case if there is one, else NOTACHAR. */

if ((ucp_table[mid].f0 & f0_rangeflag) != 0) return NOTACHAR;

offset = ucp_table[mid].f1 & f1_casemask;
if ((offset & f1_caseneg) != 0) offset |= f1_caseneg;
return (offset == 0)? NOTACHAR : c + offset;
}


/* End of pcre_ucp_searchfuncs.c */
#endif /* CONFIG_COM_PCRE */
