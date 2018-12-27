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

#define BUF_MAX_LEN_16K 16384
#define FIL_MAX_SIZ_1G  1073741824 

main()
{
    int fd, size;
    char s[] = "Linux Programmer!\n", buffer[80];
    fd = open("./datafile", O_WRONLY|O_CREAT);
    write(fd, s, sizeof(s));
    close(fd);
    fd = open("./datafile", O_RDONLY);
    size = read(fd, buffer, sizeof(buffer));
    close(fd);
    printf("%s", buffer);
}


