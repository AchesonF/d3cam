/**
    osdep.h -- O/S abstraction for products using MakeMe.
 */

#ifndef _h_OSDEP
#define _h_OSDEP 1

/********************************** Includes **********************************/

#include "me.h"

/******************************* Default Features *****************************/
/*
    MakeMe defaults
 */
#ifndef CONFIG_COM_SSL
    #define CONFIG_COM_SSL 0                /**< Build without SSL support */
#endif
#ifndef CONFIG_DEBUG
    #define CONFIG_DEBUG 0                  /**< Default to a debug build */
#endif
#ifndef ME_FLOAT
    #define ME_FLOAT 1                  /**< Build with floating point support */
#endif


/********************************* CPU Families *******************************/
/*
    CPU Architectures
 */
#define ME_CPU_UNKNOWN     0
#define ME_CPU_ARM         1           /**< Arm */
#define ME_CPU_ITANIUM     2           /**< Intel Itanium */
#define ME_CPU_X86         3           /**< X86 */
#define ME_CPU_X64         4           /**< AMD64 or EMT64 */
#define ME_CPU_MIPS        5           /**< Mips */
#define ME_CPU_PPC         6           /**< Power PC */
#define ME_CPU_SPARC       7           /**< Sparc */
#define ME_CPU_TIDSP       8           /**< TI DSP */
#define ME_CPU_SH          9           /**< SuperH */

/*
    Byte orderings
 */
#define ME_LITTLE_ENDIAN   1           /**< Little endian byte ordering */
#define ME_BIG_ENDIAN      2           /**< Big endian byte ordering */

/*
    Use compiler definitions to determine the CPU type.
    The default endianness can be overridden by configure --endian big|little.
 */
#if defined(__arm__)
    #define ME_CPU "arm"
    #define ME_CPU_ARCH ME_CPU_ARM
    #define CPU_ENDIAN ME_LITTLE_ENDIAN

#elif defined(__arm64__) || defined(__aarch64__)
    #define ME_CPU "arm"
    #define ME_CPU_ARCH ME_CPU_ARM
    #define CPU_ENDIAN ME_LITTLE_ENDIAN

#elif defined(__x86_64__) || defined(_M_AMD64)
    #define ME_CPU "x64"
    #define ME_CPU_ARCH ME_CPU_X64
    #define CPU_ENDIAN ME_LITTLE_ENDIAN

#elif defined(__i386__) || defined(__i486__) || defined(__i585__) || defined(__i686__) || defined(_M_IX86)
    #define ME_CPU "x86"
    #define ME_CPU_ARCH ME_CPU_X86
    #define CPU_ENDIAN ME_LITTLE_ENDIAN

#else
    #error "Cannot determine CPU type in osdep.h"
#endif

/*
    Set the default endian if me.h does not define it explicitly
 */
#ifndef ME_ENDIAN
    #define ME_ENDIAN CPU_ENDIAN
#endif

#define CONFIG_OS "linux"
#define LINUX 1
#define ME_UNIX_LIKE 1
#define ME_WIN_LIKE 0


#if __WORDSIZE == 64 || __amd64 || __x86_64 || __x86_64__ || _WIN64 || __mips64 || __arch64__ || __arm64__ || __aarch64__
    #define ME_64 1
    #define ME_WORDSIZE 64
#else
    #define ME_64 0
    #define ME_WORDSIZE 32
#endif

/*
    Unicode
 */
#ifndef ME_CHAR_LEN
    #define ME_CHAR_LEN 1
#endif
#if ME_CHAR_LEN == 4
    typedef int wchar;
    #define UT(s) L ## s
    #define UNICODE 1
#elif ME_CHAR_LEN == 2
    typedef short wchar;
    #define UT(s) L ## s
    #define UNICODE 1
#else
    typedef char wchar;
    #define UT(s) s
#endif

#define ME_PLATFORM CONFIG_OS "-" ME_CPU "-" CONFIG_PROFILE

/********************************* O/S Includes *******************************/
/*
    Out-of-order definitions and includes. Order really matters in this section.
 */


/*
    Use GNU extensions for:
        RTLD_DEFAULT for dlsym()
 */
#define _GNU_SOURCE 1
#if !ME_64
    #define _LARGEFILE64_SOURCE 1
    #ifdef __USE_FILE_OFFSET64
        #define _FILE_OFFSET_BITS 64
    #endif
#endif

/*
    Includes in alphabetic order
 */
#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>

#if ME_FLOAT
  #include <float.h>
  #define __USE_ISOC99 1
  #include <math.h>
#endif

#include <grp.h>
#include <libgen.h>
#include <limits.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>

#include <pthread.h>
#include <pwd.h>
#include <resolv.h>

#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/utsname.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <poll.h>
#include <unistd.h>
#include <time.h>
#include <wchar.h>


#include <linux/version.h>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/sendfile.h>

/************************************** Types *********************************/
/*
    Standard types
 */
#ifndef HAS_BOOL
    #ifndef __cplusplus
        #define HAS_BOOL 1
        /**
            Boolean data type.
         */
        typedef char bool;
    #endif
#endif

#ifndef HAS_UCHAR
    #define HAS_UCHAR 1
    /**
        Unsigned char data type.
     */
    typedef unsigned char uchar;
#endif

#ifndef HAS_SCHAR
    #define HAS_SCHAR 1
    /**
        Signed char data type.
     */
    typedef signed char schar;
#endif

#ifndef HAS_CCHAR
    #define HAS_CCHAR 1
    /**
        Constant char data type.
     */
    typedef const char cchar;
#endif

#ifndef HAS_CUCHAR
    #define HAS_CUCHAR 1
    /**
        Unsigned char data type.
     */
    typedef const unsigned char cuchar;
#endif

#ifndef HAS_USHORT
    #define HAS_USHORT 1
    /**
        Unsigned short data type.
     */
    typedef unsigned short ushort;
#endif

#ifndef HAS_CUSHORT
    #define HAS_CUSHORT 1
    /**
        Constant unsigned short data type.
     */
    typedef const unsigned short cushort;
#endif

#ifndef HAS_CVOID
    #define HAS_CVOID 1
    /**
        Constant void data type.
     */
    typedef const void cvoid;
#endif

#ifndef HAS_INT8
    #define HAS_INT8 1
    /**
        Integer 8 bits data type.
     */
    typedef char int8;
#endif

#ifndef HAS_UINT8
    #define HAS_UINT8 1
    /**
        Unsigned integer 8 bits data type.
     */
    typedef unsigned char uint8;
#endif

#ifndef HAS_INT16
    #define HAS_INT16 1
    /**
        Integer 16 bits data type.
     */
    typedef short int16;
#endif

#ifndef HAS_UINT16
    #define HAS_UINT16 1
    /**
        Unsigned integer 16 bits data type.
     */
    typedef unsigned short uint16;
#endif

#ifndef HAS_INT32
    #define HAS_INT32 1
    /**
        Integer 32 bits data type.
     */
    typedef int int32;
#endif

#ifndef HAS_UINT32
    #define HAS_UINT32 1
    /**
        Unsigned integer 32 bits data type.
     */
    typedef unsigned int uint32;
#endif

#ifndef HAS_UINT
    #define HAS_UINT 1
    /**
        Unsigned integer (machine dependent bit size) data type.
     */
    typedef unsigned int uint;
#endif

#ifndef HAS_ULONG
    #define HAS_ULONG 1
    /**
        Unsigned long (machine dependent bit size) data type.
     */
    typedef unsigned long ulong;
#endif

#ifndef HAS_CINT
    #define HAS_CINT 1
    /**
        Constant int data type.
     */
    typedef const int cint;
#endif

#ifndef HAS_SSIZE
    #define HAS_SSIZE 1

    /**
        Signed integer size field large enough to hold a pointer offset.
     */
    typedef ssize_t ssize;
#endif


typedef ssize wsize;


#ifndef HAS_INT64
    __extension__ typedef long long int int64;
#endif

#ifndef HAS_UINT64
    __extension__ typedef unsigned long long int uint64;
#endif

/**
    Signed file offset data type. Supports large files greater than 4GB in size on all systems.
 */
typedef int64 Offset;

/** Size to hold the length of a socket address */
typedef socklen_t Socklen;

/** Argument for sockets */
typedef int Socket;
#ifndef SOCKET_ERROR
    #define SOCKET_ERROR -1
#endif
#define SOCKET_ERROR -1
#ifndef INVALID_SOCKET
    #define INVALID_SOCKET -1
#endif

typedef int64 Time;

/**
    Elapsed time data type. Stores time in milliseconds from some arbitrary start epoch.
 */
typedef int64 Ticks;

/**
    Time/Ticks units per second (milliseconds)
 */
#define TPS 1000

/*********************************** Defines **********************************/

#ifndef BITSPERBYTE
    #define BITSPERBYTE     ((int) (8 * sizeof(char)))
#endif

#ifndef BITS
    #define BITS(type)      ((int) (BITSPERBYTE * (int) sizeof(type)))
#endif

#if ME_FLOAT
    #ifndef MAXFLOAT
        #define MAXFLOAT        FLT_MAX
    #endif

    #define isNan(f) (isnan(f))
#endif

#define INT64(x)    (x##LL)
#define UINT64(x)   (x##ULL)

#ifndef MAXINT
#if INT_MAX
    #define MAXINT      INT_MAX
#else
    #define MAXINT      0x7fffffff
#endif
#endif

#ifndef MAXUINT
#if UINT_MAX
    #define MAXUINT     UINT_MAX
#else
    #define MAXUINT     0xffffffff
#endif
#endif

#ifndef MAXINT64
    #define MAXINT64    INT64(0x7fffffffffffffff)
#endif
#ifndef MAXUINT64
    #define MAXUINT64   INT64(0xffffffffffffffff)
#endif

#if SIZE_T_MAX
    #define MAXSIZE     SIZE_T_MAX
#elif ME_64
    #define MAXSIZE     INT64(0xffffffffffffffff)
#else
    #define MAXSIZE     MAXINT
#endif

#if SSIZE_T_MAX
    #define MAXSSIZE     SSIZE_T_MAX
#elif ME_64
    #define MAXSSIZE     INT64(0x7fffffffffffffff)
#else
    #define MAXSSIZE     MAXINT
#endif

#if OFF_T_MAX
    #define MAXOFF       OFF_T_MAX
#else
    #define MAXOFF       INT64(0x7fffffffffffffff)
#endif

/*
    Word size and conversions between integer and pointer.
 */
#if ME_64
    #define ITOP(i)     ((void*) ((int64) i))
    #define PTOI(i)     ((int) ((int64) i))
    #define LTOP(i)     ((void*) ((int64) i))
    #define PTOL(i)     ((int64) i)
#else
    #define ITOP(i)     ((void*) ((int) i))
    #define PTOI(i)     ((int) i)
    #define LTOP(i)     ((void*) ((int) i))
    #define PTOL(i)     ((int64) (int) i)
#endif

#undef PUBLIC
#undef PUBLIC_DATA
#undef PRIVATE


#define PUBLIC
#define PUBLIC_DATA extern
#define PRIVATE     static


/* Undefines for Qt - Ugh */
#undef max
#undef min

#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))

#ifndef PRINTF_ATTRIBUTE
    #if ((__GNUC__ >= 3) && !DOXYGEN)
        /**
            Use gcc attribute to check printf fns.  a1 is the 1-based index of the parameter containing the format,
            and a2 the index of the first argument. Note that some gcc 2.x versions don't handle this properly
         */
        #define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
    #else
        #define PRINTF_ATTRIBUTE(a1, a2)
    #endif
#endif

/*
    Optimize expression evaluation code depending if the value is likely or not
 */
#undef likely
#undef unlikely
#if (__GNUC__ >= 3)
    #define likely(x)   __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x)   (x)
    #define unlikely(x) (x)
#endif

#if !__UCLIBC__ && __USE_XOPEN2K
    #define ME_COMPILER_HAS_SPINLOCK    1
#endif

#ifdef __USE_FILE_OFFSET64
    #define ME_COMPILER_HAS_OFF64 1
#else
    #define ME_COMPILER_HAS_OFF64 0
#endif

#define ME_COMPILER_HAS_FCNTL 1

#ifndef R_OK
    #define R_OK    4
    #define W_OK    2
    #define X_OK    1
    #define F_OK    0
#endif

#define LD_LIBRARY_PATH "LD_LIBRARY_PATH"

#define ARRAY_FLEX


/*
    Deprecated API warnings
 */
#if (__GNUC__ >= 3) && CONFIG_DEPRECATED_WARNINGS
    #define ME_DEPRECATED(MSG) __attribute__ ((deprecated(MSG)))
#else
    #define ME_DEPRECATED(MSG)
#endif

#define NOT_USED(x) ((void*) x)

/********************************** Tunables *********************************/
/*
    These can be defined in main.bit settings (pascal case) to override. E.g.

    settings: {
        maxPath: 4096
    }
 */
#ifndef ME_MAX_FNAME
    #define ME_MAX_FNAME        256         /**< Reasonable filename size */
#endif
#ifndef ME_MAX_PATH
    #define ME_MAX_PATH         1024        /**< Reasonable filename size */
#endif
#ifndef ME_BUFSIZE
    #define ME_BUFSIZE          4096        /**< Reasonable size for buffers */
#endif
#ifndef ME_MAX_BUFFER
    #define ME_MAX_BUFFER       ME_BUFSIZE  /* DEPRECATE */
#endif

#ifndef ME_MAX_ARGC
    #define ME_MAX_ARGC         32          /**< Maximum number of command line args if using MAIN()*/
#endif
#ifndef ME_DOUBLE_BUFFER
    #define ME_DOUBLE_BUFFER    (DBL_MANT_DIG - DBL_MIN_EXP + 4)
#endif
#ifndef ME_MAX_IP
    #define ME_MAX_IP           1024
#endif


#ifndef ME_STACK_SIZE
/*
    If the system supports virtual memory, then stack size should use system default. Only used pages will
    actually consume memory
*/
#define ME_STACK_SIZE    0               /**< Default thread stack size (0 means use system default) */

#endif

/*********************************** Fixups ***********************************/

#ifndef ME_INLINE
    #define ME_INLINE inline
#endif


#define FILE_TEXT        ""
#define FILE_BINARY      ""


#define ME_COMPILER_HAS_MACRO_VARARGS 1

#define closesocket(x)  close(x)
#if !defined(PTHREAD_MUTEX_RECURSIVE_NP) || FREEBSD
    #define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#else
    #ifndef PTHREAD_MUTEX_RECURSIVE
        #define PTHREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE_NP
    #endif
#endif

#ifndef O_BINARY
    #define O_BINARY    0
#endif
#ifndef O_TEXT
    #define O_TEXT      0
#endif


#ifdef SIGINFO
    #define ME_SIGINFO SIGINFO
#elif defined(SIGPWR)
    #define ME_SIGINFO SIGPWR
#else
    #define ME_SIGINFO (0)
#endif


#ifndef NBBY
    #define NBBY 8
#endif

/*********************************** Externs **********************************/

#ifdef __cplusplus
extern "C" {
#endif


extern int pthread_mutexattr_gettype (__const pthread_mutexattr_t *__restrict __attr, int *__restrict __kind);
extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind);
extern char **environ;



#ifdef __cplusplus
}
#endif

#endif /* _h_OSDEP */




