//a class to hold imgui calls to avoid an unreadable main class

#pragma once
#include "SDL.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl3.h"
#include "src/PixelBuffer.h"
#include "src/Emulator.h"
#include "src/Definitions.h"

class DebugWindow {
public:
    DebugWindow(SDL_Window* window, SDL_GLContext gl_context, Emulator* emulator, PixelBuffer* pixelBuffer);
    ~DebugWindow();

    void update(int window_width, int window_height);

    //functions to further abstract all the different pages
    void ppuDebugInfo();
    void cpuDebugInfo();

    //variables to track and change things about the debug window 

    //circular buffer for graphing frametimes
    std::vector<float> frametimes;
    const unsigned int MAX_FRAMETIMES = 250;

    bool show_debug_window = true;

private:
    SDL_Window* window;
    SDL_GLContext gl_context;
    Emulator* emulator;
    PixelBuffer* pixelBuffer;

    uint8_t patternTableScalingValue = 3;

    //for rendering cpu status flags
    const char* flagNames = "CZIDB-VN";
};