/*
Copyright 2025 Spalishe

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#pragma once

#include "mmio.h"
#include "plic.hpp"
#include "../libfdt.hpp"

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <optional>
#include <cstring>

// Register offsets (MMIO)
#define VIRT_REG_MAGICVALUE 0x000
#define VIRT_REG_VERSION 0x004
#define VIRT_REG_DEVICEID 0x008
#define VIRT_REG_VENDORID 0x00C
#define VIRT_REG_DEVICEFEATURES 0x010
#define VIRT_REG_DEVICEFEATURESSEL 0x014
#define VIRT_REG_DRIVERFEATURES 0x020
#define VIRT_REG_DRIVERFEATURESSEL 0x024
#define VIRT_REG_QUEUESEL 0x030
#define VIRT_REG_QUEUENUMMAX 0x034
#define VIRT_REG_QUEUENUM 0x038
#define VIRT_REG_QUEUEREADY 0x044
#define VIRT_REG_QUEUENOTIFY 0x050
#define VIRT_REG_INTERRUPTSTATUS 0x060
#define VIRT_REG_INTERRUPTACK 0x064
#define VIRT_REG_STATUS 0x070
#define VIRT_REG_QUEUEDESCLOW 0x080
#define VIRT_REG_QUEUEDESCHIGH 0x084
#define VIRT_REG_QUEUEDRIVERLOW 0x090
#define VIRT_REG_QUEUEDRIVERHIGH 0x094
#define VIRT_REG_QUEUEDEVICELOW 0x0A0
#define VIRT_REG_QUEUEDEVICEHIGH 0x0A4
#define VIRT_REG_CONFIGGENERATION 0x0fc
#define VIRT_REG_CONFIG 0x100

// Status bits
#define VIRT_STATUS_ACKNOWLEDGE  0x01
#define VIRT_STATUS_DRIVER        0x02
#define VIRT_STATUS_FEATURES_OK   0x08
#define VIRT_STATUS_DRIVER_OK     0x04
#define VIRT_STATUS_DEVICE_NEEDS_RESET 0x40
#define VIRT_STATUS_FAILED        0x80

// Descriptor flags
static const uint16_t VIRTQ_DESC_F_NEXT  = 1;
static const uint16_t VIRTQ_DESC_F_WRITE = 2;
static const uint16_t VIRTQ_DESC_F_INDIRECT = 4;

// Virtio-block request types (basic)
static const uint32_t VIRTIO_BLK_T_IN  = 0; // read from device into guest
static const uint32_t VIRTIO_BLK_T_OUT = 1; // write from guest into device
static const uint32_t VIRTIO_BLK_S_OK  = 0;
static const uint32_t VIRTIO_BLK_S_IOERR = 1;

// Sizes
static constexpr uint32_t VIRTQ_DESC_SIZE = 16; // sizeof(desc) in spec (addr:8,len:4,flags:2,next:2)
static constexpr uint32_t SECTOR_SIZE = 512;

// Forward declarations for types from your codebase
struct HART;
struct Device;
struct DRAM;

// Descriptor layout (guest memory)
struct VirtqDesc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

// Avail ring header
struct VirtqAvail {
    uint16_t flags;
    uint16_t idx;
    // followed by uint16_t ring[]
    // optionally uint16_t used_event;
} __attribute__((packed));

// Used ring element
struct VirtqUsedElem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

// Used ring header
struct VirtqUsed {
    uint16_t flags;
    uint16_t idx;
    // followed by VirtqUsedElem ring[]
    // optionally uint16_t avail_event;
} __attribute__((packed));

// Block request header (guest memory)
struct VirtioBlkReq {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
} __attribute__((packed));

struct VirtQueueState {
    uint32_t size = 0;                // queue size (QueueNum)
    uint64_t desc_addr = 0;          // guest physical address of descriptor table
    uint64_t avail_addr = 0;         // guest physical address of avail ring
    uint64_t used_addr = 0;          // guest physical address of used ring
    uint16_t last_avail_idx = 0;     // cached last seen avail.idx
    uint16_t last_used_idx = 0;      // cached last seen used.idx
    bool ready = false;
};

struct VirtIO_BLK : public Device {
    VirtIO_BLK(uint64_t base, uint64_t size, DRAM& ram, PLIC* plic, fdt_node* fdt, uint8_t irq_num, std::string image_path);

    uint64_t read(HART* hart, uint64_t addr, uint64_t size) override;
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t val) override;

private:
    // helpers to access guest memory
    uint8_t read_u8(uint64_t gpa);
    uint16_t read_u16(uint64_t gpa);
    uint32_t read_u32(uint64_t gpa);
    uint64_t read_u64(uint64_t gpa);
    void write_u8(uint64_t gpa, uint8_t v);
    void write_u16(uint64_t gpa, uint16_t v);
    void write_u32(uint64_t gpa, uint32_t v);
    void write_u64(uint64_t gpa, uint64_t v);
    void copy_from_dram(uint64_t gpa, void* dst, uint64_t len);
    void copy_to_dram(uint64_t gpa, const void* src, uint64_t len);

    // queue/descriptor processing
    void process_queue(uint32_t qsel);
    bool fetch_descriptor_chain(uint16_t head, std::vector<VirtqDesc>& out_chain, const VirtQueueState& q);

    // disk image operations
    bool disk_read(uint64_t sector, void* buf, size_t bytes);
    bool disk_write(uint64_t sector, const void* buf, size_t bytes);

    // utilities
    void raise_interrupt();
    void clear_interrupt(uint32_t bits);

private:
    DRAM& ram;           // reference to memory
    PLIC* plic;
    uint8_t irq_num;
    std::string image_path;
    std::fstream disk;   // image file

    // device state
    uint64_t device_features = 0;
    uint64_t driver_features = 0;
    uint32_t device_features_sel = 0;
    uint32_t driver_features_sel = 0;
    uint32_t device_status = 0;
    uint32_t interrupt_status = 0;

    uint32_t queue_sel = 0;
    VirtQueueState queue0;

    uint32_t config_generation = 0;
    uint64_t capacity_sectors = 0; // number of 512-byte sectors
};
