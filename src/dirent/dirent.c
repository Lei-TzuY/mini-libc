#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#define MINI_DIRENT_BUFFER_SIZE 4096U
#define MINI_LINUX_DIRENT_NAME_OFFSET 19U

struct mini_linux_dirent64 {
    unsigned long long d_ino;
    long long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1];
};

struct __mini_DIR {
    int fd;
    size_t position;
    size_t length;
    unsigned char buffer[MINI_DIRENT_BUFFER_SIZE];
    struct dirent current;
};

static int copy_name(char *destination, size_t capacity,
                     const char *source, size_t available)
{
    size_t index;

    if (capacity == 0U) {
        return 0;
    }

    for (index = 0U; index < available; ++index) {
        if (source[index] == '\0') {
            if (index >= capacity) {
                return 0;
            }
            destination[index] = '\0';
            return 1;
        }
        if (index + 1U >= capacity) {
            return 0;
        }
        destination[index] = source[index];
    }
    return 0;
}

DIR *opendir(const char *name)
{
    DIR *directory;
    int fd;
    int saved_errno;

    if (name == (const char *)0) {
        errno = EINVAL;
        return (DIR *)0;
    }

    fd = open(name, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) {
        return (DIR *)0;
    }

    directory = (DIR *)malloc(sizeof(DIR));
    if (directory == (DIR *)0) {
        saved_errno = errno;
        (void)close(fd);
        errno = saved_errno;
        return (DIR *)0;
    }

    directory->fd = fd;
    directory->position = 0U;
    directory->length = 0U;
    return directory;
}

struct dirent *readdir(DIR *dirp)
{
    struct mini_linux_dirent64 *raw;
    long result;
    size_t remaining;
    size_t name_available;

    if (dirp == (DIR *)0) {
        errno = EINVAL;
        return (struct dirent *)0;
    }

    for (;;) {
        if (dirp->position >= dirp->length) {
            result = mini_sys_getdents64(
                dirp->fd, dirp->buffer,
                (unsigned long)sizeof(dirp->buffer));
            if (result < 0L) {
                errno = (int)-result;
                return (struct dirent *)0;
            }
            if (result == 0L) {
                return (struct dirent *)0;
            }
            dirp->position = 0U;
            dirp->length = (size_t)result;
        }

        remaining = dirp->length - dirp->position;
        if (remaining < MINI_LINUX_DIRENT_NAME_OFFSET + 1U) {
            errno = EIO;
            return (struct dirent *)0;
        }

        raw = (struct mini_linux_dirent64 *)
            (void *)(dirp->buffer + dirp->position);
        if ((size_t)raw->d_reclen < MINI_LINUX_DIRENT_NAME_OFFSET + 1U ||
            (size_t)raw->d_reclen > remaining) {
            errno = EIO;
            return (struct dirent *)0;
        }

        name_available =
            (size_t)raw->d_reclen - MINI_LINUX_DIRENT_NAME_OFFSET;
        if (!copy_name(dirp->current.d_name,
                       sizeof(dirp->current.d_name),
                       raw->d_name, name_available)) {
            errno = EIO;
            return (struct dirent *)0;
        }

        dirp->current.d_ino = (ino_t)raw->d_ino;
        dirp->current.d_off = (off_t)raw->d_off;
        dirp->current.d_reclen = raw->d_reclen;
        dirp->current.d_type = raw->d_type;
        dirp->position += (size_t)raw->d_reclen;
        return &dirp->current;
    }
}

int closedir(DIR *dirp)
{
    int fd;

    if (dirp == (DIR *)0) {
        errno = EINVAL;
        return -1;
    }

    fd = dirp->fd;
    free(dirp);
    return close(fd);
}

void rewinddir(DIR *dirp)
{
    long result;
    int saved_errno;

    if (dirp == (DIR *)0) {
        errno = EINVAL;
        return;
    }

    saved_errno = errno;
    result = mini_sys_lseek(dirp->fd, 0L, SEEK_SET);
    if (result < 0L) {
        errno = (int)-result;
        return;
    }

    dirp->position = 0U;
    dirp->length = 0U;
    errno = saved_errno;
}

int dirfd(DIR *dirp)
{
    if (dirp == (DIR *)0) {
        errno = EINVAL;
        return -1;
    }
    return dirp->fd;
}
