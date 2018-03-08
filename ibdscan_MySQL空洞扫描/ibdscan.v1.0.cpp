/*******************************************************************************
 *      File Name: main.cpp                                             
 *         Author: Hui Chen. (c) 2016                             
 *           Mail: chenhui13@baidu.com                                        
 *   Created Time: 2016/06/07-15:21                                    
 *	Modified Time: 2016/06/07-15:21                                    
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
#include<map>

using namespace std;

#ifndef TRUE

#define TRUE    1
#define FALSE   0

#endif

#define byte            unsigned char
#define ERR_EXIT(msg) do {perror(msg);exit(EXIT_FAILURE);} while(0)

#define PAGE_SIZE_16K  (16*1024L)
#define PAGE_SIZE g_page_size 
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


uint64_t g_page_size = 16*1024;
bool g_print_verbose = false;
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

int main(int argc, char** argv)
{
    char* ptr_file_name = NULL;
    if (argc > 1 
            && (strncmp(argv[1],"-h",2) == 0 
                || strncmp(argv[1],"--h",3) == 0 
                || strncmp(argv[1],"-help",5) == 0 
                || strncmp(argv[1],"--help",6) == 0)){
        printf("Usage: ibdscan [-c/-compress] ibd_file.\n");
        exit(-1);
    }

    if (argc > 2 && strncmp(argv[argc-1],"-v",2) == 0){
        g_print_verbose = true;
        argc--;
    }

    if (argc != 2 && argc != 3){
        printf("Usage: ibdscan [-c/-compress] ibd_file.\n");
        exit(-1);
    }

    if (argc == 3) {
        if (strncmp(argv[argc-2],"-c",2) != 0 && strncmp(argv[argc-2],"-compress",9) != 0) {
            printf("Usage: ibdscan [-c/-compress] ibd_file.\n");
            exit(-1);
        }
        /* using compress, the PAGE_SIZE should be 8K */
        g_page_size = g_page_size/2;
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
        if (g_print_verbose) {
            printf("processing the %d [rd] page\n",page_count);
        }
        page->page_no = page_count;

        if(!read_page_gen(fd, page)) {
            page_count++;
        } else {
            printf("readpage failed!\n");
            exit(-1);
        }
        
        page_offset = fil_header_get_field(page->buf, FIL_PAGE_OFFSET, 4);
        page_prev = fil_header_get_field(page->buf, FIL_PAGE_PREV, 4);
        page_next = fil_header_get_field(page->buf, FIL_PAGE_NEXT, 4);
        page_type = fil_header_get_field(page->buf, FIL_PAGE_TYPE, 2);
        tab_space_id = fil_header_get_field(page->buf, FIL_PAGE_SPACE_ID, 2);

        page_heap_top = page_header_get_field(page->buf, PAGE_HEAP_TOP) & 0x7fff;
        page_n_heap = page_header_get_field(page->buf, PAGE_N_HEAP) & 0x7fff;
        page_free = page_header_get_field(page->buf, PAGE_FREE) & 0x7fff;
        page_garbage = page_header_get_field(page->buf, PAGE_GARBAGE) & 0x7fff;
        page_n_recs = page_header_get_field(page->buf, PAGE_N_RECS) & 0x7fff;
        page_level = page_header_get_field(page->buf, PAGE_LEVEL) & 0x7fff;
        page_index_id = btr_page_get_index_id(page->buf);
        descr_page_no = 0;

        if (page->page_no == 0) {
            size = mach_read_from_4(page->buf + FSP_HEADER_OFFSET + FSP_SIZE);
            if (size != max_page_no) {
                printf ("the PAGE_SIZE is incorrect, please double check whether the compress is enabled.\n");
                exit(-1);
            }
            n_free_list_ext = mach_read_from_4(page->buf + FSP_HEADER_OFFSET + FSP_FREE + FLST_LEN);
            free_limit = mach_read_from_4(page->buf + FSP_HEADER_OFFSET + FSP_FREE_LIMIT);
            n_free_up = (size - free_limit) / EXTENT_SIZE;
            n_free = n_free_list_ext + n_free_up;
        }
        /* check the xdes map, calculate the freed page and the in-used page. */
        if (page_type == 17855){ 
            page_type_count_array[FIL_PAGE_INDEX]++;
            descr_page_no = xdes_calc_descriptor_page(page_count - 1);
            page->page_no = descr_page_no;

            if(!read_page_gen(fd, page)) {
                byte *descr = page->buf + XDES_ARR_OFFSET + XDES_SIZE * xdes_calc_descriptor_index(page_count - 1);
                uint64_t xdes_state = xdes_get_state(descr); 
                if (g_print_verbose) {
                    printf("xdes_state is %d\n",xdes_state);
                }
                if (xdes_state == XDES_FSEG) {
                    if (xdes_get_bit(descr, XDES_FREE_BIT, (page_count - 1) % EXTENT_SIZE) == TRUE) {
                        if (g_print_verbose) {
                            printf("this page is free\n");
                        }
                        freed_page_count++;
                    } else {
                        free_space += page_garbage;
                        if (g_print_verbose) {
                            printf("this page is in use\n");
                        }
                    }
                }
            } else {
                printf("readpage failed!\n");
                exit(-1);
            }
        } else {
            if (page_type > FIL_PAGE_TYPE_LAST) {
                printf ("error page_type:%d\n", page_type);
                exit(-1);
            }
            page_type_count_array[page_type]++;
        }

        if (g_print_verbose) {
            printf("page_offset is %d\n",page_offset);
            printf("page_prev is %d\n",page_prev);
            printf("page_next is %d\n",page_next);
            printf("page_index_id is %ld\n",page_index_id);
            printf("page_type is %x\n",page_type);
            printf("tab_space_id is %x\n",tab_space_id);
            printf("page_level is %x\n",page_level);

            printf("page_heap_top is %d\n",page_heap_top);
            printf("page_n_heap is %d\n",page_n_heap);
            printf("page_free is %d\n",page_free);
            printf("page_garbage is %d\n",page_garbage);
            printf("page_n_recs is %d\n\n",page_n_recs);
            printf("free_space is %dKB\n\n",free_space/1024);
        }
    }//for while

    if (g_print_verbose) {
        printf("size is %d\n",size);
        printf("n_free_list_ext is %d\n",n_free_list_ext);
        printf("free_limit is %d\n",free_limit);
        printf("n_free_up is %d\n",n_free_up);
        printf("n_free is %d\n\n",n_free);
        printf("--------------------------------------------------\n"); 
    }

    printf("The file size is %ldMB\n",g_file_size/1024/1024);
    uint64_t total_free_space = page_type_count_array[0] * PAGE_SIZE + freed_page_count * PAGE_SIZE + free_space;
    printf("The total available space is %ldMB \n\n", total_free_space/1024/1024);
    printf("the available space in free extents is %dMB (%d%)\n",page_type_count_array[0] * PAGE_SIZE/1024/1024, 100 * page_type_count_array[0]/max_page_no);
    printf("the space of unused pages in B-tree Node is %dMB (%d%)\n", freed_page_count * PAGE_SIZE / 1024/1024, 100 * freed_page_count/max_page_no); 
    printf("the fragment in B-tree node is %dMB (%d%)\n\n",free_space/1024/1024, 100 * free_space/PAGE_SIZE/max_page_no);

    printf("count of total pages is %d\n", max_page_no);
    for (int i=0; i<FIL_PAGE_TYPE_LAST;i++){
        printf("count of %s is %d\n",page_type_name[i], page_type_count_array[i]);
    } 

    //the end
    close(fd);
    return 0;
}
