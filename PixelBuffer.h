#pragma once
#include "SDL.h"
#include <cstdint>
#include <vector>

class PixelBuffer {
public:
    PixelBuffer(SDL_Renderer* renderer, int width, int height);
    ~PixelBuffer();

    void update(bool pause);
    uint32_t* getBuffer();
    SDL_Texture* getTexture();
    void writeBufferPixel(int x, int y, uint32_t color);
    void writeBufferPixelIndex(int index, uint32_t color);

private:
    SDL_Texture* texture;
    SDL_Renderer* renderer;
    uint32_t* pixel_buffer_buffer;
    int width, height;
};