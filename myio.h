#ifndef __MYIO_H
#define __MYIO_H

#include <stddef.h>
#include <sys/types.h>

struct file_info* myopen(char *path, int flags);
int myclose(struct file_info *file);
int myread(struct file_info *file, void *buf, size_t count);
int mywrite(struct file_info *file, void *buf, size_t count);
int myseek(struct file_info *file, off_t offset, int whence);
int myflush(struct file_info *file);

#endif /*__MYIO_H */
