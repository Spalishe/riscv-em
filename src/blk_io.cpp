#pragma once

#include "../include/blk_io.hpp"
#include <cstdint>
#include <iostream>

#define RVFILE_MAX_BUFF    0x10000000U
#define RVFILE_LEGAL_FLAGS 0x3F

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define O_CLOEXEC 0
#define O_NOATIME 0
#define O_CREAT 0
#define O_EXCL 0
#define O_TRUNC 0
#define O_DIRECT 0
#define O_SYNC 0
#define O_DSYNC O_SYNC

#define F_OFD_SETLK F_SETLK

static bool blk_io_rvfilery_lock_fd(int fd)
{
    struct flock flk = {
        .l_type   = F_WRLCK,
        .l_whence = SEEK_SET,
    };
    return !fcntl(fd, F_OFD_SETLK, &flk) || (errno != EACCES && errno != EAGAIN);
}

inline bool cas_uint64(void* ptr, void* expected, uint64_t desired) {
    uint64_t* value_ptr = (uint64_t*)ptr;
    uint64_t* expected_ptr = (uint64_t*)expected;
    
    uint64_t current = *value_ptr;
    
    if (current == *expected_ptr) {
        *value_ptr = desired;
        return true;
    } else {
        *expected_ptr = current;
        return false;
    }
}

inline void rvfile_grow_internal(blk_io_rvfile* file, uint64_t length)
{
    uint64_t file_size = 0;
    do {
        file_size = file->size;
        if (file_size >= length) {
            // File is already big enough
            break;
        }
    } while (!cas_uint64(&file->size, &file_size, length));
}

blk_io_rvfile* rvopen(std::string filepath, uint32_t filemode)
{
    int open_flags = O_CLOEXEC | O_NOATIME;
    if (filemode & RVFILE_WRITE) {
        if (filemode & RVFILE_READ) {
            open_flags |= O_RDWR;
        } else {
            open_flags |= O_WRONLY;
        }
        if (filemode & RVFILE_CREAT) {
            open_flags |= O_CREAT;
            if (filemode & RVFILE_EXCL) {
                open_flags |= O_EXCL;
            }
        }
        if (filemode & blk_io_rvfileRUNC) {
            open_flags |= O_TRUNC;
        }
        if (filemode & RVFILE_SYNC) {
            open_flags |= O_DSYNC;
        }
    } else {
        open_flags |= O_RDONLY;
    }
    if (filemode & RVFILE_DIRECT) {
        open_flags |= O_DIRECT;
    }

    int fd = open(filepath.c_str(), open_flags, 0600);
    if (fd < 0) {
        std::cerr << ("Failed to open \"%s\"", filepath);
        return NULL;
    }

    if ((filemode & RVFILE_EXCL) && !blk_io_rvfilery_lock_fd(fd)) {
        std::cerr << ("Failed to open \"%s\": File is busy", filepath);
        close(fd);
        return NULL;
    }

    uint64_t size = lseek(fd, 0, SEEK_END);
    if (size >= ((((uint64_t)1) << 63) - 1)) {
        std::cerr << ("Failed to open \"%s\": File is not seekable", filepath);
        close(fd);
        return NULL;
    }

    blk_io_rvfile* file = new blk_io_rvfile(filepath,filemode);
    file->size     = size;
    file->fd       = fd;

    if ((filemode & RVFILE_RW) && (filemode & blk_io_rvfileRUNC) && file->rvfilesize()) {
        // Handle blk_io_rvfileRUNC if failed natively
        if (!file->rvtruncate(0)) {
            file->rvclose();
            return NULL;
        }
    }

    return file;
}

void blk_io_rvfile::rvclose()
{
    rvfsync();
    close(fd);
    free(this);
}

uint64_t blk_io_rvfile::rvfilesize()
{
    return size;
}

static int32_t rvread_chunk(blk_io_rvfile* file, void* dst, size_t size, uint64_t offset)
{
    int32_t ret = pread(file->fd, dst, size, offset);
    if (ret < 0 && errno != EINTR) {
        // IO error
        return 0;
    }
    return ret;
}

static int32_t rvread_unlocked(blk_io_rvfile* file, void* dst, size_t size, uint64_t offset)
{
    uint8_t* buffer = (uint8_t*)dst;
    size_t   ret    = 0;

    while (ret < size) {
        size_t  chunk_size = (size - ret < RVFILE_MAX_BUFF) ? (size - ret) : (RVFILE_MAX_BUFF);
        int32_t tmp        = rvread_chunk(file, buffer + ret, chunk_size, offset + ret);
        if (tmp > 0) {
            ret += tmp;
            if (ret < size && offset + ret >= file->rvfilesize()) {
                // End of file reached
                break;
            }
        } else if (!tmp) {
            // IO error
            break;
        }
    }

    return ret;
}

size_t blk_io_rvfile::rvread(void* dst, size_t size, uint64_t offset)
{
    size_t ret = 0;

    uint64_t pos = (offset == RVFILE_POSITION) ? rvtell() : offset;
    ret = rvread_unlocked(this, dst, size, pos);
    if (offset == RVFILE_POSITION && ret) {
        rvseek(pos + ret, RVFILE_SEEK_SET);
    }

    return ret;
}

static int32_t rvwrite_chunk(blk_io_rvfile* file, const void* src, size_t size, uint64_t offset)
{
    int32_t ret = pwrite(file->fd, src, size, offset);
    if (ret < 0 && errno != EINTR) {
        // IO error
        return 0;
    }
    return ret;
}

// Unlocked version of rvwrite(), requires a valid file offset
static int32_t rvwrite_unlocked(blk_io_rvfile* file, const void* src, size_t size, uint64_t offset)
{
    const uint8_t* buffer = (uint8_t*)src;
    size_t         ret    = 0;

    while (ret < size) {
        size_t  chunk_size = (size - ret < RVFILE_MAX_BUFF) ? (size - ret) : RVFILE_MAX_BUFF;
        int32_t tmp        = rvwrite_chunk(file, buffer + ret, chunk_size, offset + ret);
        if (tmp > 0) {
            ret += tmp;
        } else if (!tmp) {
            // IO error
            break;
        }
    }

    rvfile_grow_internal(file, offset + ret);

    return ret;
}

size_t blk_io_rvfile::rvwrite(const void* src, size_t size, uint64_t offset)
{
    size_t ret = 0;

    uint64_t pos = (offset == RVFILE_POSITION) ? rvtell() : offset;
    ret = rvwrite_unlocked(this, src, size, offset);
    if (offset == RVFILE_POSITION && ret) {
        rvseek(pos + ret, RVFILE_SEEK_SET);
    }

    return ret;
}

bool blk_io_rvfile::rvtrim(uint64_t offset, uint64_t size)
{
    return !fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, offset, size);
}

bool blk_io_rvfile::rvseek(int64_t offset, uint32_t startpos)
{
    if (startpos == RVFILE_SEEK_CUR) {
        offset = rvtell() + offset;
    } else if (startpos == RVFILE_SEEK_END) {
        offset = rvfilesize() - offset;
    } else if (startpos != RVFILE_SEEK_SET) {
        // Invalid seek operation
        offset = -1;
    }
    if (offset >= 0) {
        pos = offset;
        return true;
    }
    return false;
}

uint64_t blk_io_rvfile::rvtell()
{
    return pos;
}

bool blk_io_rvfile::rvfsync()
{
    bool ret = false;
    // Spin on fdatasync() / fsync() for a few times in case of EINTR
    for (size_t i = 0; i < 10 && !ret; ++i) {
        ret = !fdatasync(fd);
        if (!ret && errno != EINTR) {
            break;
        }
    }
    return ret;
}

bool blk_io_rvfile::rvtruncate(uint64_t length)
{
    bool resized = false;
    if (length == rvfilesize()) {
        return true;
    }
    resized = !ftruncate(fd, length);
    if (resized) {
        size = length;
        return true;
    } else if (length >= rvfilesize()) {
        return rvfallocate(length);
    }
    return false;
}

static bool rvfile_grow_unlocked(blk_io_rvfile* file, uint64_t length)
{
    char tmp = 0;
    if (rvread_unlocked(file, &tmp, 1, length - 1)) {
        // File already big enough
        return true;
    }
    return !!rvwrite_unlocked(file, &tmp, 1, length - 1);
}

static bool rvfile_grow_generic(blk_io_rvfile* file, uint64_t length)
{
    bool ret = true;
    if (length > file->rvfilesize()) {
        // Grow the file by writing one byte at the new end
        // NOTE: This is not thread safe whenever there are writers beyond the end of file
        // This only applies to POSIX implementations, where posix_fallocate() is preferable
        ret = rvfile_grow_unlocked(file, length);
    }
    return ret;
}

bool blk_io_rvfile::rvfallocate(uint64_t length)
{
    if (length <= rvfilesize()) {
        return true;
    }
    // Try to grow file via posix_fallocate() (Available on Linux 2.6.23+, FreeBSD 9+, NetBSD 7+)
    if (!posix_fallocate(fd, length - 1, 1)) {
        rvfile_grow_internal(this,length);
        return true;
    }

    return rvfile_grow_generic(this,length);
}

static void blk_raw_close(void* dev)
{
    blk_io_rvfile* file = (blk_io_rvfile*)dev;
    file->rvclose();
}

static size_t blk_raw_read(void* dev, void* dst, size_t size, uint64_t offset)
{
    blk_io_rvfile* file = (blk_io_rvfile*)dev;
    return file->rvread(dst, size, offset);
}

static size_t blk_raw_write(void* dev, const void* src, size_t size, uint64_t offset)
{
    blk_io_rvfile* file = (blk_io_rvfile*)dev;
    return file->rvwrite(src, size, offset);
}

static bool blk_raw_trim(void* dev, uint64_t offset, uint64_t size)
{
    blk_io_rvfile* file = (blk_io_rvfile*)dev;
    return file->rvtrim(offset, size);
}

// Raw block device implementation
// Be careful with function prototypes
static const blkdev_type_t blkdev_type_raw = {
    .name  = "blk-raw",
    .close = blk_raw_close,
    .read  = blk_raw_read,
    .write = blk_raw_write,
    .trim  = blk_raw_trim,
};

static blkdev_t* blk_raw_open(std::string filename, uint32_t filemode)
{
    blk_io_rvfile* file = rvopen(filename, filemode);
    if (!file) {
        return NULL;
    }
    blkdev_t* dev = new blkdev_t(filename,filemode);
    dev->type     = &blkdev_type_raw;
    dev->size     = file->rvfilesize();
    dev->data     = file;
    return dev;
}

blkdev_t* blk_open(const char* filename, uint32_t opts)
{
    uint32_t filemode = opts | ((opts & BLKDEV_WRITE) ? RVFILE_EXCL : 0);
    return blk_raw_open(filename, filemode);
}

void blkdev_t::blk_close()
{
    type->close(data);
    free(this);
}
