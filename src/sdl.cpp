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

#include <../include/sdl.hpp>
#include <thread>
#include <sstream>
#include <iomanip>
#include <../include/main.hpp>

SDL_Window* window;
SDL_Renderer* renderer;
GLuint texture;
std::vector<uint32_t> framebuffer;

SDL_Event event;

bool using_SDL = true;
uint16_t sdl_width;
uint16_t sdl_height;

std::thread sdl_thr;

bool SDL_initSDL(uint16_t width, uint16_t height) {
    sdl_width = width;
    sdl_height = height;

    bool out = SDL_Init(SDL_INIT_VIDEO);
    if (!out) {
        SDL_Log("[ERROR] Cannot init SDL: %s", SDL_GetError());
        using_SDL = false;
        return false;
    }

    window = SDL_CreateWindow("Risc-V Em", width,height,SDL_WINDOW_OPENGL);
    SDL_ShowWindow(window);
    framebuffer = std::vector<uint32_t>(width * height, 0x0);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sdl_width, sdl_height, 0,
             GL_RGBA, GL_UNSIGNED_BYTE, framebuffer.data());

    return true;
}

void SDL_writePixel(uint64_t pos, uint32_t color) {
    // Color must be in A8R8G8B8 format
    framebuffer[pos] = 0xFF000000 | color;
}

void SDL_clearTexture() {
    for (int y = 0; y < sdl_height; ++y) {
        for (int x = 0; x < sdl_width; ++x) {
            framebuffer[y * sdl_width + x] = 0xFF000000;
        }
    }
}

void SDL_loop() {
    if(using_SDL) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) poweroff(true); break;
        }

        glClear(GL_COLOR_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sdl_width, sdl_height,
                        GL_RGBA, GL_UNSIGNED_BYTE, framebuffer.data());

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex2f(-1, -1);
            glTexCoord2f(1, 1); glVertex2f(1, -1);
            glTexCoord2f(1, 0); glVertex2f(1, 1);
            glTexCoord2f(0, 0); glVertex2f(-1, 1);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        SDL_GL_SwapWindow(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}