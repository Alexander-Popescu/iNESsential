#pragma once
#include "SDL.h"
#include <cstdint>
#include <vector>
#include <GLFW/glfw3.h>
#include "../imgui/imgui.h"

class PixelBuffer {
public:
    PixelBuffer(SDL_Renderer* renderer, int width, int height);
    ~PixelBuffer();

    // turns array of pixel information into texture and puts into the pattern table buffer
    void addPixelArrayToPatternTable(const uint32_t* pixels, int index);

    void update(bool pause);
    uint32_t* getBuffer();
    SDL_Texture* getTexture();
    void writeBufferPixel(int x, int y, uint32_t color);
    void writeBufferPixelIndex(int index, uint32_t color);

    ImTextureID getPatternTableTexture(int index);

private:
    SDL_Texture* texture;
    SDL_Renderer* renderer;
    uint32_t* pixel_buffer_buffer;
    int width, height;

    //pattern table
    ImTextureID patternTables[2];

};