
#include "me.h"

#if CONFIG_COM_ESP

#include "osdep.h"


/*
    mdb.h -- Memory Database (MDB).
 */

#ifndef _MDB_H_
#define _MDB_H_ 1

/********************************* Includes ***********************************/

#include "libhttp.h"


#if CONFIG_COM_MDB

#ifdef __cplusplus
extern "C" {
#endif

/****************************** Forward Declarations **************************/

#if !DOXYGEN
#endif

/********************************** Tunables **********************************/

#define MDB_INCR    8                   /**< Default memory allocation increment for MDB */

/*
    Per column structure
 */
typedef struct MdbCol {
    char            *name;              /* Column name */
    int             type;               /* Column type */
    int             flags;              /* Column flags */
    int             cid;                /* Column index in MdbSchema.cols */
    int64           lastValue;          /* Last value if auto-inc */
} MdbCol;

/*
    Table schema
 */
typedef struct MdbSchema {
    int             ncols;              /* Number of columns in table */
    int             capacity;           /* Capacity of cols */
    MdbCol          cols[ARRAY_FLEX];   /* Array of columns */
} MdbSchema;

/*
    Per row structure
 */
typedef struct MdbRow {
    struct MdbTable *table;             /* Reference to MdbTable */
    int             rid;                /* Table index in MdbTable.row */
    int             nfields;            /* Number of fields in fields */
    cchar           *fields[ARRAY_FLEX];/* All data stored as strings */
} MdbRow;

/*
    Per table structure
 */
typedef struct MdbTable {
    char            *name;              /* Table name */
    MdbSchema       *schema;            /* Table columns schema */
    MprHash         *index;             /* Table index */
    MdbCol          *keyCol;            /* Reference to the key column (unmanaged) */
    MdbCol          *indexCol;          /* Reference to the index column (unmanaged) */
    MprList         *rows;              /* Table row */
} MdbTable;

/*
    Mdb flags
 */
#define MDB_LOADING     0x1

/*
    Per database structure
 */
typedef struct Mdb {
    Edi             edi;                /**< EDI database interface structure */
    MprList         *tables;            /**< List of tables */

    /*
        When loading from file only (do not mark)
     */
    MdbTable        *loadTable;         /* Current table */
    MdbCol          *loadCol;           /* Current column */
    MdbRow          *loadRow;           /* Current row */
    MprList         *loadStack;         /* State stack */
    MprHash         *validations;       /**< Validations */
    int             loadCid;            /* Current column index to load */
    int             loadState;          /* Current state */
    int             loadNcols;          /* Expected number of cols */
    int             lineNumber;         /* Current line number in path */
} Mdb;


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* CONFIG_COM_MDB */
#endif /* _MDB_H_ */



#endif /* CONFIG_COM_ESP */



