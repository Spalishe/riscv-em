#pragma once

#include "mmio.h"
#include "plic.hpp"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <mutex>

static constexpr uint32_t VIRTIO_MAGIC = 0x74726976; // 'virt'
static constexpr uint32_t VIRTIO_MMIO_VERSION_1 = 1;
static constexpr uint32_t VIRTIO_MMIO_VERSION_2 = 2; // often used for modern virtio
static constexpr uint32_t VIRTIO_DEVICE_BLOCK = 2;

static constexpr uint32_t VIRTIO_BLK_T_IN  = 0; // read
static constexpr uint32_t VIRTIO_BLK_T_OUT = 1; // write

static constexpr uint8_t VIRTIO_BLK_S_OK     = 0;
static constexpr uint8_t VIRTIO_BLK_S_IOERR  = 1;
static constexpr uint8_t VIRTIO_BLK_S_UNSUPP = 2;

static constexpr size_t SECTOR_SIZE = 512;

struct VirtqDesc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};
struct VirtioBlkReq {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
};

class VirtioBlkDevice : public Device {
public:
    VirtioBlkDevice(uint64_t base, uint64_t size, DRAM& ram, PLIC* plic, fdt_node* fdt, uint8_t irq_number, const std::string& image_path);

    ~VirtioBlkDevice() {
        if (disk.is_open()) disk.close();
    }

    uint64_t read(HART* hart,uint64_t addr,uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size,uint64_t val);

private:
    // Internal state
    std::string disk_path;
    std::fstream disk;
    std::mutex mtx;

    uint32_t device_status;
    uint64_t device_features;
    uint32_t device_features_sel = 0;
    uint64_t driver_features;
    uint32_t driver_features_sel = 0;

    uint32_t queue_sel;
    uint16_t queue_num;
    uint16_t queue_size;
    uint64_t queue_desc_addr;
    uint64_t queue_driver_addr;
    uint64_t queue_device_addr;
    bool     queue_ready;

    uint32_t interrupt_status;
    uint32_t config_generation;
    uint64_t blk_config_capacity; // in 512-byte sectors

    uint16_t last_used_idx;

    uint8_t irq_number;
    PLIC* plic;

    uint32_t queue_max_size() const {
        // provide a reasonable default
        return 1024;
    }

    // Reset internal data (on driver reset)
    void reset_device();
    uint64_t read_config(uint64_t off, uint64_t sz);
    void write_config(uint64_t off, uint64_t sz, uint64_t value); 

    // Helpers to access memory
    template<typename T>
    T h_read_u(uint64_t phys_addr);
    VirtqDesc h_read_desc(uint64_t phys_addr);
    void h_write_u(uint64_t phys_addr, const void* buf, size_t len);

    void trigger_interrupt(PLIC* plic);
    void process_queue();

    void h_read_into(uint64_t phys_addr, void* out, size_t n);
    template<typename T>
    T h_read_u_simple(uint64_t phys_addr);
}; 
