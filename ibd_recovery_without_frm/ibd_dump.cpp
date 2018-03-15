/*******************************************************************************
 *      File Name: ibd_dump.cpp                                             
 *         Author: wisehead. (c) 2018                             
 *   Created Time: 2018/03/11-07:21                                    
 *	Modified Time: 2018/03/11-07:21                                    
 *******************************************************************************/
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <pthread.h>
#include <assert.h>
#include <map>
#include <string>
#include <iostream>

using namespace std;

#ifndef TRUE

#define TRUE    1
#define FALSE   0

#endif

#define byte            unsigned char
#define ERR_EXIT(msg) do {perror(msg);exit(EXIT_FAILURE);} while(0)

#define PAGE_SIZE_16K  (16*1024L)
#define PAGE_SIZE  (16*1024)
#define EXTENT_SIZE (64UL)
#define MAX_FILE_LEN 100

/** The byte offsets on a file page for various variables @{ */
#define FIL_PAGE_SPACE_OR_CHKSUM 0  /*!< in < MySQL-4.0.14 space id the
                    page belongs to (== 0) but in later
                    versions the 'new' checksum of the
                    page */
#define FIL_PAGE_OFFSET     4   /*!< page offset inside space */
#define FIL_PAGE_PREV       8   /*!< if there is a 'natural'
                    predecessor of the page, its
                    offset.  Otherwise FIL_NULL.
                    This field is not set on BLOB
                    pages, which are stored as a
                    singly-linked list.  See also
                    FIL_PAGE_NEXT. */
#define FIL_PAGE_NEXT       12  /*!< if there is a 'natural' successor
                    of the page, its offset.
                    Otherwise FIL_NULL.
                    B-tree index pages
                    (FIL_PAGE_TYPE contains FIL_PAGE_INDEX)
                    on the same PAGE_LEVEL are maintained
                    as a doubly linked list via
                    FIL_PAGE_PREV and FIL_PAGE_NEXT
                    in the collation order of the
                    smallest user record on each page. */
#define FIL_PAGE_LSN        16  /*!< lsn of the end of the newest
                    modification log record to the page */
#define FIL_PAGE_TYPE       24  /*!< file page type: FIL_PAGE_INDEX,...,
                    2 bytes.

                    The contents of this field can only
                    be trusted in the following case:
                    if the page is an uncompressed
                    B-tree index page, then it is
                    guaranteed that the value is
                    FIL_PAGE_INDEX.
                    The opposite does not hold.

                    In tablespaces created by
                    MySQL/InnoDB 5.1.7 or later, the
                    contents of this field is valid
                    for all uncompressed pages. */
#define FIL_PAGE_FILE_FLUSH_LSN 26  /*!< this is only defined for the
                    first page in a system tablespace
                    data file (ibdata*, not *.ibd):
                    the file has been flushed to disk
                    at least up to this lsn */
#define FIL_PAGE_ARCH_LOG_NO_OR_SPACE_ID  34 /*!< starting from 4.1.x this
                    contains the space id of the page */
#define FIL_PAGE_SPACE_ID  FIL_PAGE_ARCH_LOG_NO_OR_SPACE_ID
#define FIL_PAGE_DATA       38  /*!< start of the data on the page */


#define PAGE_HEADER  FIL_PAGE_DATA /* index page header starts at this
                offset */

#define FIL_ADDR_SIZE   6   /* address size is 6 bytes */
/* The physical size of a list base node in bytes */
#define FLST_BASE_NODE_SIZE (4 + 2 * FIL_ADDR_SIZE)
/* File space header size */
#define FSP_HEADER_SIZE     (32 + 5 * FLST_BASE_NODE_SIZE)

/** Offset of the space header within a file page */
#define FSP_HEADER_OFFSET   FIL_PAGE_DATA

/** Offset of the descriptor array on a descriptor page */
#define XDES_ARR_OFFSET     (FSP_HEADER_OFFSET + FSP_HEADER_SIZE)
#define XDES_SIZE 40
#define XDES_STATE 20
#define XDES_FSEG       4   /* extent belongs to a segment */
#define XDES_BITMAP    24 
                    /* Descriptor bitmap of the pages
                    in the extent */
#define XDES_BITS_PER_PAGE  2   /* How many bits are there per page */
#define XDES_FREE_BIT       0   /* Index of the bit which tells if
                    the page is free */
#define XDES_CLEAN_BIT      1   /* NOTE: currently not used!
                    Index of the bit which tells if
                    there are old versions of tuples
                    on the page */
/*-----------------------------*/
#define PAGE_N_DIR_SLOTS 0  /* number of slots in page directory */
#define PAGE_HEAP_TOP    2  /* pointer to record heap top */
#define PAGE_N_HEAP  4  /* number of records in the heap,
                bit 15=flag: new-style compact page format */
#define PAGE_FREE    6  /* pointer to start of page free record list */
#define PAGE_GARBAGE     8  /* number of bytes in deleted records */
#define PAGE_LAST_INSERT 10 /* pointer to the last inserted record, or
                NULL if this info has been reset by a delete,
                for example */
#define PAGE_DIRECTION   12 /* last insert direction: PAGE_LEFT, ... */
#define PAGE_N_DIRECTION 14 /* number of consecutive inserts to the same
                direction */
#define PAGE_N_RECS  16 /* number of user records on the page */
#define PAGE_MAX_TRX_ID  18 /* highest id of a trx which may have modified
                a record on the page; trx_id_t; defined only
                in secondary indexes and in the insert buffer
                tree */
#define PAGE_HEADER_PRIV_END 26 /* end of private data structure of the page
                header which are set in a page create */
/*----*/
#define PAGE_LEVEL   26 /* level of the node in an index tree; the
                leaf level is the level 0.  This field should
                not be written to after page creation. */
#define PAGE_INDEX_ID    28 /* index id where the page belongs.
                This field should not be written to after
                page creation. */
#define PAGE_BTR_SEG_LEAF 36    /* file segment header for the leaf pages in
                a B-tree: defined only on the root page of a
                B-tree, but not in the root of an ibuf tree */
#define PAGE_BTR_IBUF_FREE_LIST PAGE_BTR_SEG_LEAF
#define PAGE_BTR_IBUF_FREE_LIST_NODE PAGE_BTR_SEG_LEAF
                /* in the place of PAGE_BTR_SEG_LEAF and _TOP
                there is a free list base node if the page is
                the root page of an ibuf tree, and at the same
                place is the free list node if the page is in
                a free list */
#define PAGE_BTR_SEG_TOP (36 + FSEG_HEADER_SIZE)
                /* file segment header for the non-leaf pages
                in a B-tree: defined only on the root page of
                a B-tree, but not in the root of an ibuf
                tree */
/*----*/
#define PAGE_DATA   (PAGE_HEADER + 36 + 2 * FSEG_HEADER_SIZE)
                /* start of data on the page */

/*          SPACE HEADER
            ============

File space header data structure: this data structure is contained in the
first page of a space. The space for this header is reserved in every extent
descriptor page, but used only in the first. */

/*-------------------------------------*/
#define FSP_SPACE_ID        0   /* space id */
#define FSP_NOT_USED        4   /* this field contained a value up to
                    which we know that the modifications
                    in the database have been flushed to
                    the file space; not used now */
#define FSP_SIZE        8   /* Current size of the space in
                    pages */
#define FSP_FREE_LIMIT      12  /* Minimum page number for which the
                    free list has not been initialized:
                    the pages >= this limit are, by
                    definition, free; note that in a
                    single-table tablespace where size
                    < 64 pages, this number is 64, i.e.,
                    we have initialized the space
                    about the first extent, but have not
                    physically allocted those pages to the
                    file */
#define FSP_FREE        24  /* list of free extents */

/*-------------------------------------*/
/* We define the field offsets of a base node for the list */
#define FLST_LEN    0   /* 32-bit list length field */
#define FLST_FIRST  4   /* 6-byte address of the first element
                of the list; undefined if empty list */
#define FLST_LAST   (4 + FIL_ADDR_SIZE) /* 6-byte address of the
                last element of the list; undefined
                if empty list */


#define X_TOTAL_PAGE_HEADER_SIZE 120
#define X_INFIMUM_REC 99//(0x63) 
#define X_1st_REC_OFFSET 97//(0x61) 
#define X_NEXT_REC_LEN  6 
#define X_REC_HEADER_OFFSET 3
#define X_REC_ROW_ID 8
#define X_REC_TRANCATION_ID 12
#define X_REC_ROLLUP_PTR 18
#define X_REC_START 25 

//uint64_t g_page_size = 16*1024;
uint64_t g_file_size = 0;

typedef struct {
        uint64_t page_no;
        uint64_t page_type;
        unsigned char buf[PAGE_SIZE_16K];
} Page;

/** File page types (values of FIL_PAGE_TYPE) @{ */
enum file_page_type {
    FIL_PAGE_TYPE_ALLOCATED,//0   /*!< Freshly allocated page */
    FIL_PAGE_INDEX, //1  /*!< B-tree node */
    FIL_PAGE_UNDO_LOG,//   2   /*!< Undo log page */
    FIL_PAGE_INODE,//      3   /*!< Index node */
    FIL_PAGE_IBUF_FREE_LIST,// 4   /*!< Insert buffer free list */
    FIL_PAGE_IBUF_BITMAP,//    5   /*!< Insert buffer bitmap */
    FIL_PAGE_TYPE_SYS,//   6   /*!< System page */
    FIL_PAGE_TYPE_TRX_SYS,//   7   /*!< Transaction system data */
    FIL_PAGE_TYPE_FSP_HDR,//   8   /*!< File space header */
    FIL_PAGE_TYPE_XDES,//  9   /*!< Extent descriptor page */
    FIL_PAGE_TYPE_BLOB,//  10  /*!< Uncompressed BLOB page */
    FIL_PAGE_TYPE_ZBLOB,// 11  /*!< First compressed BLOB page */
    FIL_PAGE_TYPE_ZBLOB2,//    12  /*!< Subsequent compressed BLOB page */
    FIL_PAGE_TYPE_LAST//13
};

char page_type_name[FIL_PAGE_TYPE_LAST][40] = {
    "Freshly Allocated Page",//0
    "B-tree Node",//1
    "Undo Log Page",//2
    "File Segment inode",//3
    "Insert Buffer Free List",//4
    "Insert Buffer Bitmap",//5
    "System Page",//6
    "Transaction system Page",//7
    "File Space Header",//8
    "Extent descriptor page",//9
    "Uncompressed BLOB Page",//10
    "1st compressed BLOB Page",//11
    "Subsequent compressed BLOB Page"//12
};

/*************************************************************//**
Calculates fast the remainder of n/m when m is a power of two.
@param n    in: numerator
@param m    in: denominator, must be a power of two
@return     the remainder of n/m */
#define ut_2pow_remainder(n, m) ((n) & ((m) - 1))
/*************************************************************//**
Calculates the biggest multiple of m that is not bigger than n
when m is a power of two.  In other words, rounds n down to m * k.
@param n    in: number to round down
@param m    in: alignment, must be a power of two
@return     n rounded down to the biggest possible integer multiple of m */
#define ut_2pow_round(n, m) ((n) & ~((m) - 1))
    
/********************************************************//**
The following function is used to fetch data from one byte.
@return uint64_t integer, >= 0, < 256 */
uint64_t
mach_read_from_1(
/*=============*/
    const byte* b)  /*!< in: pointer to byte */ 
{
    return((uint64_t)(b[0]));
}

/********************************************************//**
The following function is used to fetch data from 2 consecutive
bytes. The most significant byte is at the lowest address.
@return uint64_t integer */
uint64_t
mach_read_from_2(
/*=============*/
    const byte* b)  /*!< in: pointer to 2 bytes */
{
    return(((uint64_t)(b[0]) << 8) | (uint64_t)(b[1]));
}

/********************************************************//**
The following function is used to fetch data from 4 consecutive
bytes. The most significant byte is at the lowest address.
@return uint64_t integer */
uint64_t
mach_read_from_4(
/*=============*/
    const byte* b)  /*!< in: pointer to four bytes */
{
    return( ((uint64_t)(b[0]) << 24)
        | ((uint64_t)(b[1]) << 16)
        | ((uint64_t)(b[2]) << 8)
        | (uint64_t)(b[3])
        );      
}

/********************************************************//**
The following function is used to fetch data from 8 consecutive
bytes. The most significant byte is at the lowest address.
@return 64-bit integer */
uint64_t
mach_read_from_8(
/*=============*/
    const byte* b)  /*!< in: pointer to 8 bytes */
{
    uint64_t ull;

    ull = ((uint64_t) mach_read_from_4(b)) << 32;
    ull |= (uint64_t) mach_read_from_4(b + 4);

    return(ull);
}

/*****************************************************************//**
Gets the nth bit of a uint64_t.
@return TRUE if nth bit is 1; 0th bit is defined to be the least significant */
int
ut_bit_get_nth(
/*===========*/
    uint64_t   a,  /*!< in: uint64_t */
    uint64_t   n)  /*!< in: nth bit requested */
{
    return(1 & (a >> n)); 
}

/**************************************************************//**
Gets the index id field of a page. 
@return index id */
uint64_t
btr_page_get_index_id(
/*==================*/
    const byte*   page)   /*!< in: index page */ 
{
    return(mach_read_from_8(page + PAGE_HEADER + PAGE_INDEX_ID));
}

/*************************************************************//**
Reads the given header field. */
uint64_t
fil_header_get_field(
/*==================*/
    const byte*   page,   /*!< in: page */ 
    uint64_t      field,  /*!< in: PAGE_LEVEL, ... */
    int           bytes)  /*!< in: num of bytes ... */
{
    if (bytes == 2) {
        return(mach_read_from_2(page + field));
    }

    if (bytes == 4) {
        return(mach_read_from_4(page + field));
    }
    return 0;
}

/*************************************************************//**
Reads the given header field. */
uint64_t
page_header_get_field(
/*==================*/
    const byte*   page,   /*!< in: page */ 
    uint64_t      field)  /*!< in: PAGE_LEVEL, ... */
{
    return(mach_read_from_2(page + PAGE_HEADER + field));
}

int read_page_gen(int fd, Page* page/*out*/)
{
    uint64_t page_no = page->page_no; 
    int read_ret = pread(fd, page->buf, PAGE_SIZE, page_no * PAGE_SIZE);
    if( read_ret != PAGE_SIZE )
        ERR_EXIT("read Page failed");
    return 0;
}

/********************************************************************//**
Calculates the page where the descriptor of a page resides.
@return descriptor page offset */
uint64_t
xdes_calc_descriptor_page(
/*======================*/
    uint64_t   offset)     /*!< in: page offset */
{
    return(ut_2pow_round(offset, PAGE_SIZE));
}

/********************************************************************//**
Calculates the descriptor index within a descriptor page.
@return descriptor index */
uint64_t
xdes_calc_descriptor_index(
/*=======================*/
    uint64_t   offset)     /*!< in: page offset */
{
    return(ut_2pow_remainder(offset, PAGE_SIZE)
           / EXTENT_SIZE);
}

/**********************************************************************//**
Gets the state of an xdes.
@return state */
uint64_t
xdes_get_state(
/*===========*/
    const byte*   descr)  /*!< in: descriptor */
{
    uint64_t   state;  
    state = mach_read_from_4(descr + XDES_STATE);
    return(state);
}


/**********************************************************************//**
Gets a descriptor bit of a page. 
@return TRUE if free */ 
int
xdes_get_bit(
/*=========*/
    const byte*   descr,  /*!< in: descriptor */
    uint64_t       bit,    /*!< in: XDES_FREE_BIT or XDES_CLEAN_BIT */
    uint64_t       offset) /*!< in: page offset within extent: 
                0 ... FSP_EXTENT_SIZE - 1 */
{
    uint64_t   index = bit + XDES_BITS_PER_PAGE * offset; 

    uint64_t   bit_index = index % 8;
    uint64_t   byte_index = index / 8;

    return(ut_bit_get_nth(
            mach_read_from_1(descr + XDES_BITMAP + byte_index),
            bit_index));
}


enum Columns
{
    mid = 0,
	musername,
	mrealname,      
	mimgurl,       
	mgroupid,        
	mpower,        
	mcreate_time,    
	mcreate_user,   
	mdeleted,   
	mextinfo,        
	mextcount,       
	mlevel,          
	mmclass,          
	munionid,        
	munionlevel,     
	muniontime,      
	msum_profit,     
	mperiod_profit,  
	midentity,       
	mcourse,         
	mgrade_level,    
	midcard,         
	mmobile,         
	mpassword,       
	morg_pid,        
	morg_cid,        
	morg_aid,        
	morg_name,       
	mp_card1,        
	mp_card2,        
	mp_card3,        
	mactivate,       
	mp_card4,        
	muserlevel,      
	mpoint,          
	maccount_id,     
	maccount_status, 
	minvate,         
	mrealstatus,     
	mactivate_time,  
	mwealthscore,    
	mgrowscore,     
	mmonthgrowscore, 
	memail          
}columns;
/*
+----------------+--------------+------+-----+---------+----------------+
| Field          | Type         | Null | Key | Default | Extra          |
+----------------+--------------+------+-----+---------+----------------+
| id             | bigint(20)   | NO   | PRI | NULL    | auto_increment |
| username       | varchar(255) | NO   | UNI | NULL    |                |
| realname       | varchar(255) | NO   | MUL | NULL    |                |
| imgurl         | varchar(255) | NO   |     | NULL    |                |
| groupid        | int(11)      | YES  |     | 0       |                |
| power          | varchar(255) | NO   |     | NULL    |                |
| create_time    | bigint(20)   | NO   |     | NULL    |                |
| create_user    | varchar(255) | NO   |     | NULL    |                |
| deleted        | tinyint(4)   | NO   |     | NULL    |                |
| extinfo        | text         | NO   |     | NULL    |                |
| extcount       | text         | NO   |     | NULL    |                |
| level          | int(11)      | NO   |     | 1       |                |
| class          | int(11)      | NO   | MUL | 1       |                |
| unionid        | int(11)      | NO   | MUL | NULL    |                |
| unionlevel     | int(11)      | NO   |     | NULL    |                |
| uniontime      | int(11)      | NO   |     | NULL    |                |
| sum_profit     | int(11)      | NO   |     | 0       |                |
| period_profit  | int(11)      | NO   |     | 0       |                |
| identity       | int(11)      | YES  | MUL | 0       |                |
| course         | int(11)      | YES  | MUL | 0       |                |
| grade_level    | int(11)      | YES  | MUL | 0       |                |
| idcard         | varchar(25)  | NO   | MUL | NULL    |                |
| mobile         | varchar(20)  | YES  |     | NULL    |                |
| password       | varchar(50)  | YES  |     | NULL    |                |
| org_pid        | int(11)      | YES  |     | 0       |                |
| org_cid        | int(11)      | YES  |     | 0       |                |
| org_aid        | int(11)      | YES  |     | 0       |                |
| org_name       | varchar(100) | YES  |     | NULL    |                |
| p_card1        | varchar(255) | YES  |     | NULL    |                |
| p_card2        | varchar(255) | YES  |     | NULL    |                |
| p_card3        | varchar(255) | YES  |     | NULL    |                |
| activate       | int(11)      | YES  |     | 0       |                |
| p_card4        | varchar(255) | YES  |     | NULL    |                |
| userlevel      | int(11)      | NO   |     | 0       |                |
| point          | int(11)      | NO   | MUL | 0       |                |
| account_id     | varchar(255) | YES  |     | NULL    |                |
| account_status | tinyint(4)   | NO   |     | 0       |                |
| invate         | int(11)      | NO   |     | 0       |                |
| realstatus     | tinyint(4)   | YES  |     | 0       |                |
| activate_time  | int(11)      | YES  |     | 0       |                |
| wealthscore    | int(10)      | YES  |     | 0       |                |
| growscore      | int(10)      | YES  |     | 0       |                |
| monthgrowscore | int(10)      | YES  |     | 0       |                |
| email          | varchar(126) | NO   | MUL |         |                |
+----------------+--------------+------+-----+---------+----------------+
*/

unsigned char full_field_map[45] = {0};

int null_bit_array[24] = {mgroupid,midentity,mcourse,mgrade_level,
                          mmobile,mpassword,morg_pid,morg_cid,
                          morg_aid,morg_name,mp_card1,mp_card2,
                          mp_card3,mactivate,mp_card4,maccount_id,
                          mrealstatus,mactivate_time,mwealthscore,mgrowscore,
                          mmonthgrowscore,0,0,0} ;
std::string field_names[45] =
{
    "id",
	"username",
	"realname",      
	"imgurl",       
	"groupid",        
	"power",        
	"create_time",    
	"create_user",   
	"deleted",   
	"extinfo",        
	"extcount",       
	"level",          
	"mclass",          
	"unionid",        
	"unionlevel",     
	"uniontime",      
	"sum_profit",     
	"period_profit",  
	"identity",       
	"course",         
	"grade_level",    
	"idcard",         
	"mobile",         
	"password",       
	"org_pid",        
	"org_cid",        
	"org_aid",        
	"org_name",       
	"p_card1",        
	"p_card2",        
	"p_card3",        
	"activate",       
	"p_card4",        
	"userlevel",      
	"point",          
	"account_id",     
	"account_status", 
	"invate",         
	"realstatus",     
	"activate_time",  
	"wealthscore",    
	"growscore",     
	"monthgrowscore", 
	"email"          
};

int main(int argc, char** argv)
{
    char* ptr_file_name = NULL;
    if (argc != 2){
        printf("Usage: ibdscan ibd_file.\n");
        exit(-1);
    }

    ptr_file_name = argv[argc-1];

    if (strlen(ptr_file_name) > MAX_FILE_LEN) {
        printf("Usage: the file name is too long, the max file length is %d.\n", MAX_FILE_LEN);
        exit(-1);
    }

    char filename[MAX_FILE_LEN];
    memset(filename, 0, sizeof(char)*MAX_FILE_LEN);
    strncpy(filename, ptr_file_name, strlen(ptr_file_name));

    printf("--------------------------------------------------\n"); 
    printf("|                 ibdscan                        |\n"); 
    printf("--------------------------------------------------\n"); 
    printf("scanning %s ...\n", filename); 
    printf("--------------------------------------------------\n"); 

    int ret = -1;
    int fd;
    uint64_t page_count = 0;
    Page page_obj;
    Page *page = &page_obj;
    uint64_t page_type_count_array[FIL_PAGE_TYPE_LAST] = {0};

    unsigned int page_offset = 0;
    unsigned int page_prev = 0;
    unsigned int page_next = 0;
    unsigned short page_type = 0;
    unsigned int tab_space_id = 0;

    unsigned short page_heap_top = 0; 
    unsigned short page_n_heap = 0; 
    unsigned short page_free = 0; 
    unsigned short page_garbage = 0; 
    unsigned short page_n_recs = 0; 
    unsigned short page_level = 0;
    uint64_t page_index_id = 0;
    uint64_t descr_page_no = 0;

    uint64_t free_space = 0;
    uint64_t freed_page_count = 0;

    uint64_t size = 0; 
    uint64_t n_free_list_ext = 0; 
    uint64_t free_limit = 0; 
    uint64_t n_free_up = 0; 
    uint64_t n_free = 0; 

    if(!(fd = open(filename, O_RDONLY))) {
        ERR_EXIT("open failed!\n");
    } 

    if((g_file_size = lseek(fd,0,SEEK_END))<0)
    {
       ERR_EXIT("lseek file failure!");
    }

    uint64_t max_page_no = g_file_size/PAGE_SIZE;

    while (page_count < max_page_no) {
        page->page_no = page_count;

        if(!read_page_gen(fd, page)) {
            page_count++;
        } else {
            printf("readpage failed!\n");
            exit(-1);
        }
        
        // the files are cut into 16K pages.
        // not a whole ibd file.
        if (page->page_no == 0)
        {
            uint64_t rec_len = 0;
            rec_len = mach_read_from_2(page->buf + X_1st_REC_OFFSET);
            printf("xxx rec_len is %d\n", rec_len);

            byte* cur_ptr = page->buf + X_TOTAL_PAGE_HEADER_SIZE;
            byte* last_cur_ptr = page->buf + X_INFIMUM_REC;
            byte* rec_start_ptr = NULL;
            int rec_count = 0;

            //while (last_cur_ptr + rec_len != page->buf + 0x70 && rec_count < 2000)
            while (rec_count < 2000)
            {
                bzero(full_field_map, 45);
                rec_count++;
                printf("xxx processing rec:%d \n", rec_count);
                printf("xxx last_cur_ptr is:%lx \n", last_cur_ptr);
                printf("xxx rec_len is %d\n", rec_len);
                cur_ptr = last_cur_ptr + rec_len;
                printf("xxx cur_ptr is:%lx \n", cur_ptr);
                printf("xxx page->buf + 0x4000 is:%lx \n", page->buf + 0x4000);

                // to process the file hole of a page.
                // the hole might be reused.
                if ((uint64_t)cur_ptr > (uint64_t)page->buf + 0x4000)
                {
                    cur_ptr = cur_ptr - 0x10000;
                    printf("xxx new cur_ptr is:%lx \n", cur_ptr);
                }
                if (cur_ptr == page->buf + 0x70)
                {
                    printf("xxx the end, exiting. \n");
                    break;
                }
                rec_start_ptr = cur_ptr;
                printf("xxx rec_start_ptr is:%lx \n", rec_start_ptr);

                last_cur_ptr = cur_ptr;
                
                // record_header 5
                // null_bits 3
                // we'll read one more byte for easy coding.. but will not use the first byte.
                cur_ptr = cur_ptr -5 - 4;
                unsigned int null_bits =  mach_read_from_4(cur_ptr);
                printf("xxx null_bits is:%lx\n", null_bits);
                unsigned int null_mask = 0x00800000;
                int null_bits_count = 0;
                for (int i=0; i<24; i++)
                {
                    null_mask = 1<<i;
                    //printf("xxx null_mask is:%lx\n", null_mask);
                    if((null_bits & null_mask) != 0)
                    {
                        int field_no =  null_bit_array[i];
                        printf("xxx i:%d,   field_no is:%d,   field_name is:%s.\n", i, field_no, field_names[field_no].c_str());
                        full_field_map[field_no] = 1;
                        null_bits_count++;
                    }
                    null_mask = null_mask>>1;
                }
                rec_len =  mach_read_from_2(cur_ptr + 7);
                printf("xxx rec_len is %d\n", rec_len);
                
/*
0000c070  73 75 70 72 65 6d 75 6d  00 0f 04 04 04 04 04 20  |supremum....... |
                                   |        |
                                   email    p_card3
                                       |        p_card2
                                       account_id | 
                                         |        p_card1
                                         p_card4     |
                                                     org_name
                                                         |
                                                         password（32）
                                                         
0000c080  0b 12 00 bd 80 08 04 00  06 0f 00 00 00 00 00 10  |................|
          |              |            
          mobile         create_user(liuhao21) 
          (18612708964) 
             |              |
             idcard         power(1031)
             (610104198605128316)
                |              |
                extcount(0)     imgurl(0)
                   |               |
                   extinfo(duo)    realname(刘豪)
                                      |
                                      username(锤锤在燃烧)
*/
                unsigned int len_email = 0, len_account_id = 0, len_p_card4 = 0, len_p_card3 = 0, len_p_card2 = 0, len_p_card1 = 0, len_org_name = 0, len_password = 0;
                unsigned int len_mobile = 0, len_idcard = 0, len_extcount = 0, len_extinfo = 0, len_create_user = 0, len_power = 0, len_imgurl = 0, len_realname = 0, len_username = 0;


                //len_email = mach_read_from_1(cur_ptr++);
                //-------------------------------------
                len_username = mach_read_from_1(cur_ptr--);
                printf("xxx len_username is %lu\n", len_username);

                len_realname = mach_read_from_1(cur_ptr--);
                printf("xxx len_realname is %lu\n", len_realname);

                len_imgurl = mach_read_from_1(cur_ptr--);
                printf("xxx len_imgurl is %lu\n", len_imgurl);

                len_power = mach_read_from_1(cur_ptr--);
                printf("xxx len_power is %lu\n", len_power);

                len_create_user = mach_read_from_1(cur_ptr--);
                printf("xxx len_create_user is %lu\n", len_create_user);

                len_extinfo = mach_read_from_1(cur_ptr--);
                if ((len_extinfo & 0x80) != 0)
                {
                    len_extinfo = mach_read_from_1(cur_ptr--);
                }
                printf("xxx len_extinfo is %lu\n", len_extinfo);

                len_extcount = mach_read_from_1(cur_ptr--);
                printf("xxx len_extcount is %lu\n", len_extcount);

                len_idcard = mach_read_from_1(cur_ptr--);
                printf("xxx len_idcard is %lu\n", len_idcard);

                if (!full_field_map[mmobile])
                {
                    len_mobile = mach_read_from_1(cur_ptr--);
                    printf("xxx len_mobile is %lu\n", len_mobile);
                }
                if (!full_field_map[mpassword])
                {
                    len_password = mach_read_from_1(cur_ptr--);
                    printf("xxx len_password is %lu\n", len_password);
                }
                if (!full_field_map[morg_name])
                {
                    len_org_name = mach_read_from_1(cur_ptr--);
                    printf("xxx len_org_name is %lu\n", len_org_name);
                }
                if (!full_field_map[mp_card1])
                {
                    len_p_card1 = mach_read_from_1(cur_ptr--);
                    printf("xxx len_p_card1 is %lu\n", len_p_card1);
                }

                if (!full_field_map[mp_card2])
                {
                    len_p_card2 = mach_read_from_1(cur_ptr--);
                    printf("xxx len_p_card2 is %lu\n", len_p_card2);
                }
                if (!full_field_map[mp_card3])
                {
                    len_p_card3 = mach_read_from_1(cur_ptr--);
                    printf("xxx len_p_card3 is %lu\n", len_p_card3);
                }
                if (!full_field_map[mp_card4])
                {
                    len_p_card4 = mach_read_from_1(cur_ptr--);
                    printf("xxx len_p_card4 is %lu\n", len_p_card4);
                }

                if (!full_field_map[maccount_id])
                {
                    len_account_id = mach_read_from_1(cur_ptr--);
                    printf("xxx len_account_id is %lu\n", len_account_id);
                }

                // start processing trx_id
                cur_ptr = rec_start_ptr;

                uint64_t row_id = 0;
                uint64_t mask = 0x7FFFFFFF;
                uint64_t mask_1_byte = 0x7F;
                uint64_t mask_8_byte = 0x7FFFFFFFFFFFFFFF;
                row_id = mach_read_from_8(cur_ptr);
                row_id = (row_id & mask_8_byte);
                printf("xxx row_id is %lu\n", row_id);
                //skip the 8 bytes row id
                cur_ptr += 8;

                // skip transaction_id and rollup_ptr
                cur_ptr += 13;

                //`username` varchar
                std::string username;
                for (int i=0; i<len_username; i++)
                {
                    char c = mach_read_from_1(cur_ptr + i);
                    username.push_back(c);
                }
                cout<<"xxx username is:"<<username<<endl;
                cur_ptr += len_username;

                //`realname` varchar
                std::string realname;
                for (int i=0; i<len_realname; i++)
                {
                    char c = mach_read_from_1(cur_ptr + i);
                    realname.push_back(c);
                }
                cout<<"xxx realname is:"<<realname<<endl;
                cur_ptr += len_realname;

                //`imgurl` varchar
                std::string imgurl;
                for (int i=0; i<len_imgurl; i++)
                {
                    char c = mach_read_from_1(cur_ptr + i);
                    imgurl.push_back(c);
                }
                cout<<"xxx imgurl is:"<<imgurl<<endl;
                cur_ptr += len_imgurl;

                //groupid int, can be NULL
                uint64_t groupid = 0;
                if (!full_field_map[mgroupid])
                {
                    groupid = mach_read_from_4(cur_ptr);
                    groupid = (groupid & mask);
                    printf("xxx groupid is %lu\n", groupid);
                    cur_ptr += 4;
                }

                //`power` varchar
                std::string power;
                for (int i=0; i<len_power; i++)
                {
                    char c = mach_read_from_1(cur_ptr + i);
                    power.push_back(c);
                }
                cout<<"xxx power is:"<<power<<endl;
                cur_ptr += len_power;

                //create_time bigint
                uint64_t create_time = 0;
                create_time = mach_read_from_8(cur_ptr);
                create_time = (create_time & mask_8_byte);
                printf("xxx create_time is %lu\n", create_time);
                cur_ptr += 8;

                //`create_user` varchar
                std::string create_user;
                for (int i=0; i<len_create_user; i++)
                {
                    char c = mach_read_from_1(cur_ptr + i);
                    create_user.push_back(c);
                }
                cout<<"xxx create_user is:"<<create_user<<endl;
                cur_ptr += len_create_user;

                //deleted tinyint
                uint64_t deleted = 0;
                deleted = mach_read_from_1(cur_ptr);
                deleted = (deleted & mask_1_byte);
                printf("xxx deleted is %lu\n", deleted);
                cur_ptr += 1;

                //`extinfo`  text
                std::string extinfo;
                for (int i=0; i<len_extinfo; i++)
                {
                    char c = mach_read_from_1(cur_ptr + i);
                    extinfo.push_back(c);
                }
                cout<<"xxx extinfo is:"<<extinfo<<endl;
                cur_ptr += len_extinfo;

                //`extcount`  text
                std::string extcount;
                for (int i=0; i<len_extcount; i++)
                {
                    char c = mach_read_from_1(cur_ptr + i);
                    extcount.push_back(c);
                }
                cout<<"xxx extcount is:"<<extcount<<endl;
                cur_ptr += len_extcount;

                //level int
                uint64_t level = 0;
                level = mach_read_from_4(cur_ptr);
                level = (level & mask);
                printf("xxx level is %lu\n", level);
                cur_ptr += 4;

                //class int
                uint64_t mclass = 0;
                mclass = mach_read_from_4(cur_ptr);
                mclass = (mclass & mask);
                printf("xxx class is %lu\n", mclass);
                cur_ptr += 4;

                //unionid int
                uint64_t unionid = 0;
                unionid = mach_read_from_4(cur_ptr);
                unionid = (unionid & mask);
                printf("xxx unionid is %lu\n", unionid);
                cur_ptr += 4;

                //unionlevel int
                uint64_t unionlevel = 0;
                unionlevel = mach_read_from_4(cur_ptr);
                unionlevel = (unionlevel & mask);
                printf("xxx unionlevel is %lu\n", unionlevel);
                cur_ptr += 4;

                //uniontime int
                //fix me: the time columns need to be double checked.
                uint64_t uniontime = 0;
                uniontime = mach_read_from_4(cur_ptr);
                uniontime = (uniontime & mask);
                printf("xxx uniontime is %lu\n", uniontime);
                cur_ptr += 4;

                //sum_profit int
                uint64_t sum_profit = 0;
                sum_profit = mach_read_from_4(cur_ptr);
                sum_profit = (sum_profit & mask);
                printf("xxx sum_profit is %lu\n", sum_profit);
                cur_ptr += 4;

                //period_profit int
                uint64_t period_profit = 0;
                period_profit = mach_read_from_4(cur_ptr);
                period_profit = (period_profit & mask);
                printf("xxx period_profit is %lu\n", period_profit);
                cur_ptr += 4;

                //identity int, can be NULL
                uint64_t identity = 0;
                if (!full_field_map[midentity])
                {
                    identity = mach_read_from_4(cur_ptr);
                    identity = (identity & mask);
                    printf("xxx identity is %lu\n", identity);
                    cur_ptr += 4;
                }

                //course int, can be NULL.
                uint64_t course = 0;
                if (!full_field_map[mcourse])
                {
                    course = mach_read_from_4(cur_ptr);
                    course = (course & mask);
                    printf("xxx course is %lu\n", course);
                    cur_ptr += 4;
                }

                //grade_level int, can be NULL.
                uint64_t grade_level = 0;
                if (!full_field_map[mgrade_level])
                {
                    grade_level = mach_read_from_4(cur_ptr);
                    grade_level = (grade_level & mask);
                    printf("xxx grade_level is %lu\n", grade_level);
                    cur_ptr += 4;
                }

                //`idcard` varchar 
                std::string idcard;
                for (int i=0; i<len_idcard; i++)
                {
                    char c = mach_read_from_1(cur_ptr + i);
                    idcard.push_back(c);
                }
                cout<<"xxx idcard is:"<<idcard<<endl;
                cur_ptr += len_idcard;

                //`mobile` varchar, can be NULL.
                std::string mobile;
                if (!full_field_map[mmobile])
                {
                    for (int i=0; i<len_mobile; i++)
                    {
                        char c = mach_read_from_1(cur_ptr + i);
                        mobile.push_back(c);
                    }
                    cout<<"xxx mobile is:"<<mobile<<endl;
                    cur_ptr += len_mobile;
                }

                //`password` varchar 
                std::string password;
                if (!full_field_map[mpassword])
                {
                    for (int i=0; i<len_password; i++)
                    {
                        char c = mach_read_from_1(cur_ptr + i);
                        password.push_back(c);
                    }
                    cout<<"xxx password is:"<<password<<endl;
                    cur_ptr += len_password;
                }

                //org_pid int
                uint64_t org_pid = 0;
                if (!full_field_map[morg_pid])
                {
                    org_pid = mach_read_from_4(cur_ptr);
                    org_pid = (org_pid & mask);
                    printf("xxx org_pid is %lu\n", org_pid);
                    cur_ptr += 4;
                }

                //org_cid int
                uint64_t org_cid = 0;
                if (!full_field_map[morg_cid])
                {
                    org_cid = mach_read_from_4(cur_ptr);
                    org_cid = (org_cid & mask);
                    printf("xxx org_cid is %lu\n", org_cid);
                    cur_ptr += 4;
                }

                //org_aid int
                uint64_t org_aid = 0;
                if (!full_field_map[morg_aid])
                {
                    org_aid = mach_read_from_4(cur_ptr);
                    org_aid = (org_aid & mask);
                    printf("xxx org_aid is %lu\n", org_aid);
                    cur_ptr += 4;
                }

                //`org_name` varchar 
                std::string org_name;
                if (!full_field_map[morg_name])
                {
                    for (int i=0; i<len_org_name; i++)
                    {
                        char c = mach_read_from_1(cur_ptr + i);
                        org_name.push_back(c);
                    }
                    cout<<"xxx org_name is:"<<org_name<<endl;
                    cur_ptr += len_org_name;
                }

                //`p_card1` varchar 
                std::string p_card1;
                if (!full_field_map[mp_card1])
                {
                    for (int i=0; i<len_p_card1; i++)
                    {
                        char c = mach_read_from_1(cur_ptr + i);
                        p_card1.push_back(c);
                    }
                    cout<<"xxx p_card1 is:"<<p_card1<<endl;
                    cur_ptr += len_p_card1;
                }

                //`p_card2` varchar 
                std::string p_card2;
                if (!full_field_map[mp_card2])
                {
                    for (int i=0; i<len_p_card2; i++)
                    {
                        char c = mach_read_from_1(cur_ptr + i);
                        p_card2.push_back(c);
                    }
                    cout<<"xxx p_card2 is:"<<p_card2<<endl;
                    cur_ptr += len_p_card2;
                }

                //`p_card3` varchar 
                std::string p_card3;
                if (!full_field_map[mp_card3])
                {
                    for (int i=0; i<len_p_card3; i++)
                    {
                        char c = mach_read_from_1(cur_ptr + i);
                        p_card3.push_back(c);
                    }
                    cout<<"xxx p_card3 is:"<<p_card3<<endl;
                    cur_ptr += len_p_card3;
                }

                //activate int
                uint64_t activate = 0;
                if (!full_field_map[mactivate])
                {
                    activate = mach_read_from_4(cur_ptr);
                    activate = (activate & mask);
                    printf("xxx activate is %lu\n", activate);
                    cur_ptr += 4;
                }

                //`p_card4` varchar 
                std::string p_card4;
                if (!full_field_map[mp_card4])
                {
                    for (int i=0; i<len_p_card4; i++)
                    {
                        char c = mach_read_from_1(cur_ptr + i);
                        p_card4.push_back(c);
                    }
                    cout<<"xxx p_card4 is:"<<p_card4<<endl;
                    cur_ptr += len_p_card4;
                }

                //userlevel int
                uint64_t userlevel = 0;
                userlevel = mach_read_from_4(cur_ptr);
                userlevel = (userlevel & mask);
                printf("xxx userlevel is %lu\n", userlevel);
                cur_ptr += 4;

                //point int
                uint64_t point = 0;
                point = mach_read_from_4(cur_ptr);
                point = (point & mask);
                printf("xxx point is %lu\n", point);
                cur_ptr += 4;

                //`account_id` varchar 
                std::string account_id;
                if (!full_field_map[maccount_id])
                {
                    for (int i=0; i<len_account_id; i++)
                    {
                        char c = mach_read_from_1(cur_ptr + i);
                        account_id.push_back(c);
                    }
                    cout<<"xxx account_id is:"<<account_id<<endl;
                    cur_ptr += len_account_id;
                }

                //account_status tinyint
                char account_status = 0;
                account_status = mach_read_from_1(cur_ptr);
                if (account_status & 0x80 != 0)
                {
                     account_status= (~account_status);
                     account_status = (account_status & mask_1_byte);
                     account_status++;
                     account_status = 0 - account_status;
                     //account_status = (int)account_status + 1;
                }
                else
                    account_status = (account_status & mask_1_byte);
                printf("xxx account_status is %d\n", account_status);
                cur_ptr += 1;

                //invate int
                uint64_t invate = 0;
                invate = mach_read_from_4(cur_ptr);
                invate = (invate & mask);
                printf("xxx invate is %lu\n", invate);
                cur_ptr += 4;

                //unknowned int column.
                //xxxint int
                uint64_t xxxint = 0;
                xxxint = mach_read_from_4(cur_ptr);
                xxxint = (xxxint & mask);
                printf("xxx xxxint is %lu\n", xxxint);
                cur_ptr += 4;

                //realstatus tinyint
                char realstatus = 0;
                if (!full_field_map[mrealstatus])
                {
                    realstatus = mach_read_from_1(cur_ptr);
                    if (realstatus & 0x80 != 0)
                    {
                         realstatus= (~realstatus);
                         realstatus = (realstatus & mask_1_byte);
                         realstatus++;
                         realstatus = 0 - realstatus;
                    }
                    else
                        realstatus = (realstatus & mask_1_byte);
                    printf("xxx realstatus is %d\n", realstatus);
                    cur_ptr += 1;
                }

                //activate_time int
                uint64_t activate_time = 0;
                if (!full_field_map[mactivate_time])
                {
                    activate_time = mach_read_from_4(cur_ptr);
                    activate_time = (activate_time & mask);
                    printf("xxx activate_time is %lu\n", activate_time);
                    cur_ptr += 4;
                }

                //unknown tinyint column
                //xxxtinyint tinyint
                char xxxtinyint = 0;
                xxxtinyint = mach_read_from_1(cur_ptr);
                if (xxxtinyint & 0x80 != 0)
                {
                     xxxtinyint= (~xxxtinyint);
                     xxxtinyint = (xxxtinyint & mask_1_byte);
                     xxxtinyint++;
                     xxxtinyint = 0 - xxxtinyint;
                }
                else
                    xxxtinyint = (xxxtinyint & mask_1_byte);
                printf("xxx xxxtinyint is %d\n", xxxtinyint);
                cur_ptr += 1;

                //wealthscore int
                uint64_t wealthscore = 0;
                if (!full_field_map[mwealthscore])
                {
                    wealthscore = mach_read_from_4(cur_ptr);
                    wealthscore = (wealthscore & mask);
                    printf("xxx wealthscore is %lu\n", wealthscore);
                    cur_ptr += 4;
                }

                //growscore int
                uint64_t growscore = 0;
                if (!full_field_map[mgrowscore])
                {
                    growscore = mach_read_from_4(cur_ptr);
                    growscore = (growscore & mask);
                    printf("xxx growscore is %lu\n", growscore);
                    cur_ptr += 4;
                }

                //monthgrowscore int
                uint64_t monthgrowscore = 0;
                if (!full_field_map[mmonthgrowscore])
                {
                    monthgrowscore = mach_read_from_4(cur_ptr);
                    monthgrowscore = (monthgrowscore & mask);
                    printf("xxx monthgrowscore is %lu\n", monthgrowscore);
                    cur_ptr += 4;
                }

                //`email` varchar 
                /*
                std::string email;
                for (int i=0; i<len_email; i++)
                {
                    char c = mach_read_from_1(cur_ptr + i);
                    email.push_back(c);
                }
                cout<<"xxx email is:"<<email<<endl;
                cur_ptr += len_email;
                */

                char line_buf[2048] = {0};
                snprintf(line_buf, 2048, "%lu,%s,%s,%s,%lu,%s,%lu,%s,%lu,%s," \ 
                        "%s,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu," \
                        "%lu,%s,%s,%s,%lu,%lu,%lu,%s,%s,%s," \
                        "%s,%lu,%s,%lu,%lu,%s,%lu,%lu,%lu,%lu," \
                        "%lu,%lu,%lu,%lu,%lu" \
                        "\n", 
                        row_id, username.c_str(), realname.c_str(), imgurl.c_str(), groupid, power.c_str(),create_time,create_user.c_str(),deleted,extinfo.c_str(),
                        extcount.c_str(),level,mclass,unionid,unionlevel,uniontime,sum_profit,period_profit,identity,course,
                        grade_level,idcard.c_str(),mobile.c_str(),password.c_str(),org_pid,org_cid,org_aid,org_name.c_str(),p_card1.c_str(),p_card2.c_str(),
                        p_card3.c_str(),activate,p_card4.c_str(),userlevel,point,account_id.c_str(),account_status,invate,xxxint,realstatus,
                        activate_time,xxxtinyint,wealthscore,growscore,monthgrowscore
                        );
                printf("yyy: %s\n", line_buf);
            }
        }

    }//for while

    //the end
    close(fd);
    return 0;
}
