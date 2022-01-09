#include<fcntl.h>
#include<stdio.h>
#include<unistd.h>

#include "myio.h"


int main(int argc, char *argv[])
{
    struct file_info *file1;
	struct file_info *file2;

    char buf[100];

    int readBytes = 0;
	int writeBytes = 0;

    file1 = myopen("file1", O_RDWR);
	file2 = myopen("file2", O_RDWR);

	printf("\nmyread 15 from file1 and mywrite into file2 3 times\n");	

	readBytes = myread(file1, buf, 15);
	writeBytes = mywrite(file2, buf, readBytes);
	
	readBytes = myread(file1, buf, 15);
	writeBytes = mywrite(file2, buf, readBytes);
	
	readBytes = myread(file1, buf, 15);
	writeBytes = mywrite(file2, buf, readBytes);
	printf("finalread: %d, finalwrite: %d\n", readBytes, writeBytes);

    printf("\ncheck file2 for correct output\n");	

	myclose(file1);
	myclose(file2);
}
