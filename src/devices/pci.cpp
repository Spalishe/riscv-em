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

#include "../include/devices/pci.hpp"

PCI::PCI(uint64_t base, uint64_t size, DRAM& ram,fdt_node* fdt)
        : Device(base, size, ram)
    {
        if(fdt != NULL) {
            struct fdt_node* pci_fdt = fdt_node_create_reg("pci", base);
            fdt_node_add_prop_reg(pci_fdt, "reg", base, size);
            fdt_node_add_prop_str(pci_fdt, "compatible", "pci-host-ecam-generic");
            fdt_node_add_prop_str(pci_fdt, "device_type", "pci");
            fdt_node_add_prop(pci_fdt, "dma-coherent", NULL, 0);
            fdt_node_add_prop_cells(pci_fdt, "bus-range", std::vector<uint32_t>{0x00, 0xFF}, 2);
            fdt_node_add_prop_u32(pci_fdt,"#address-cells",3);
            fdt_node_add_prop_u32(pci_fdt,"#size-cells",2);
            fdt_node_add_prop_u32(pci_fdt,"#interrupt-cells",1);

            std::vector<uint32_t> pci_ranges = {
                0x00, 0x00000000,       // parent addr (2 words)
                0x00, 0x00000000, 0x00, // child addr (3 words)
                0x00, 0x1000000          // size (2 words)
            };
            fdt_node_add_prop_cells(pci_fdt, "ranges", pci_ranges, pci_ranges.size());

            uint32_t plich = fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"soc"),"plic",0x0C000000));
            std::vector<uint32_t> pci_interrupt_map = {
                0, 0, 0, 1,   // PCI: bus/devfn/int
                plich, 1,      // PLIC phandle & hwirq
                0,0,0,0,       // next device
                plich, 2
            };
            fdt_node_add_prop_cells(pci_fdt, "interrupt-map", pci_interrupt_map, pci_interrupt_map.size());

            fdt_node_add_prop_cells(pci_fdt, "interrupt-map-mask", std::vector<uint32_t>{0x1800, 0, 0, 7}, 4);

            fdt_node_add_child(fdt_node_find(fdt,"soc"), pci_fdt);
        }
    }

uint64_t PCI::read(HART* hart,uint64_t addr, uint64_t size) {
    return dram_load(&ram,addr,size);
}

void PCI::write(HART* hart, uint64_t addr, uint64_t size, uint64_t val) {
    dram_store(&ram,addr,size,val);
}