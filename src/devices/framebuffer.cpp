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

#include "../../include/devices/framebuffer.hpp"
#include "../../include/sdl.hpp"

FRAMEBUFFER::FRAMEBUFFER(uint64_t base, DRAM& ram,fdt_node* fdt, uint16_t width, uint16_t height)
        : Device(base, uint64_t(width)*height*4, ram)
    {
        if(fdt != NULL) {
            struct fdt_node* fb_fdt = fdt_node_create_reg("framebuffer", base);
            fdt_node_add_prop_reg(fb_fdt, "reg", base, height*width*4);
            fdt_node_add_prop_str(fb_fdt, "compatible", "simple-framebuffer");
            fdt_node_add_prop_str(fb_fdt, "format", "a8r8g8b8");
            fdt_node_add_prop_u32(fb_fdt, "width", width);
            fdt_node_add_prop_u32(fb_fdt, "height", height);
            fdt_node_add_prop_u32(fb_fdt, "stride", width*4);

            fdt_node_add_child(fdt_node_find(fdt,"soc"), fb_fdt);
        }
    }

uint64_t FRAMEBUFFER::read(HART* hart,uint64_t addr, uint64_t size) {
    return dram_load(&ram,addr,size);
}

void FRAMEBUFFER::write(HART* hart, uint64_t addr, uint64_t size, uint64_t val) {
    if (addr > base + framebuffer.size()*4) {
        std::cerr << "FB: invalid write " << std::hex << addr << std::endl;
        return;
    }

    dram_store(&ram, addr, size, val);

    uint64_t offset = addr - base;
    uint64_t pixel_index = offset / 4;

    uint8_t r = hart->dram.mmap->load(base + pixel_index * 4 + 0, 8);
    uint8_t g = hart->dram.mmap->load(base + pixel_index * 4 + 1, 8);
    uint8_t b = hart->dram.mmap->load(base + pixel_index * 4 + 2, 8);

    uint32_t color = (0xFF << 24) | (r << 16) | (g << 8) | b;

    SDL_writePixel(pixel_index, color);
}


void FRAMEBUFFER::clear() {
    SDL_clearTexture();
}