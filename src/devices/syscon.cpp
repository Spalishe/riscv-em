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

#include "../include/devices/syscon.hpp"
#include "../include/main.hpp"

SYSCON::SYSCON(uint64_t base, uint64_t size, DRAM& ram,fdt_node* fdt)
        : Device(base, size, ram)
    {
        if(fdt != NULL) {
            struct fdt_node* test_fdt = fdt_node_create_reg("test", base);
            fdt_node_add_prop_reg(test_fdt, "reg", base, size);
            fdt_node_add_prop(test_fdt, "compatible", "sifive,test1\0sifive,test0\0syscon\0",33);
            fdt_node_add_child(fdt_node_find(fdt,"soc"), test_fdt);

            fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"soc"),"test",base));

            
            struct fdt_node* poweroff_fdt = fdt_node_create("poweroff");
            fdt_node_add_prop_u32(poweroff_fdt, "value", 0x5555);
            fdt_node_add_prop_u32(poweroff_fdt, "offset", 0x0);
            fdt_node_add_prop_u32(poweroff_fdt, "regmap", fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"soc"),"test",base)));
            fdt_node_add_prop_str(poweroff_fdt, "compatible", "syscon-poweroff");
            fdt_node_add_child(fdt, poweroff_fdt);

            struct fdt_node* reboot_fdt = fdt_node_create("reboot");
            fdt_node_add_prop_u32(reboot_fdt, "value", 0x7777);
            fdt_node_add_prop_u32(reboot_fdt, "offset", 0x0);
            fdt_node_add_prop_u32(reboot_fdt, "regmap", fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"soc"),"test",base)));
            fdt_node_add_prop_str(reboot_fdt, "compatible", "syscon-reboot");
            fdt_node_add_child(fdt, reboot_fdt);
        }
    }

uint64_t SYSCON::read(HART* hart,uint64_t addr, uint64_t size) {
    return dram_load(&ram,addr,size);
}

void SYSCON::write(HART* hart, uint64_t addr, uint64_t size, uint64_t val) {
    uint64_t offset = base - addr;
    switch(offset) {
        case 0:
            switch(val) {
                case 0x5555: {
                    //Power off
                    poweroff();
                }; break;
                case 0x7777: {
                    //Reboot
                    reset();
                }; break;
            }; break;
    }
}