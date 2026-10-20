#ifndef MIOS_SYS_STAT_H
#define MIOS_SYS_STAT_H

/*
 * MiOS - sys/stat.h (cabecera publica).
 *
 * struct stat es la que espera el kernel en newfstatat() para x86_64.
 */

#include "types.h"

struct stat {
    dev_t    st_dev;
    ino_t    st_ino;
    nlink_t  st_nlink;
    mode_t   st_mode;
    uid_t    st_uid;
    gid_t    st_gid;
    unsigned int __pad0;
    dev_t    st_rdev;
    off_t    st_size;
    long     st_blksize;
    long     st_blocks;
    time_t   st_atime;
    unsigned long st_atime_nsec;
    time_t   st_mtime;
    unsigned long st_mtime_nsec;
    time_t   st_ctime;
    unsigned long st_ctime_nsec;
    long     __unused[3];
};

#define S_IFMT  0170000
#define S_IFSOCK 0140000
#define S_IFLNK 0120000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFIFO 0010000

#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000
#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 0070
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IRWXO 0007
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001

int stat(const char *path, struct stat *st);
int fstat(int fd, struct stat *st);
int lstat(const char *path, struct stat *st);
int chmod(const char *path, mode_t mode);
int fchmod(int fd, mode_t mode);
int mkdir(const char *path, mode_t mode);
int umask(mode_t mask);

#endif /* MIOS_SYS_STAT_H */
