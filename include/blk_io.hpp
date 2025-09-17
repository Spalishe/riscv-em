#include <cstddef>
#include <cstdint>
#include <string>

#define RVFILE_READ     0x01 // Permit reading the file (Implicit without RVFILE_WRITE)
#define RVFILE_WRITE    0x02 // Permit writing the file
#define RVFILE_RW       0x03 // Open file in read/write mode
#define RVFILE_CREAT    0x04 // Create file if it doesn't exist (for RW only)
#define RVFILE_EXCL     0x08 // Prevent other processes from opening this file
#define blk_io_rvfileRUNC    0x10 // Truncate file conents upon opening (for RW only)
#define RVFILE_DIRECT   0x20 // Direct read/write DMA with the underlying storage
#define RVFILE_SYNC     0x40 // Disable writeback buffering

#define RVFILE_SEEK_SET 0x00 // Set file cursor position relative to file beginning
#define RVFILE_SEEK_CUR 0x01 // Set file cursor position relative to current position
#define RVFILE_SEEK_END 0x02 // Set file cursor position relative to file end

#define RVFILE_POSITION ((uint64_t)-1)

struct blk_io_rvfile {
    uint64_t size;
    uint64_t pos;
    int fd;

    blk_io_rvfile(std::string filepath, uint32_t filemode);

    void rvclose();
    uint64_t rvfilesize();
    size_t rvread(void* dst, size_t size, uint64_t offset);
    size_t rvwrite(const void* src, size_t size, uint64_t offset);
    bool rvseek(int64_t offset, uint32_t startpos);
    uint64_t rvtell();
    bool rvtrim(uint64_t offset, uint64_t size);
    bool rvtruncate(uint64_t length);
    bool rvfallocate(uint64_t length);

    bool rvfsync();
    int rvfile_get_fd();
};

#define BLKDEV_READ     RVFILE_READ
#define BLKDEV_WRITE    RVFILE_WRITE
#define BLKDEV_RW       RVFILE_RW

// Block device seeking
#define BLKDEV_SEEK_SET RVFILE_SEEK_SET
#define BLKDEV_SEEK_CUR RVFILE_SEEK_CUR
#define BLKDEV_SEEK_END RVFILE_SEEK_END

#define BLKDEV_POSITION RVFILE_POSITION

struct blkdev_type_t {
    const char* name;
    void        (*close)(void* dev);
    size_t      (*read)(void* dev, void* dst, size_t count, uint64_t offset);
    size_t      (*write)(void* dev, const void* src, size_t count, uint64_t offset);
    bool        (*trim)(void* dev, uint64_t offset, uint64_t count);
    bool        (*sync)(void* dev);
};

struct blkdev_t {
    void*                data;
    const blkdev_type_t* type;
    uint64_t             size;
    uint64_t             pos;

    blkdev_t(std::string filename, uint32_t opts);
    void blk_close();

    inline uint64_t blk_getsize()
    {
        return size;
    }
    inline uint64_t blk_tell()
    {
        return pos;
    }
    inline bool blk_seek(int64_t offset, uint32_t startpos)
    {
        if (startpos == BLKDEV_SEEK_CUR) {
            offset = blk_tell() + offset;
        } else if (startpos == BLKDEV_SEEK_END) {
            offset = blk_getsize() - offset;
        } else if (startpos != BLKDEV_SEEK_SET) {
            // Invalid seek operation
            offset = -1;
        }
        if (offset >= 0 && ((uint64_t)offset) < blk_getsize()) {
            pos = offset;
            return true;
        }
    }

    inline uint64_t blk_read(void* dst, uint64_t size, uint64_t offset)
    {
        if (type->read) {
            uint64_t pos = (offset == BLKDEV_POSITION) ? blk_tell() : offset;
            if (pos + size <= blk_getsize()) {
                size_t ret = type->read(data, dst, size, pos);
                if (offset == BLKDEV_POSITION) {
                    blk_seek(pos + ret, BLKDEV_SEEK_SET);
                }
                return ret;
            }
        }
        return 0;
    }

    inline uint64_t blk_write(const void* src, uint64_t size, uint64_t offset)
    {
        if (type->write) {
            uint64_t pos = (offset == BLKDEV_POSITION) ? blk_tell() : offset;
            if (pos + size <= blk_getsize()) {
                size_t ret = type->write(data, src, size, pos);
                if (offset == BLKDEV_POSITION) {
                    blk_seek(pos + ret, BLKDEV_SEEK_SET);
                }
                return ret;
            }
        }
        return 0;
    }

    inline bool blk_trim(uint64_t offset, uint64_t size)
    {
        if (type->trim) {
            uint64_t pos = (offset == BLKDEV_POSITION) ? blk_tell() : offset;
            if (pos + size <= size) {
                return type->trim(data, pos, size);
            }
        }
        return false;
    }

    inline bool blk_sync()
    {
        if (type->sync) {
            return type->sync(data);
        }
        return false;
    }
};
