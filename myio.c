/*CSCI 315 Assignment 2*/

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "myio.h"
#define BUFFER_SIZE 10

enum IOtype{NOTREAD, READ};

struct file_info{
    int fd;

	char *read_buf;
	int read_pointer;
	int read_actual;
	
	char *write_buf;
	int write_pointer;

    int previous;
	int file_offset;
};

// helper functions
int __write_after_read(struct file_info *file, char *buf, size_t count);

/**
 * @brief open the file specified by pathname
 * 
 * @param pathname file to open
 * @param flags open flags
 * @return struct file_info* 
 */
struct file_info* myopen(char *pathname, int flags){
	struct file_info *file;    

	file = malloc(sizeof(struct file_info));
	if (file == NULL)
		goto struct_failure;
	file->fd = open(pathname, flags, 0666);
	if (file->fd == -1)
		goto fd_failure;
	file->write_buf = (char *)malloc(BUFFER_SIZE);
	if (file->write_buf == NULL)
		goto wbuf_failure;
	file->read_buf = (char *)malloc(BUFFER_SIZE);
	if (file->read_buf == NULL)
		goto rbuf_failure;


    file->read_pointer = 0;
    file->read_actual = 0;
    file->write_pointer = 0;
    file->file_offset = 0;
    file->previous = NOTREAD;
	
	return file;

rbuf_failure:
	free(file->write_buf);
wbuf_failure:
	close(file->fd);
fd_failure:
	free(file);
struct_failure:
	return NULL;
}


/**
 * @brief closes a file
 * 
 * @param file file to close
 * @return int 
 */
int myclose(struct file_info *file){
	if (file->write_pointer != 0)
		myflush(file);
	free(file->read_buf);
	free(file->write_buf);
	int closeFd = file->fd;
	free(file);
	return close(closeFd);
}


/**
 * @brief read up to count bytes from file into the buffer starting at buf
 * 
 * @param file file to read from 
 * @param buf buffer
 * @param count bytes to read
 * @return int bytes actuall read
 */
int myread(struct file_info *file, void *buf,  size_t count){
	int add_offset = 0;
	int bytes_read = 0;
    
	if (count > BUFFER_SIZE){
		if ((bytes_read = read(file->fd, buf, count)) == -1)
            return -1;
    
		file->read_pointer = bytes_read;
        file->file_offset += bytes_read;
        file->previous = READ;
        return bytes_read;
	}

	if (file->read_pointer == 0){ // first myread call
		if ((bytes_read = read(file->fd, file->read_buf, BUFFER_SIZE)) == -1)
            return -1;
		file->read_actual = bytes_read;

		if (bytes_read < count){
			file->read_pointer = bytes_read;
		} else {
			file->read_pointer = count;
		}
		memcpy(buf, file->read_buf, file->read_pointer);
		add_offset = file->read_pointer;
	} else if (file->read_pointer + count > BUFFER_SIZE ){ // overflow
		int remain = file->read_actual - file->read_pointer;
        int overflow = count - remain;

        // read remaining bytes
		memcpy(buf, file->read_buf + file->read_pointer, remain);

        // refill buffer
		if ((bytes_read = read(file->fd, file->read_buf, BUFFER_SIZE)) == -1)
            return -1;
		file->read_actual = bytes_read;

		if (bytes_read < overflow){
			add_offset = remain + bytes_read;
            file->read_pointer = bytes_read;
		} else {
			file->read_pointer = overflow;
			add_offset = count;
		}

        // read overflow bytes
		memcpy((char*)buf + remain, file->read_buf, file->read_pointer);
	} else if (file->read_pointer + count > file->read_actual){ // overflow near EOF
        int remain = file->read_actual - file->read_pointer;
		memcpy(buf, file->read_buf + file->read_pointer, remain);
		file->read_pointer = 0;
		add_offset = remain;
	} else {
		memcpy(buf, file->read_buf + file->read_pointer, count);
		file->read_pointer += count;
		add_offset = count;
	}

	file->file_offset += add_offset;
    file->previous = READ;
	return add_offset;
}


/**
 * @brief write up to count bytes from the buffer starting at buf to file
 * 
 * @param file file to write
 * @param buf buffer to write from
 * @param count bytes to write
 * @return int bytes actually wrote
 */
int mywrite(struct file_info *file, void *buf, size_t count) {
	int bytes_wrote = 0;

	if (count > BUFFER_SIZE){
		if ((bytes_wrote = write(file->fd, buf, count)) == -1)
            return -1;
        count = bytes_wrote;
	} else if (file->previous == READ) {
        if ((bytes_wrote = __write_after_read(file, buf, count)) == -1)
            return -1;
    } else if (file->write_pointer + count > BUFFER_SIZE) {
		int remain = BUFFER_SIZE - file->write_pointer;
        int overflow = count - remain;
        // write remain bytes
		memcpy(file->write_buf + file->write_pointer, buf, remain);
		file->write_pointer = BUFFER_SIZE;

		if ((bytes_wrote = myflush(file)) == -1)
			return -1;
        if (file->previous == READ){
        	file->read_pointer += bytes_wrote;
        }
		file->write_pointer = overflow;

        // write over flow bytes
		memcpy(file->write_buf, (char*)buf + remain, overflow);
	} else {
		memcpy(file->write_buf + file->write_pointer, buf, count);	
		file->write_pointer += count;
	}

    file->previous = NOTREAD;
	return count;
}


/**
 * @brief repositions the file offset of the file according to flag
 * 
 * @param file file to reposition pointer
 * @return int resulting file offset
 */
int myseek(struct file_info *file, off_t offset, int whence){
	int new_offset;
	file->read_pointer = 0;

	if (whence == SEEK_SET){
		new_offset = lseek(file->fd, offset, SEEK_SET);
	} else {
		new_offset = lseek(file->fd, file->file_offset + offset, SEEK_SET);
	}

	file->file_offset = new_offset;
	return new_offset;
}


/**
 * @brief force write of all buffered write data
 * 
 * @param file file to flush from
 * @return int bytes flushed
 */
int myflush(struct file_info *file){
	int bytes_wrote = 0;
	// save current actual offset
	int temp_offset = lseek(file->fd, 0, SEEK_CUR);

	// move to internal offset
	lseek(file->fd, file->file_offset, SEEK_SET);

	bytes_wrote = write(file->fd, file->write_buf, file->write_pointer);
	file->file_offset += bytes_wrote;

	// move back to actual offset
    lseek(file->fd, temp_offset, SEEK_SET);

	file->write_pointer = 0;
	return bytes_wrote;
}


int __write_after_read(struct file_info *file, char *buf, size_t count){
    int bytes_wrote = 0;

    memcpy(file->write_buf, buf, count);
    file->write_pointer += count;

	if ((bytes_wrote = myflush(file)) == -1)
		return -1;
	file->read_pointer += bytes_wrote;

    return bytes_wrote;
}

