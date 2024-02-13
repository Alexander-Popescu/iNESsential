#include "PixelBuffer.h"
#include "../imgui/imgui.h"

PixelBuffer::PixelBuffer(SDL_Renderer* renderer, int width, int height) {
    this->renderer = renderer;
    this->width = width;
    this->height = height;
    //all zeros
    this->pixel_buffer_buffer = (uint32_t*) calloc(width * height, sizeof(uint32_t));
    this->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    // create pattern table textures, for testing just use noise
    uint32_t pixels[128 * 128];
    for (int i = 0; i < 128 * 128; i++) {
        pixels[i] = rand() % 0xFFFFFFFF;
    }
    addPixelArrayToPatternTable(pixels, 0);

    for (int i = 0; i < 128 * 128; i++) {
        pixels[i] = rand() % 0xFFFFFFFF;
    }
    addPixelArrayToPatternTable(pixels, 1);
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
    
    //fill with noise for testing
    for(int i = 0; i < width * height; i++) {
        pixel_buffer_buffer[i] = rand() % 0xFFFFFFFF;
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
    // 128x128 patterntable texture from array of pixels
    uint32_t texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // nearest for no anti-aliasing
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    //double cast in case pointer is larger than u32
    patternTables[index] = (ImTextureID)(uintptr_t)texture;

    return;
}

ImTextureID PixelBuffer::getPatternTableTexture(int index) {
    return patternTables[index];
}

ImVec4* PixelBuffer::getPalette(int index) {
    return palettes[index];
}

void PixelBuffer::updatePalettes() {
    //not implemented but this will take the current palette ram and update the palettes as vec4s
    return;
}

void PixelBuffer::updatePatternTables() {
    //not implemented but this pulls pattern tables from the cartridge
    return;            
}