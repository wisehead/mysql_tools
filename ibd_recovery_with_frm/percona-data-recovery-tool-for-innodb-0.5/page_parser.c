#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <error.h>
#include <tables_dict.h>
#include <fil0fil.h>

#include "innochecksum.h"

static time_t timestamp = 0;

// Global flags from getopt
bool deleted_pages_only = 0;
bool debug = 0;
bool process_compact = 0;
bool process_redundant = 0;

dulint filter_id;
bool use_filter_id = 0;

bool count_pages = 0;
bool ignore_crap = 0;
unsigned int page_counters[1000][10000UL]; // FIXME: need a hash here
unsigned long cache_size = 104857600; // 100M
off_t ib_size = 0;

// Checks if *page is valid InnoDB page
int valid_innodb_page(page_t *page){
    char buffer[32];
    int version = 0; // 1 - new, 0 - old
    unsigned int page_n_heap;
    int inf_offset = 0, sup_offset = 0;

    page_n_heap = mach_read_from_4(page + PAGE_HEADER + PAGE_N_HEAP);
    if(debug) fprintf(stderr, "Page PAGE_N_HEAP: %08X\n", page_n_heap);
    version = ((page_n_heap & 0x80000000) == 0) ? 0 : 1;
    if(debug) fprintf(stderr, "Page version: %d\n", version);
    if(version == 1){
        inf_offset = PAGE_NEW_INFIMUM;
        sup_offset = PAGE_NEW_SUPREMUM;
        }
    else{
        inf_offset = PAGE_OLD_INFIMUM;
        sup_offset = PAGE_OLD_SUPREMUM;
        }
    if(debug) fprintf(stderr, "Page infimum offset: %08X\n", inf_offset);
    if(debug) fprintf(stderr, "Page supremum offset: %08X\n", sup_offset);

    memcpy(buffer, page + inf_offset, 7);
    buffer[7] = '\0';
    if(debug) fprintf(stderr, "Page infimum record: %s\n", buffer);
    if(strncmp(buffer, "infimum", strlen(buffer)) == 0 ){
        memcpy(buffer, page + sup_offset, 8);
        buffer[8] = '\0';
        if(debug) fprintf(stderr, "Page supremum record: %s\n", buffer);
        if(strncmp(buffer, "supremum", 8) == 0 ){
            if(debug) fprintf(stderr, "Found valid page\n");
            return 1;
            }
        }
    if(debug) fprintf(stderr, "Invalid page\n");
    return 0;
}

inline int valid_innodb_checksum(page_t *p){
	ulint oldcsum, oldcsumfield, csum, csumfield;
	int result = 0;

	// Checking old style checksums
	oldcsum= buf_calc_page_old_checksum(p);
	oldcsumfield= mach_read_from_4(p + UNIV_PAGE_SIZE - FIL_PAGE_END_LSN_OLD_CHKSUM);
	if (oldcsumfield != mach_read_from_4(p + FIL_PAGE_LSN) && oldcsumfield != oldcsum){
		result = 0;
		goto valid_innodb_checksum_exit;
		}
	// Checking new style checksums
	csum= buf_calc_page_new_checksum(p);
	csumfield= mach_read_from_4(p + FIL_PAGE_SPACE_OR_CHKSUM);
	if (csumfield != 0 && csum != csumfield){
		result = 0;
		goto valid_innodb_checksum_exit;
		}
	// return success
	result = 1;
valid_innodb_checksum_exit:
	return result;
}

/*******************************************************************/
void process_ibpage(page_t *page) {
    static ulint id = 0;
    ulint page_id;
    ulint page_type;
    dulint index_id;
    int fn;

    // Get page info
    page_id = mach_read_from_4(page + FIL_PAGE_OFFSET);
    page_type = mach_read_from_2(page + FIL_PAGE_TYPE);
    index_id = mach_read_from_8(page + PAGE_HEADER + PAGE_INDEX_ID);
    
    // Skip empty pages
    if (ignore_crap && index_id.high == 0 && index_id.low == 0) goto process_ibpage_exit;
    
    // Skip tables if filter used
    if (use_filter_id && (index_id.low != filter_id.low || index_id.high != filter_id.high)) goto process_ibpage_exit;
        
    if (count_pages) {
        if (index_id.high >= 1000) {
            if (ignore_crap) goto process_ibpage_exit;
            printf("ERROR: Too high tablespace id! %ld >= 1000!\n", index_id.high);
            exit(1);
        }

        if (index_id.low >= 10000) {
            if (ignore_crap) goto process_ibpage_exit;
            printf("ERROR: Too high index id! %ld >= 10000!\n", index_id.low);
            exit(1);
        }
        
        page_counters[index_id.high][index_id.low]++;
        goto process_ibpage_exit;
    }
        
    // Create table directory
    char dir_name[256];
    char file_name[256];
    switch (page_type){
        case FIL_PAGE_INDEX:
            sprintf(dir_name, "pages-%u/FIL_PAGE_INDEX", (unsigned int)timestamp);
            if(mkdir(dir_name, 0755)&& errno != EEXIST) { 
                fprintf(stderr, "Can't make a directory %s: %s\n", dir_name, strerror(errno)); 
                exit(-1);
                }
            sprintf(dir_name, "%s/%lu-%lu", dir_name, index_id.high, index_id.low);
            if(mkdir(dir_name, 0755) && errno != EEXIST) {
                fprintf(stderr, "Can't make a directory %s: %s\n", dir_name, strerror(errno)); 
                exit(-1);
                }
            break;
        case FIL_PAGE_TYPE_BLOB:
            sprintf(dir_name, "pages-%u/FIL_PAGE_TYPE_BLOB", (unsigned int)timestamp);
            if(mkdir(dir_name, 0755)&& errno != EEXIST) { 
                fprintf(stderr, "Can't make a directory %s: %s\n", dir_name, strerror(errno));
                exit(-1);
                }
            break;
        default:
            sprintf(dir_name, "pages-%u/%lu", (unsigned int)timestamp, page_type);
            if(mkdir(dir_name, 0755)&& errno != EEXIST) {
                fprintf(stderr, "Can't make a directory %s: %s\n", dir_name, strerror(errno)); 
                exit(-1);
                }
            sprintf(dir_name, "%s/%lu-%lu", dir_name, index_id.high, index_id.low);
            if(mkdir(dir_name, 0755) && errno != EEXIST) { 
                fprintf(stderr, "Can't make a directory %s: %s\n", dir_name, strerror(errno)); 
                exit(-1);
                }
                break;

        }
    
    // Compose page file_name
    if(page_type == FIL_PAGE_TYPE_BLOB){
        sprintf(file_name, "%s/page-%08lu.page", dir_name, page_id);
        }
    else{
        sprintf(file_name, "%s/%lu-%08lu.page", dir_name, id++, page_id);
        }
    
    if(debug) fprintf(stderr, "Read page #%lu.. saving it to %s\n", page_id, file_name);

    // Save page data
    fn = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (!fn) error("Can't open file to save page!");
    write(fn, page, UNIV_PAGE_SIZE);
    close(fn);
process_ibpage_exit:
	return;
}

/*******************************************************************/
void process_ibfile(int fn) {
    unsigned int read_bytes;
    page_t *page = malloc(UNIV_PAGE_SIZE);
    page_t *cache = malloc(cache_size);
    ulint page_type;
    char tmp[20];
    off_t pos = 0;
    off_t pos_prev = 0;
    int last = 0;
    time_t ts, ts_prev;
    
    if (!page) error("Can't allocate page buffer!");
    if (!cache){
        fprintf(stderr, "Can't allocate %lu bytes for disk cache\n", cache_size);
        error("Disk cache allocation failed");
        }
    

    // Create pages directory
    timestamp = time(0);
    if (!count_pages) {
        sprintf(tmp, "pages-%u", (unsigned int)timestamp);
        mkdir(tmp, 0755);
    }
    
    if(cache_size > SSIZE_MAX){
        fprintf(stderr, "Cache can't be bigger than %lu bytes\n", SSIZE_MAX);
        error("Disk cache size is too big");
        }
    // Read pages to the end of file
    ts = time(0);
    ts_prev = ts;
    while ((read_bytes = read(fn, cache, cache_size)) > 0) {
        unsigned long i = 0;
	last = (read_bytes < cache_size) ? 1 : 0;
        if(debug) fprintf(stderr, "Read %u bytes\n", read_bytes);
        while (i < read_bytes - UNIV_PAGE_SIZE) {
            page_type = mach_read_from_2(cache + i + FIL_PAGE_TYPE);
            if (((page_type == FIL_PAGE_INDEX) && valid_innodb_page(cache + i)) || ((page_type == FIL_PAGE_TYPE_BLOB )&& valid_innodb_checksum(cache + i) )) {
                process_ibpage(cache + i);
            	i+=UNIV_PAGE_SIZE;
            	pos += UNIV_PAGE_SIZE;
                }
            else{
		i++;
            	pos++;
                }
            if ((pos - pos_prev) > 0.01 * ib_size) {
		ts = time(0);
		if((ts != ts_prev)){
                	char buf[32];
			struct tm timeptr;
			time_t t = (ib_size - pos)/((pos - pos_prev)/(ts - ts_prev)) + ts;
			memcpy(&timeptr,localtime(&t), sizeof(timeptr));
			strftime(buf, sizeof(buf), "%F %T", &timeptr);
                	fprintf(stderr, "%.2f%% done. %s ETA(in %02lu:%02lu hours). Processing speed: %lu B/sec\n", 
				100.0 * pos / ib_size, 
				buf,
				(t - ts)/3600,
				((t - ts) - ((t - ts)/3600)*3600)/60,
				((pos - pos_prev))/(ts - ts_prev)
				);
        	        pos_prev = pos;
			ts_prev = ts;
        	        }
		}
            }
	if(!last){
		lseek(fn, -(read_bytes - i), SEEK_CUR);
		}
        }
}

/*******************************************************************/
int open_ibfile(char *fname) {
    struct stat st;
    int fn;

    fprintf(stderr, "Opening file: %s:\n", fname);
       
    if(stat(fname, &st) != 0) {
       printf("Errno = %d, Error = %s\n", errno, strerror(errno));
       error("Can't stat data file! Looks like this file is too large and requires 64-bit stat(). Check your Makefile."); 
       }
    fprintf(stderr, "%lu\t\tID of device containing file\n", st.st_dev);
    fprintf(stderr, "%lu\t\tinode number\n", st.st_ino);
    fprintf(stderr, "%u\t\tprotection\n", st.st_mode);
    fprintf(stderr, "%lu\t\tnumber of hard links\n", st.st_nlink);
    fprintf(stderr, "%u\t\tuser ID of owner\n", st.st_uid);
    fprintf(stderr, "%u\t\tgroup ID of owner\n", st.st_gid);
    fprintf(stderr, "%lu\t\tdevice ID (if special file)\n", st.st_rdev);
    fprintf(stderr, "%lu\t\ttotal size, in bytes\n", st.st_size);
    fprintf(stderr, "%lu\t\tblocksize for filesystem I/O\n", st.st_blksize);
    fprintf(stderr, "%lu\t\tnumber of blocks allocated\n", st.st_blocks);
    fprintf(stderr, "%lu\ttime of last access\n", st.st_atime);
    fprintf(stderr, "%lu\ttime of last modification\n", st.st_mtime);
    fprintf(stderr, "%lu\ttime of last status change\n", st.st_ctime);

    fn = open(fname, O_RDONLY, 0);
    if (fn == -1){
        perror("Can't open file");
        exit(-1);
        }
    if(ib_size == 0){ // determine tablespace size if not given
    	if(st.st_size != 0){
    		ib_size = st.st_size;
   	 	}
   	 else{
		ib_size = lseek(fn, 0, SEEK_END);
		if(ib_size == -1){
			perror("Can't get the end postion of the file");
			exit(-1);
			}
		lseek(fn, 0, SEEK_SET);
	    	}
	    if(ib_size == 0){
	    	fprintf(stderr, "Can't determine size of %s. Specify it manually with -t option\n", fname);
	    	}
    	}
    fprintf(stderr, "%lu\tSize to process in bytes\n", ib_size);
    fprintf(stderr, "%lu\tDisk cache size in bytes\n", cache_size);
       
    return fn;
}

/*******************************************************************/
void init_page_counters() {
    int i, j;
    
    for (i = 0; i < 1000; i++)
        for (j = 0; j < 10000; j++)
            page_counters[i][j] = 0;
}

/*******************************************************************/
void dump_page_counters() {
    int i, j;
    
    for (i = 0; i < 1000; i++)
        for (j = 0; j < 10000; j++) {
            if (page_counters[i][j] == 0) continue;
            printf("%d:%d\t%u\n", i, j, page_counters[i][j]);
        }
}

/*******************************************************************/
void set_filter_id(char *id) {
    int cnt = sscanf(id, "%lu:%lu", &filter_id.high, &filter_id.low);
    if (cnt < 2) {
        error("Invalid index id provided! It should be in N:M format, where N and M are unsigned integers");
    }
    use_filter_id = 1;
}

/*******************************************************************/
void usage() {
    error(
      "Usage: ./page_parser -4|-5 [-dDhcCV] -f <innodb_datafile> [-T N:M] [-s size] [-t size]\n"
      "  Where\n"
      "    -h  -- Print this help\n"
      "    -V  -- Print debug information\n"
      "    -d  -- Process only those pages which potentially could have deleted records (default = NO)\n"
      "    -s size -- Amount of memory used for disk cache (allowed examples 1G 10M). Default 100M\n"
      "    -T  -- retrieves only pages with index id = NM (N - high word, M - low word of id)\n"
      "    -c  -- count pages in the tablespace and group them by index id\n"
      "    -C  -- count pages in the tablespace and group them by index id (and ignore too high/zero indexes)\n"
      "    -t  size -- Size of InnoDB tablespace to scan. Use it only if the parser can't determine it by himself.\n"
    );
}

/*******************************************************************/
int main(int argc, char **argv) {
    int fn = 0, ch;
        unsigned int m;
        char suffix;

    while ((ch = getopt(argc, argv, "45VhdcCf:T:s:t:")) != -1) {
        switch (ch) {
            case 'd':
                deleted_pages_only = 1;
                break;

            case 'f':
                fn = open_ibfile(optarg);
                break;

            case 'V':
                debug = 1;
                break;

            case '4':
                process_redundant = 1;
                break;

            case '5':
                process_compact = 1;
                break;

            case 's':
        sscanf(optarg, "%u%c", &m, &suffix);
        switch (suffix) {
            case 'k':
            case 'K':
                cache_size = m * 1024;
                break;
            case 'm':
            case 'M':
                cache_size = m * 1024 * 1024;
                break;
            case 'g':
            case 'G':
                cache_size = m * 1024 * 1024 * 1024;
                break;
            default:
                fprintf(stderr, "Unrecognized size suffix %c\n", suffix);
                usage();
            }
        if(cache_size < UNIV_PAGE_SIZE){
            fprintf(stderr, "Disk cache size %lu can't be less than %u\n", cache_size, UNIV_PAGE_SIZE);
            usage();
            }
	cache_size = (cache_size / UNIV_PAGE_SIZE ) * UNIV_PAGE_SIZE;
                break;
            case 't':
        sscanf(optarg, "%u%c", &m, &suffix);
        switch (suffix) {
            case 'k':
            case 'K':
                ib_size = m * 1024;
                break;
            case 'm':
            case 'M':
                ib_size = m * 1024 * 1024;
                break;
            case 'g':
            case 'G':
                ib_size = m * 1024 * 1024 * 1024;
                break;
            case 't':
            case 'T':
                ib_size = m * 1024 * 1024 * 1024 * 1024;
                break;
            default:
                fprintf(stderr, "Unrecognized size suffix %c\n", suffix);
                usage();
            }
                break;

            case 'T':
                set_filter_id(optarg);
                break;
                
            case 'c':
                count_pages = 1;
                ignore_crap = 0;
                init_page_counters();
                break;

            case 'C':
                count_pages = 1;
                ignore_crap = 1;
                init_page_counters();
                break;

            default:
            case '?':
            case 'h':
                usage();
        }
    }

    if (fn != 0) {
        process_ibfile(fn);
        close(fn);
        if (count_pages) {
            dump_page_counters();
        }
    } else usage();
    
    return 0;
}
