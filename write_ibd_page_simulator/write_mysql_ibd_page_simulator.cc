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
#include <set>
#include <assert.h>

#define errExit(msg) do {perror(msg);exit(EXIT_FAILURE);} while(0)

static uint64_t rand_seed = 1;
static const uint64_t PAGE_SIZE = 16*1024;
static const uint64_t CHECKSUM_SIZE = 4;
static const uint64_t BUFFER_SIZE = 1024*1024;
static const uint64_t file_size = 15 * 1024 * 1024 * 1024L;
static const uint64_t max_page_no = file_size / PAGE_SIZE;
static pthread_mutex_t count_mutex;
static uint64_t op_count;
typedef struct {
        uint64_t page_no;
        unsigned char buf[PAGE_SIZE];
        unsigned int checksum;
} Page;
typedef struct{
	int fd;
	pthread_t  pt;
	uint64_t start;
	uint64_t end;
	int worker_id;
}Work_Area;

uint64_t rand_size(uint64_t seed);
static unsigned int do_csum(const unsigned char *buff, int len);

int create_open_file(const char* file_name,uint64_t file_size);
int read_page_gen(int fd,Page* page/*out*/);
void init_page_for_write(Page *page);
int write_page_gen(int fd,Page* page);

int check_stop();
void* page_worker(void*);

int main(void)
{
	size_t work_num = 1;
  uint64_t max_page = file_size / PAGE_SIZE;
	pthread_mutex_init(&count_mutex,NULL);	
  int fd = create_open_file("./simulate_ibd",file_size);
	Work_Area *wa = new Work_Area[10];
	for( int i = 0;i < work_num ;i++)	
	{
		wa[i].fd = fd;
		wa[i].start = (file_size / work_num) * i;
		wa[i].end = (file_size / work_num) * (i + 1);
		wa[i].worker_id = i;
		fprintf(stderr,"worker %lu start with fd %lu [%lu,%lu)\n",wa[i].worker_id,wa[i].fd,wa[i].start,wa[i].end);
		if(wa[i].end > file_size)
			wa[i].end = file_size;
		pthread_create(&wa[i].pt,NULL,page_worker,(void*)(wa + i));
	}
	for( int i = 0;i < work_num ;i++)	
	{
		pthread_join(wa[i].pt,NULL);
	}
	delete []wa;
        close(fd);	
}
void* page_worker(void* ptr)
{
	Work_Area* wa = (Work_Area*)ptr;
	int fd = wa->fd;
	uint64_t start_page_no = wa->start /PAGE_SIZE;
	uint64_t slot_width = (wa->end - wa->start) / PAGE_SIZE;
        Page page;
	uint64_t count = 0;
        while(true)
        {
            page.page_no = start_page_no + rand_size(slot_width);
	    assert(page.page_no < max_page_no);
            if(read_page_gen(fd,&page) != 0 )
	    {
                errExit("Read Page Failed");
	    }

           page.page_no = start_page_no + rand_size(slot_width);
	   assert(page.page_no < max_page_no);
           if(write_page_gen(fd,&page) <= 0)
	   {
               fprintf(stderr,"Worker %lu write fd %lu Page %lu Failed\n",wa->worker_id,wa->fd,page.page_no);
               errExit("Write Page Failed");
	   }

	    if(check_stop())
		break;
        } 
        fprintf(stderr,"Worker %lu with fd %lu exit\n",wa->worker_id,wa->fd);
}
int check_stop()
{
	pthread_mutex_lock(&count_mutex);
	op_count++;	
	if( op_count % 10000 == 0)
		fprintf(stderr,"%lu ",op_count);			
	pthread_mutex_unlock(&count_mutex);
	return 0;
}

int create_open_file(const char* file_name,uint64_t file_size)
{
        int fd = open(file_name,O_CREAT|O_RDWR|O_TRUNC,S_IRWXU);
        uint8_t buf[BUFFER_SIZE];
        memset(buf,0,BUFFER_SIZE);
        uint64_t size_write = 0;
        if(lseek(fd,0,SEEK_SET) == -1)
                errExit("lseek failed");
	fprintf(stderr,"Create File %s Succeed Begin to Init Pages\n",file_name);
        while (size_write < file_size )
        {
                int write_ret_val;
                uint64_t write_once = file_size - size_write;
                Page page;
                if(write_once > BUFFER_SIZE )
                        write_once = BUFFER_SIZE;
                for(int offset = 0;offset < write_once ;offset += PAGE_SIZE )
                {
                    page.page_no = (size_write + offset) / PAGE_SIZE;
                    init_page_for_write(&page);
                    memcpy(buf + offset,page.buf,PAGE_SIZE);
                }
                write_ret_val = pwrite( fd, buf, write_once,size_write);
                if (write_ret_val != BUFFER_SIZE)
                        errExit("write failed");
                size_write += write_once;
		if(size_write % (100*1024*1024) == 0)
			fprintf(stderr,"%luMB ",size_write / (1024*1024));
        }
	fprintf(stderr," 100%!!!\n");
        return fd;
}
int read_page_gen(int fd,Page* page/*out*/)
{
        uint64_t page_no = page->page_no; 
        //if(lseek(fd,page_no * PAGE_SIZE,SEEK_SET) == -1)
         //       errExit("lseek failed");
        int read_ret = pread(fd,page->buf,PAGE_SIZE,page_no * PAGE_SIZE);
        if( read_ret != PAGE_SIZE )
                errExit("read Page failed");
        page->checksum = *(unsigned int*)(page->buf);
        unsigned int caculate_checksum = do_csum(page->buf + CHECKSUM_SIZE, PAGE_SIZE - CHECKSUM_SIZE);
        if( page->checksum != caculate_checksum )
        {
            fprintf(stdout,"Found checksum error in page %lu,stored checksum is %lu ,caculated checksum is %d\n",page_no,page->checksum,caculate_checksum);
            return 1;
        }else{
            return 0;
        }
        
}
int write_page_gen(int fd,Page* page)
{
     int ret = 0;
     init_page_for_write(page);
     assert(page->page_no < max_page_no);
     return pwrite(fd,page->buf,PAGE_SIZE,page->page_no * PAGE_SIZE);   
}
void init_page_for_write(Page *page)
{
        uint64_t write_size = CHECKSUM_SIZE;
        uint64_t write_once = 0;
        uint64_t chunk_size = 4*1024;
        unsigned char * buf = page->buf;
        char     byte[4];
        int      i = 0;

        while(write_size < PAGE_SIZE)
        {
          if(PAGE_SIZE - write_size > chunk_size)
                write_once = chunk_size;
          else
                write_once = PAGE_SIZE - write_size;
          char c = (char)rand_size(26);
          byte[i] = c;
          i++;
          memset(buf + write_size,c,write_once);
          write_size += write_once;
        }
        page->checksum = do_csum(page->buf + CHECKSUM_SIZE, PAGE_SIZE - CHECKSUM_SIZE);
        fprintf(stdout, "write byte:%02x-%02x-%02x-%02x, checksum:%08x, offset:%lx\n",
                byte[0], byte[1], byte[2], byte[3], page->checksum, page->page_no * PAGE_SIZE);

        *(unsigned int*)(page->buf) = page->checksum;
}
static unsigned int do_csum(const unsigned char *buff, int len)
{
        int odd;
        unsigned int result = 0;

        if (len <= 0)
                goto out;
        odd = 1 & (unsigned long) buff;
        if (odd) {
#ifdef __LITTLE_ENDIAN
                result += (*buff << 8);
#else
                result = *buff;
#endif
                len--;
                buff++;
        }
        if (len >= 2) {
                if (2 & (unsigned long) buff) {
                        result += *(unsigned short *) buff;
                        len -= 2;
                        buff += 2;
                }
                if (len >= 4) {
                        const unsigned char *end = buff + ((unsigned)len & ~3);
                        unsigned int carry = 0;
                        do {
                                unsigned int w = *(unsigned int *) buff;
                                buff += 4;
                                result += carry;
                                result += w;
                                carry = (w > result);
                        } while (buff < end);
                        result += carry;
                        result = (result & 0xffff) + (result >> 16);
                }
                if (len & 2) {
                        result += *(unsigned short *) buff;
                        buff += 2;
                }
        }
        if (len & 1)
#ifdef __LITTLE_ENDIAN
                result += *buff;
#else
                result += (*buff << 8);
#endif
out:
        return result;
}
uint64_t rand_size(uint64_t seed){
        if(seed == 0)
                return 0;
        srand(rand_seed++);
        return rand()%seed;
}
