#include "../include/devices/virtio.hpp"

VirtioBlkDevice::VirtioBlkDevice(uint64_t base, uint64_t size, DRAM& ram, PLIC* plic, fdt_node* fdt, uint8_t irq_number, const std::string& image_path)
    : Device(base, size, ram),
        disk_path(image_path),
        device_status(0),
        device_features(0),
        driver_features(0),
        queue_sel(0),
        queue_num(0),
        queue_size(0),
        queue_desc_addr(0),
        queue_driver_addr(0),
        queue_device_addr(0),
        queue_ready(false),
        interrupt_status(0),
        config_generation(0),
        last_used_idx(0)
{

    if(fdt != NULL) {
        struct fdt_node* virtio_fdt = fdt_node_create_reg("virtio", base);
        fdt_node_add_prop_reg(virtio_fdt, "reg", base, size);
        fdt_node_add_prop_str(virtio_fdt, "compatible", "virtio,mmio");
        fdt_node_add_prop_str(virtio_fdt, "status", "okay");
        fdt_node_add_prop_u32(virtio_fdt, "interrupt-parent", fdt_node_get_phandle(fdt_node_find(fdt_node_find(fdt,"soc"),"plic")));
        fdt_node_add_prop_u32(virtio_fdt, "interrupts", irq_number);

        fdt_node_add_child(fdt_node_find(fdt,"soc"), virtio_fdt);
    }

    // open disk image
    disk.open(disk_path, std::ios::binary | std::ios::in | std::ios::out);
    std::cout << "Reading disk image: " << disk_path << std::endl;
    if (!disk.is_open()) {
        disk.clear();
        disk.open(disk_path, std::ios::binary | std::ios::out);
        disk.close();
        disk.open(disk_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!disk.is_open()) {
            throw std::runtime_error("Cannot open disk image: " + disk_path);
        }
    }

    device_features = 0; // set bits as needed

    disk.seekg(0, std::ios::end);
    uint64_t bytes = disk.tellg();
    uint64_t sectors = bytes / SECTOR_SIZE;
    blk_config_capacity = sectors;
}

uint64_t VirtioBlkDevice::read(HART* hart, uint64_t addr, uint64_t size) {
    uint64_t off = addr - base;
    std::lock_guard<std::mutex> lk(mtx);

    switch (off) {
        case 0x000: // MagicValue (32-bit)
            return VIRTIO_MAGIC;
        case 0x004: // Version
            return VIRTIO_MMIO_VERSION_2; // present as modern virtio
        case 0x008: // DeviceID
            return VIRTIO_DEVICE_BLOCK;
        case 0x00c: // VendorID
            return 0x554d4551; // example vendor
        case 0x010: // DeviceFeatures (low)
            return static_cast<uint32_t>(device_features & 0xffffffffu);
        case 0x014: // DeviceFeaturesSel (read returns current)
            return device_features_sel;
        case 0x020: // DriverFeatures (write-only) - return driver's selected (we keep)
            return static_cast<uint32_t>(driver_features & 0xffffffffu);
        case 0x024:
            return driver_features_sel;
        case 0x030: // QueueSel
            return queue_sel;
        case 0x034: // QueueNumMax
            return queue_max_size();
        case 0x038: // QueueNum
            return queue_num;
        case 0x044: // QueueReady
            return queue_ready ? 1 : 0;
        case 0x060: // InterruptStatus (read)
            return interrupt_status;
        case 0x064: // InterruptACK is write-only
            return 0;
        case 0x070: // Status
            return device_status;
        case 0x0fc: // ConfigGeneration
            return config_generation;
        default:
            break;
    }

    if (off >= 0x100) {
        uint64_t coff = off - 0x100;
        return read_config(coff, size);
    }

    return 0;
}

void VirtioBlkDevice::write(HART* hart, uint64_t addr, uint64_t size, uint64_t val) {
    uint64_t off = addr - base;
    std::lock_guard<std::mutex> lk(mtx);

    switch (off) {
        case 0x014: // DeviceFeaturesSel (driver sets index to read features)
            device_features_sel = static_cast<uint32_t>(val & 0xffffffffu);
            return;
        case 0x010: // DeviceFeatures (read-only)
            return;
        case 0x020: // DriverFeatures (low)
            if (driver_features_sel == 0) {
                driver_features = (driver_features & (~0xffffffffull)) | (val & 0xffffffffull);
            } else {
                driver_features = (driver_features & 0xffffffffull) | ((uint64_t)(val) << 32);
            }
            return;
        case 0x024: // DriverFeaturesSel
            driver_features_sel = static_cast<uint32_t>(val & 0xffffffffu);
            return;
        case 0x030: // QueueSel
            queue_sel = static_cast<uint32_t>(val & 0xffffffffu);
            return;
        case 0x038: // QueueNum
            queue_num = static_cast<uint16_t>(val & 0xffffu);
            queue_size = queue_num;
            return;
        case 0x044: // QueueReady
            queue_ready = (val & 1);
            return;
        case 0x080: // QueueDescLow
            queue_desc_addr = (queue_desc_addr & 0xffffffff00000000ull) | (val & 0xffffffffull);
            return;
        case 0x084: // QueueDescHigh
            queue_desc_addr = (queue_desc_addr & 0xffffffffull) | ((uint64_t)val << 32);
            return;
        case 0x090: // QueueDriverLow
            queue_driver_addr = (queue_driver_addr & 0xffffffff00000000ull) | (val & 0xffffffffull);
            return;
        case 0x094: // QueueDriverHigh
            queue_driver_addr = (queue_driver_addr & 0xffffffffull) | ((uint64_t)val << 32);
            return;
        case 0x0a0: // QueueDeviceLow
            queue_device_addr = (queue_device_addr & 0xffffffff00000000ull) | (val & 0xffffffffull);
            return;
        case 0x0a4: // QueueDeviceHigh
            queue_device_addr = (queue_device_addr & 0xffffffffull) | ((uint64_t)val << 32);
            return;
        case 0x050: // QueueNotify (driver tells device to process queue)
            {
                uint32_t qsel = static_cast<uint32_t>(val & 0xffffffffu);
                // For our device we only have queue 0
                if (qsel == 0) {
                    // process available buffers
                    process_queue();
                }
            }
            return;
        case 0x064: // InterruptACK - driver writes bits to clear
            interrupt_status &= ~static_cast<uint32_t>(val & 0xffffffffu);
            return;
        case 0x070: // Status
            device_status = static_cast<uint32_t>(val & 0xffffffffu);
            if (device_status == 0) {
                reset_device();
            }
            return;
        default:
            break;
    }

    // Config writes
    if (off >= 0x100) {
        uint64_t coff = off - 0x100;
        write_config(coff, size, val);
        return;
    }
}


// Reset internal data (on driver reset)
void VirtioBlkDevice::reset_device() {
    // minimal reset behavior
    queue_sel = 0;
    queue_num = 0;
    queue_desc_addr = 0;
    queue_driver_addr = 0;
    queue_device_addr = 0;
    queue_ready = false;
    interrupt_status = 0;
    device_status = 0;
    // do not truncate disk
}

uint64_t VirtioBlkDevice::read_config(uint64_t off, uint64_t sz) {
    if (off == 0 && sz == 8) {
        return blk_config_capacity;
    }
    return 0;
}

void VirtioBlkDevice::write_config(uint64_t off, uint64_t sz, uint64_t value) {
    //ignore writes
    (void)off; (void)sz; (void)value;
}

template<typename T>
T VirtioBlkDevice::h_read_u(uint64_t phys_addr) {
    return (T)dram_load(&ram,phys_addr,sizeof(T));
}

VirtqDesc VirtioBlkDevice::h_read_desc(uint64_t phys_addr) {
    VirtqDesc d;
    d.addr = dram_load(&ram,phys_addr,64);
    d.len = dram_load(&ram,phys_addr+4,32);
    d.flags = dram_load(&ram,phys_addr+6,16);
    d.next = dram_load(&ram,phys_addr+8,16);
    return d;
}

void VirtioBlkDevice::h_write_u(uint64_t phys_addr, const void* buf, size_t len) {
    const uint8_t* buf1 = reinterpret_cast<const uint8_t*>(buf);
    for(int i=0; i < len; i++)
        dram_store(&ram,phys_addr, (buf1)[i], 8);
}

void VirtioBlkDevice::trigger_interrupt(PLIC* plic) {
    interrupt_status |= 1;
    if (plic) {
        plic->raise(irq_number);
    }
}

void VirtioBlkDevice::process_queue() {
    if (!queue_ready) return;
    if (queue_desc_addr == 0 || queue_driver_addr == 0 || queue_device_addr == 0) return;
    if (queue_num == 0) return;

    uint16_t qsz = queue_size;
    uint64_t desc_base = queue_desc_addr;
    uint64_t avail_base = queue_driver_addr;
    uint64_t used_base = queue_device_addr;

    // Read avail.idx
    uint16_t avail_idx = 0;
    // avail layout: flags(2), idx(2), ring[qsz], ...
    // idx is at offset 2
    avail_idx = h_read_u<uint16_t>(avail_base + 2);

    // We'll iterate from last_used_idx until avail_idx
    while (last_used_idx != avail_idx) {
        uint32_t ring_idx_addr = static_cast<uint32_t>(avail_base + 4 + (last_used_idx % qsz) * sizeof(uint16_t));
        uint16_t desc_idx = h_read_u<uint16_t>(ring_idx_addr);

        // read descriptor 0
        VirtqDesc d0 = h_read_desc(desc_base + desc_idx * sizeof(VirtqDesc));
        // read request header from d0.addr
        VirtioBlkReq req;
        if (d0.len < sizeof(VirtioBlkReq)) {
            last_used_idx++;
            continue;
        }
        // read header
        h_read_into(d0.addr, &req, sizeof(req));

        // follow chain for data and status
        uint16_t idx1 = d0.next;
        VirtqDesc d1 = h_read_desc(desc_base + idx1 * sizeof(VirtqDesc));
        uint32_t data_len = d1.len;

        uint16_t idx2 = d1.next;
        VirtqDesc d2 = h_read_desc(desc_base + idx2 * sizeof(VirtqDesc));
        // d2.len is expected to be >= 1 (status byte)
        uint8_t status = VIRTIO_BLK_S_OK;

        uint64_t offset = req.sector * SECTOR_SIZE;
        try {
            if (req.type == VIRTIO_BLK_T_OUT) {
                // write: read guest buffer then write to disk
                std::vector<uint8_t> buf(data_len);
                h_read_into(d1.addr, buf.data(), data_len);
                disk.seekp(offset);
                disk.write(reinterpret_cast<char*>(buf.data()), data_len);
                disk.flush();
            } else if (req.type == VIRTIO_BLK_T_IN) {
                // read from disk then write to guest buffer
                std::vector<uint8_t> buf(data_len, 0);
                disk.seekg(offset);
                disk.read(reinterpret_cast<char*>(buf.data()), data_len);
                h_write_u(d1.addr, buf.data(), data_len);
            } else {
                // unsupported command
                status = VIRTIO_BLK_S_UNSUPP;
            }
        } catch (...) {
            status = VIRTIO_BLK_S_IOERR;
        }

        // write status byte into d2.addr
        h_write_u(d2.addr, &status, 1u);

        // add to used ring: used_base layout: flags(2), idx(2), struct{uint32 id; uint32 len;} ring[qsz]
        uint16_t used_idx = h_read_u<uint16_t>(used_base + 2);
        uint64_t used_elem_addr = used_base + 4 + (used_idx % qsz) * 8; // each used elem 8 bytes
        // write id
        uint32_t id32 = desc_idx;
        h_write_u(used_elem_addr + 0, &id32, 4);
        uint32_t len32 = data_len; // length reported
        h_write_u(used_elem_addr + 4, &len32, 4);
        // increment used.idx
        uint16_t new_used_idx = used_idx + 1;
        h_write_u(used_base + 2, &new_used_idx, 2);

        last_used_idx++;
    }

    // update our local last_used_idx (wrap implicitly via 16-bit)
    trigger_interrupt(plic);
}

// Helper: read arbitrary bytes from memory
void VirtioBlkDevice::h_read_into(uint64_t phys_addr, void* out, size_t n) {
    uint8_t* buf = reinterpret_cast<uint8_t*>(out);
    for(int i=0;i < n;i++) {
        buf[i] = dram_load(&ram, phys_addr+i,8);
    }
}

template<typename T>
T VirtioBlkDevice::h_read_u_simple(uint64_t phys_addr) {
    T out;
    h_read_into(phys_addr, &out, sizeof(T));
    return out;
}