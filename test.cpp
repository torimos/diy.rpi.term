#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
int main_tttt(int argc, char *argv[])
{
	struct timespec ts1;
	timespec_get(&ts1, TIME_UTC);
	int ttyfd = open("/dev/tty1", O_RDWR);
//	
//	char* fbp = (char *)mmap(0, 53*204, PROT_READ | PROT_WRITE, MAP_SHARED, ttyfd, 0);
//	if (fbp == 0)
//	{
//		return -1;		
//	}
//	
//	for (int i = 0; i < 100; i++) *fbp++='#';
	
	printf("Test %d", ts1.tv_sec);
	write(ttyfd, "\033[2J\033[1;1H", 14);
	write(ttyfd, "1 hello tty!\n", 11);
	write(ttyfd, "2 hello tty!\n", 11);
	return 0;
}