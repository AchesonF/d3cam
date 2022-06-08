
/* Settings */
#ifndef CONFIG_COMPANY
    #define CONFIG_COMPANY "ht"
#endif

#ifndef CONFIG_COMPILER_HAS_ATOMIC
    #define CONFIG_COMPILER_HAS_ATOMIC 0
#endif

#ifndef CONFIG_COMPILER_HAS_DYN_LOAD
    #define CONFIG_COMPILER_HAS_DYN_LOAD 1
#endif

#ifndef CONFIG_COMPILER_HAS_PAM
    #define CONFIG_COMPILER_HAS_PAM 0
#endif

#ifndef CONFIG_COMPILER_HAS_SYNC
    #define CONFIG_COMPILER_HAS_SYNC 1
#endif
#ifndef CONFIG_COMPILER_HAS_SYNC64
    #define CONFIG_COMPILER_HAS_SYNC64 1
#endif
#ifndef CONFIG_COMPILER_HAS_SYNC_CAS
    #define CONFIG_COMPILER_HAS_SYNC_CAS 0
#endif

#ifndef CONFIG_FILE
    #define CONFIG_FILE "webapp.conf"
#endif
#ifndef CONFIG_DEBUG
    #define CONFIG_DEBUG 0
#endif
#ifndef CONFIG_DEPRECATED_WARNINGS
    #define CONFIG_DEPRECATED_WARNINGS 0
#endif

#ifndef CONFIG_HTTP_BASIC
    #define CONFIG_HTTP_BASIC 1
#endif
#ifndef CONFIG_HTTP_CACHE
    #define CONFIG_HTTP_CACHE 1
#endif
#ifndef CONFIG_HTTP_DEFENSE
    #define CONFIG_HTTP_DEFENSE 1
#endif
#ifndef CONFIG_HTTP_DIGEST
    #define CONFIG_HTTP_DIGEST 1
#endif
#ifndef CONFIG_HTTP_DIR
    #define CONFIG_HTTP_DIR 1
#endif
#ifndef CONFIG_HTTP_HTTP2
    #define CONFIG_HTTP_HTTP2 1
#endif
#ifndef CONFIG_HTTP_PAM
    #define CONFIG_HTTP_PAM 1
#endif
#ifndef CONFIG_HTTP_SENDFILE
    #define CONFIG_HTTP_SENDFILE 1
#endif
#ifndef CONFIG_HTTP_UPLOAD
    #define CONFIG_HTTP_UPLOAD 1
#endif
#ifndef CONFIG_HTTP_WEB_SOCKETS
    #define CONFIG_HTTP_WEB_SOCKETS 1
#endif
#ifndef CONFIG_MPR_LOGGING
    #define CONFIG_MPR_LOGGING 1
#endif
#ifndef CONFIG_MPR_OSLOG
    #define CONFIG_MPR_OSLOG 0
#endif

#ifndef CONFIG_MPR_SSL_CACHE
    #define CONFIG_MPR_SSL_CACHE 512
#endif
#ifndef CONFIG_MPR_SSL_HANDSHAKES
    #define CONFIG_MPR_SSL_HANDSHAKES 3
#endif
#ifndef CONFIG_MPR_SSL_LOG_LEVEL
    #define CONFIG_MPR_SSL_LOG_LEVEL 6
#endif
#ifndef CONFIG_MPR_SSL_TICKET
    #define CONFIG_MPR_SSL_TICKET 1
#endif
#ifndef CONFIG_MPR_SSL_TIMEOUT
    #define CONFIG_MPR_SSL_TIMEOUT 86400
#endif
#ifndef CONFIG_MPR_THREAD_LIMIT_BY_CORES
    #define CONFIG_MPR_THREAD_LIMIT_BY_CORES 1
#endif
#ifndef CONFIG_MPR_THREAD_STACK
    #define CONFIG_MPR_THREAD_STACK 0
#endif
#ifndef CONFIG_NAME
    #define CONFIG_NAME "webapp"
#endif

#ifndef CONFIG_PROFILE
    #define CONFIG_PROFILE "undefined"
#endif
#ifndef CONFIG_SERVER_ROOT
    #define CONFIG_SERVER_ROOT "."
#endif
#ifndef CONFIG_TITLE
    #define CONFIG_TITLE "Webshow"
#endif
#ifndef CONFIG_VERSION
    #define CONFIG_VERSION "1.0.0"
#endif
#ifndef CONFIG_WEB_GROUP
    #define CONFIG_WEB_GROUP "nogroup"
#endif
#ifndef CONFIG_WEB_USER
    #define CONFIG_WEB_USER "nobody"
#endif

/* Prefixes */
#ifndef CONFIG_ROOT_PREFIX
    #define CONFIG_ROOT_PREFIX "/"
#endif
#ifndef CONFIG_BASE_PREFIX
    #define CONFIG_BASE_PREFIX "/usr/local"
#endif
#ifndef CONFIG_DATA_PREFIX
    #define CONFIG_DATA_PREFIX "/"
#endif
#ifndef CONFIG_STATE_PREFIX
    #define CONFIG_STATE_PREFIX "/var"
#endif
#ifndef CONFIG_APP_PREFIX
    #define CONFIG_APP_PREFIX "/usr/local/lib/webapp"
#endif
#ifndef CONFIG_VAPP_PREFIX
    #define CONFIG_VAPP_PREFIX "/usr/local/lib/webapp/1.0.0"
#endif
#ifndef CONFIG_BIN_PREFIX
    #define CONFIG_BIN_PREFIX "/usr/local/bin"
#endif
#ifndef CONFIG_INC_PREFIX
    #define CONFIG_INC_PREFIX "/usr/local/include"
#endif
#ifndef CONFIG_LIB_PREFIX
    #define CONFIG_LIB_PREFIX "/usr/local/lib"
#endif
#ifndef CONFIG_MAN_PREFIX
    #define CONFIG_MAN_PREFIX "/usr/local/share/man"
#endif
#ifndef CONFIG_SBIN_PREFIX
    #define CONFIG_SBIN_PREFIX "/usr/local/sbin"
#endif
#ifndef CONFIG_ETC_PREFIX
    #define CONFIG_ETC_PREFIX "/etc/webapp"
#endif
#ifndef CONFIG_WEB_PREFIX
    #define CONFIG_WEB_PREFIX "/var/www/webapp"
#endif
#ifndef CONFIG_LOG_PREFIX
    #define CONFIG_LOG_PREFIX "/var/log/webapp"
#endif
#ifndef CONFIG_SPOOL_PREFIX
    #define CONFIG_SPOOL_PREFIX "/var/spool/webapp"
#endif
#ifndef CONFIG_CACHE_PREFIX
    #define CONFIG_CACHE_PREFIX "/var/spool/webapp/cache"
#endif


/* Suffixes */
#ifndef CONFIG_SHLIB
    #define CONFIG_SHLIB ".so"
#endif
#ifndef CONFIG_SHOBJ
    #define CONFIG_SHOBJ ".so"
#endif
#ifndef CONFIG_OBJ
    #define CONFIG_OBJ ".o"
#endif

/* Profile */

#ifndef CONFIG_WEBAPP_PRODUCT
    #define CONFIG_WEBAPP_PRODUCT 1
#endif
#ifndef CONFIG_PROFILE
    #define CONFIG_PROFILE "default"
#endif
#ifndef CONFIG_TUNE_SIZE
    #define CONFIG_TUNE_SIZE 1
#endif

/* Components */
#ifndef CONFIG_COM_CGI
  #define CONFIG_COM_CGI 1
#endif

#ifndef CONFIG_COM_CC
  #define CONFIG_COM_CC 1
#endif

#ifndef CONFIG_COM_DIR
  #define CONFIG_COM_DIR 0
#endif
#ifndef CONFIG_COM_EJS
    #define CONFIG_COM_EJS 0
#endif
#ifndef CONFIG_COM_ESP
    #define CONFIG_COM_ESP 1
#endif
#ifndef CONFIG_COM_FAST
    #define CONFIG_COM_FAST 1
#endif

#ifndef CONFIG_COM_MDB
    #define CONFIG_COM_MDB 1
#endif
#ifndef CONFIG_COM_OPENSSL
    #define CONFIG_COM_OPENSSL 1
#endif
#ifndef CONFIG_COM_PCRE
    #define CONFIG_COM_PCRE 1
#endif
#ifndef CONFIG_COM_PHP
    #define CONFIG_COM_PHP 1
#endif
#ifndef CONFIG_COM_PROXY
    #define CONFIG_COM_PROXY 1
#endif
#ifndef CONFIG_COM_SQLITE
    #define CONFIG_COM_SQLITE 0
#endif
#ifndef CONFIG_COM_SSL
    #define CONFIG_COM_SSL 1
#endif


