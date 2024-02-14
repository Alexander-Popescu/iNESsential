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

    //ppu debugging things
    ImTextureID getPatternTableTexture(int index);
    ImVec4* getPalette(int index);
    void addPixelArrayToPatternTable(const uint32_t* pixels, int index);
    void updatePalettes();
    void updatePatternTables();

    private:
        SDL_Texture* texture;
        SDL_Renderer* renderer;
        uint32_t* pixel_buffer_buffer;
        int width, height;

        //pattern table
        ImTextureID patternTables[2];

        //test palettes, should pull directly from ppu later but for debugging they are hardcoded
        ImVec4 palettes[8][4] = {
            ImVec4(0.462f, 0.462f, 0.462f, 1.0f), ImVec4(0.561f, 0.561f, 0.561f, 1.0f), ImVec4(0.670f, 0.670f, 0.670f, 1.0f), ImVec4(0.796f, 0.796f, 0.796f, 1.0f),
            ImVec4(0.561f, 0.310f, 0.310f, 1.0f), ImVec4(0.627f, 0.416f, 0.416f, 1.0f), ImVec4(0.706f, 0.518f, 0.518f, 1.0f), ImVec4(0.788f, 0.627f, 0.627f, 1.0f),
            ImVec4(0.369f, 0.180f, 0.561f, 1.0f), ImVec4(0.471f, 0.290f, 0.647f, 1.0f), ImVec4(0.561f, 0.420f, 0.722f, 1.0f), ImVec4(0.655f, 0.561f, 0.796f, 1.0f),
            ImVec4(0.235f, 0.235f, 0.235f, 1.0f), ImVec4(0.357f, 0.357f, 0.357f, 1.0f), ImVec4(0.482f, 0.482f, 0.482f, 1.0f), ImVec4(0.608f, 0.608f, 0.608f, 1.0f),
            ImVec4(0.561f, 0.310f, 0.310f, 1.0f), ImVec4(0.627f, 0.416f, 0.416f, 1.0f), ImVec4(0.706f, 0.518f, 0.518f, 1.0f), ImVec4(0.788f, 0.627f, 0.627f, 1.0f),
            ImVec4(0.369f, 0.180f, 0.561f, 1.0f), ImVec4(0.471f, 0.290f, 0.647f, 1.0f), ImVec4(0.561f, 0.420f, 0.722f, 1.0f), ImVec4(0.655f, 0.561f, 0.796f, 1.0f),
            ImVec4(0.235f, 0.235f, 0.235f, 1.0f), ImVec4(0.357f, 0.357f, 0.357f, 1.0f), ImVec4(0.482f, 0.482f, 0.482f, 1.0f), ImVec4(0.608f, 0.608f, 0.608f, 1.0f),
            ImVec4(0.561f, 0.310f, 0.310f, 1.0f), ImVec4(0.627f, 0.416f, 0.416f, 1.0f), ImVec4(0.706f, 0.518f, 0.518f, 1.0f), ImVec4(0.788f, 0.627f, 0.627f, 1.0f)
        };

};