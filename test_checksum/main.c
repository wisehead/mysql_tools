/*******************************************************************************
 *      File Name: main.c                                               
 *         Author: Hui Chen. (c) 2015                             
 *           Mail: chenhui13@baidu.com                                        
 *   Created Time: 2015/05/21-19:45                                    
 *	Modified Time: 2015/05/21-19:45                                    
 *******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>


#define BUF_MAX_LEN_16K 16384
#define FIL_MAX_SIZ_1G  1073741824 
#define FIL_MAX_SIZ_500M  536870912 
#define FIL_MAX_SIZ_10G  10737418240 
#define PAG_COUNT_IN_FILE 655360
#define PAG_COUNT_IN_1G_FILE 65536

#define UNIV_PAGE_SIZE 16384
#define CHECKSUM_LEN 8

typedef unsigned char byte;
typedef unsigned long int   ulint;
#define UT_HASH_RANDOM_MASK 1463735687
#define UT_HASH_RANDOM_MASK2    1653893711

#define MAX 10
#define THD_NUM 10

pthread_t id[THD_NUM];
pthread_mutex_t mut; //初始化静态互斥锁
int number = 0;
char buf[BUF_MAX_LEN_16K*10] = {0};
int fd;

ulint buf_calc_page_new_checksum(const byte* page);

void thread_fun(void *args)
{
    ulint i=(ulint)args;
    printf("Hello,I am pthread%d!\n",args);
    
    while(1)
    {
        rand_read_write(fd, &buf[i*BUF_MAX_LEN_16K], i*FIL_MAX_SIZ_1G, (i+1)*FIL_MAX_SIZ_1G);
        //rand_write(fd, &buf[i*BUF_MAX_LEN_16K], i*FIL_MAX_SIZ_1G, (i+1)*FIL_MAX_SIZ_1G);
        //if(is_the_file_corrupt(fd, &buf[i*BUF_MAX_LEN_16K], i*FIL_MAX_SIZ_1G, (i+1)*FIL_MAX_SIZ_1G))
        //    break;
    }
    pthread_exit(NULL); //线程通过执行此函数，终止执行。返回是一个空指针类型
}

void thread_create(void)
{
    int temp, i;
    memset(&id, 0, sizeof(id));

    for (i=0; i<THD_NUM; i++)
    {
        if(temp = pthread_create(&id[i], NULL, (void *)thread_fun,(void *)i )!= 0)
            printf("Thread %d fail to create!\n",i);
        else
            printf("Thread %d created\n",i);        
    }
}

void thread_wait()
{
    int i;
    for (i=0; i<THD_NUM; i++)
    {
        if (id[i] != 0)
        {
            pthread_join(id[i], NULL);
            printf("Thread%d completed!\n",i);
        }
    }
}

int main(void)
{
    int i,ret1,ret2;
    pthread_mutex_init(&mut, NULL); //动态互斥锁

    fd = open("./datafile",O_CREAT|O_RDWR|O_TRUNC,S_IRWXU);
   
    ulint checksum = buf_calc_page_new_checksum(buf);
    printf("checksum = %x\n",checksum);
    ulint* ptr = (ulint *)&buf[UNIV_PAGE_SIZE - CHECKSUM_LEN];
    *ptr = checksum;

    for (i=0; i<PAG_COUNT_IN_FILE; i++)
    {
        write_page(fd, i, buf);
    }

    printf("Main fuction,creating thread...\n");
    thread_create();
    printf("Main fuction, waiting for the pthread end!\n");
    thread_wait();
    close(fd);
    return (0);

}

int read_page(int fd, ulint page, char* buf)
{
  //printf("in read_page()\n");
  int ret_offset, ret;
  ulint offs = page * BUF_MAX_LEN_16K;
  ulint stored_checksum, calculated_checksum;
  ulint* ptr;

  ret = pread(fd, buf, (ssize_t) (BUF_MAX_LEN_16K), offs);
  if (ret < 0)
  {
      printf(" in read_page, read error\n");
      return ret;
  }
  
  ptr = (ulint *)&buf[UNIV_PAGE_SIZE - CHECKSUM_LEN];
  stored_checksum = *ptr;
  calculated_checksum = buf_calc_page_new_checksum(buf);
  if (stored_checksum != calculated_checksum)
  {
    printf("stored checksum = %d,  calculated checksum = %d\n",stored_checksum, calculated_checksum);
    ret = -1;
  }  
  if (ret >=0 )
    ret = 0;
  return ret;
}

int is_the_file_corrupt(int fd, char* buf, ulint start, ulint end)
{
    int i, j, ret=0, j_start, j_end;
    j_start = start/UNIV_PAGE_SIZE;
    j_end = end/UNIV_PAGE_SIZE;
    printf("=== in is_the_file_corrupt. j_start=%d, j_end=%d\n",j_start,j_end);
    for (j=j_start; j<j_end; j++)
    {  
        if( read_page(fd, j, buf))
            return -1;
    }
    return ret;
}
int rand_read_write(int fd, char* buf, ulint start, ulint end)
{
    int i, j, ret=0, j_start, j_end;
    j_start = start/UNIV_PAGE_SIZE;
    j_end = end/UNIV_PAGE_SIZE;
    int random = (rand()%(end - start))/UNIV_PAGE_SIZE;
    int page_no = j_start + random;
    //printf("=== in rand_read_write. j_start=%d, j_end=%d, randam=%d, page_no=%d\n",j_start,j_end,random,page_no);
    printf("page_no=%d\n",page_no);
    if( read_page(fd, page_no, buf))
    {
        return -1; 
        exit(0);
    }
    rand_buf_data(buf);
    ulint checksum = buf_calc_page_new_checksum(buf);
    //printf("checksum = %x\n",checksum);
    ulint* ptr = (ulint *)&buf[UNIV_PAGE_SIZE - CHECKSUM_LEN];
    *ptr = checksum;
    //printf("=== in rand_read_write, changing byte = %d ============\n", i); 
    write_page(fd, page_no, buf );
   
    return ret;
}

int rand_buf_data(char* buf)
{
    ulint random = rand();
    int pos = random%BUF_MAX_LEN_16K;
    if (pos >= BUF_MAX_LEN_16K - 8)
        pos -= 8;
    int data = random%256;
    //printf("== in rand_buf_data, pos=%x, data=%x\n",pos,data);
    buf[pos] = data;
}

int write_page(int fd, ulint page, char* buf)
{
  //printf("in write_page(), page num = %d\n", page);
  int ret_offset, ret;
  ulint offs = page * BUF_MAX_LEN_16K;
  ret = pwrite(fd, buf, (ssize_t) (BUF_MAX_LEN_16K), offs);
  return ret;
}

int rand_write(int fd, char* buf, ulint start, ulint end)
{
    int i, j, j_start, j_end;
    j_start = start/UNIV_PAGE_SIZE;
    j_end = end/UNIV_PAGE_SIZE;
    ulint checksum;
    ulint* ptr=NULL;
    //fd = open("./datafile", O_WRONLY|O_CREAT);
    for (i=0; i<BUF_MAX_LEN_16K; i++)
    {
        buf[i] = 1;
        checksum = buf_calc_page_new_checksum(buf);
        printf("checksum = %x\n",checksum);
        ptr = (ulint *)&buf[UNIV_PAGE_SIZE - CHECKSUM_LEN];
        *ptr = checksum;
        printf("========== in rand_write, changing byte = %d ============\n", i);
        //for (j=0; j<PAG_COUNT_IN_FILE; j++)
        for (j=j_start; j<j_end; j++)
        {   
            write_page(fd, j, buf);
        }
    }   
    //close(fd); 
}





ulint
ut_fold_ulint_pair(
/*===============*/
    ulint   n1, /*!< in: ulint */
    ulint   n2) /*!< in: ulint */
{
    return(((((n1 ^ n2 ^ UT_HASH_RANDOM_MASK2) << 8) + n1)
        ^ UT_HASH_RANDOM_MASK) + n2);
}

ulint
ut_fold_binary(
/*===========*/
    const byte* str,    /*!< in: string of bytes */
    ulint       len)    /*!< in: length */
{
    ulint       fold = 0;
    const byte* str_end = str + (len & 0xFFFFFFF8);

    //ut_ad(str || !len);

    while (str < str_end) {
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
    }

    switch (len & 0x7) {
    case 7:
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
    case 6:
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
    case 5:
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
    case 4:
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
    case 3:
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
    case 2:
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
    case 1:
        fold = ut_fold_ulint_pair(fold, (ulint)(*str++));
    }

    return(fold);
}



ulint
buf_calc_page_new_checksum(
/*=======================*/
    const byte* page)   /*!< in: buffer page */
{
    ulint checksum;

    checksum = ut_fold_binary(page,
                  UNIV_PAGE_SIZE - CHECKSUM_LEN);
    //checksum = checksum & 0xFFFFFFFFUL;

    return(checksum);
}


