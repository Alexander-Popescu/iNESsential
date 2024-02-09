#include "PixelBuffer.h"

PixelBuffer::PixelBuffer(SDL_Renderer* renderer, int width, int height) {
    this->renderer = renderer;
    this->width = width;
    this->height = height;
    //all zeros
    this->pixel_buffer_buffer = (uint32_t*) calloc(width * height, sizeof(uint32_t));
    this->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
}

PixelBuffer::~PixelBuffer() {
    delete[] pixel_buffer_buffer;
    SDL_DestroyTexture(texture);
}

void PixelBuffer::update(bool pause) {
    if (pause)
    {
        //prevent black screen on pause
        SDL_GL_BindTexture(texture, NULL, NULL);
        return; 
    }
    SDL_UpdateTexture(texture, NULL, pixel_buffer_buffer, width * sizeof(uint32_t));
}

void PixelBuffer::writeBufferPixel(int x, int y, uint32_t color)
{
    pixel_buffer_buffer[x + y * width] = color;
}

void PixelBuffer::writeBufferPixelIndex(int index, uint32_t color)
{
    pixel_buffer_buffer[index] = color;
}

uint32_t* PixelBuffer::getBuffer() {
    return pixel_buffer_buffer;
}

SDL_Texture* PixelBuffer::getTexture() {
    return texture;
}