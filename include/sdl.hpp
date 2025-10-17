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

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <cstdint>
#include <thread>
#include <vector>

extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern GLuint texture;
extern std::vector<uint32_t> framebuffer;

// For exiting from program
extern SDL_Event event;

extern bool using_SDL;
extern uint16_t sdl_width;
extern uint16_t sdl_height;

extern bool SDL_initSDL(uint16_t width, uint16_t height);
extern void SDL_writePixel(uint64_t pos, uint32_t color);
extern void SDL_clearTexture();
extern void SDL_loop();