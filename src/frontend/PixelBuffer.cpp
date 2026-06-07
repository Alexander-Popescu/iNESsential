#include "PixelBuffer.h"
#include "../imgui/imgui.h"

PixelBuffer::PixelBuffer(SDL_Renderer* renderer, int width, int height) {
    this->renderer = renderer;
    this->width = width;
    this->height = height;
    //all zeros
    this->pixel_buffer_buffer = (uint32_t*) calloc(width * height, sizeof(uint32_t));
    this->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    this->patternTables[0] = nullptr;
    this->patternTables[1] = nullptr;
}

PixelBuffer::~PixelBuffer() {
    delete[] pixel_buffer_buffer;
    SDL_DestroyTexture(texture);
    if (patternTables[0]) {
        SDL_DestroyTexture((SDL_Texture*)patternTables[0]);
    }
    if (patternTables[1]) {
        SDL_DestroyTexture((SDL_Texture*)patternTables[1]);
    }
}

void PixelBuffer::update(bool update) {
    if (!update)
    {
        return; 
    }

    SDL_UpdateTexture(texture, NULL, pixel_buffer_buffer, width * sizeof(uint32_t));
}

//functions for changing individual pixels in the main texture, either by index or coords
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

//functions for pattern table rendering
void PixelBuffer::addPixelArrayToPatternTable(const uint32_t* pixels, int index)
{
    SDL_Texture* tex = (SDL_Texture*)patternTables[index];
    if (!tex) {
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 128, 128);
        patternTables[index] = (ImTextureID)tex;
    }
    SDL_UpdateTexture(tex, NULL, pixels, 128 * sizeof(uint32_t));
}

ImTextureID PixelBuffer::getPatternTableTexture(int index) {
    return patternTables[index];
}

ImVec4* PixelBuffer::getPalette(int index) {
    return palettes[index];
}