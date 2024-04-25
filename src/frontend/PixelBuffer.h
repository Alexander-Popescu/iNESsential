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

    void update(bool update);
    uint32_t* getBuffer();
    SDL_Texture* getTexture();
    void writeBufferPixel(int x, int y, uint32_t color);
    void writeBufferPixelIndex(int index, uint32_t color);

    ImVec4 palettes[8][4];

    //ppu debugging things
    ImTextureID getPatternTableTexture(int index);
    ImVec4* getPalette(int index);
    void addPixelArrayToPatternTable(const uint32_t* pixels, int index);

    private:
        SDL_Texture* texture;
        SDL_Renderer* renderer;
        uint32_t* pixel_buffer_buffer;
        int width, height;

        //pattern table
        ImTextureID patternTables[2];
};