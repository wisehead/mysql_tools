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
//#include <set>
#include <assert.h>
#include<map>

using namespace std;

#define byte            unsigned char
#define errExit(msg) do {perror(msg);exit(EXIT_FAILURE);} while(0)

#define PAGE_SIZE  (16*1024L)

#define FIL_PAGE_DATA       38  /*!< start of the data on the page */
#define FSEG_PAGE_DATA      FIL_PAGE_DATA
#define PAGE_HEADER FSEG_PAGE_DATA  /* index page header starts at this
                offset */
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

uint64_t file_size = 0;

typedef struct {
        uint64_t page_no;
        uint64_t page_type;
        unsigned char buf[PAGE_SIZE];
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
    

//typedef map<int, uint64_t> ;

/********************************************************//**
The following function is used to fetch data from 2 consecutive
bytes. The most significant byte is at the lowest address.
@return ulint integer */
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
@return ulint integer */
uint64_t
mach_read_from_4(
/*=============*/
    const byte* b)  /*!< in: pointer to four bytes */
{
    //ut_ad(b);
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
page_header_get_field(
/*==================*/
    const byte*   page,   /*!< in: page */ 
    uint64_t      field)  /*!< in: PAGE_LEVEL, ... */
{
    //ut_ad(page);
    //ut_ad(field <= PAGE_INDEX_ID);

    return(mach_read_from_2(page + PAGE_HEADER + field));
}

int read_page_gen(int fd, Page* page/*out*/)
{
    uint64_t page_no = page->page_no; 
    int read_ret = pread(fd, page->buf, PAGE_SIZE, page_no * PAGE_SIZE);
    if( read_ret != PAGE_SIZE )
        errExit("read Page failed");
    return 0;
}

int main()
{
    int ret = -1;
    int fd;
    uint64_t page_count = 0;
    //uint64_t result[] = 0;
    Page page_obj;
    Page *page = &page_obj;
    uint64_t page_type_count_array[FIL_PAGE_TYPE_LAST] = {0};

    unsigned char *tmp;
    unsigned int page_offset = 0;
    unsigned int page_prev = 0;
    unsigned int page_next = 0;
    unsigned short page_type = 0;
    unsigned int tab_space_id = 0;
    //unsigned short page_level = 0;
    uint64_t free_space = 0;

    if(!(fd = open("./tb_defragment.ibd",O_RDONLY))) {
        printf("open failed!\n");
        return -1;
    } 
    if((file_size = lseek(fd,0,SEEK_END))<0)
    {
       perror("lseek file failure!");
    }
    printf("file_size is %d\n",file_size);

    uint64_t max_page_no = file_size/PAGE_SIZE;

    while (page_count < max_page_no) {
        printf("processing the %d [rd] page\n",page_count);
        page->page_no = page_count;

        if(!read_page_gen(fd, page)) {
            page_count++;
        } else {
            printf("readpage failed!\n");
            //system("pause");
            exit(-1);
        }
        
        tmp = (unsigned char *)&page_offset;
        tmp[0] = page->buf[7];
        tmp[1] = page->buf[6];
        tmp[2] = page->buf[5];
        tmp[3] = page->buf[4];

        tmp = (unsigned char *)&page_prev;
        tmp[0] = page->buf[11];
        tmp[1] = page->buf[10];
        tmp[2] = page->buf[9];
        tmp[3] = page->buf[8];
                
        tmp = (unsigned char *)&page_next;
        tmp[0] = page->buf[15];
        tmp[1] = page->buf[14];
        tmp[2] = page->buf[13];
        tmp[3] = page->buf[12];

        tmp = (unsigned char *)&page_type;
        tmp[0] = page->buf[25];
        tmp[1] = page->buf[24];
                
        tmp = (unsigned char *)&tab_space_id;
        tmp[0] = page->buf[37];
        tmp[1] = page->buf[36];
        tmp[2] = page->buf[35];
        tmp[3] = page->buf[34];
                
        /*
        tmp = (unsigned char *)&page_level;
        tmp[0] = page->buf[61];
        tmp[1] = page->buf[60];
        */
        
        unsigned short page_heap_top = page_header_get_field(page->buf, PAGE_HEAP_TOP) & 0x7fff;
        unsigned short page_n_heap = page_header_get_field(page->buf, PAGE_N_HEAP) & 0x7fff;
        unsigned short page_free = page_header_get_field(page->buf, PAGE_FREE) & 0x7fff;
        unsigned short page_garbage = page_header_get_field(page->buf, PAGE_GARBAGE) & 0x7fff;
        unsigned short page_n_recs = page_header_get_field(page->buf, PAGE_N_RECS) & 0x7fff;
        unsigned short page_level = page_header_get_field(page->buf, PAGE_LEVEL) & 0x7fff;
        uint64_t       page_index_id = btr_page_get_index_id(page->buf);

        if (page_type == 17855){ 
            page_type_count_array[FIL_PAGE_INDEX]++;
        } else {
            page_type_count_array[page_type]++;
        }
        //if (page_type == 17855 && page_n_heap != page_n_recs+2) 
        //if (page_type == 17855) 
            free_space += page_garbage;
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
        //}
        printf("free_space is %dMB\n\n",free_space/1024/1024);
    }//for while

    for (int i=0; i<FIL_PAGE_TYPE_LAST;i++){
        printf("count of %s is %d\n",page_type_name[i], page_type_count_array[i]);
    } 


    //the end
    close(fd);
    //system("pause");
    return 0;
}
