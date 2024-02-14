//a class to hold imgui calls to avoid an unreadable main class

#include "SDL.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl3.h"
#include "src/PixelBuffer.h"
#include "src/Emulator.h"

#define DEFAULT_WIDTH 256
#define DEFAULT_HEIGHT 240
#define RED "\x1b[31m"
#define YELLOW "\x1b[33m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

//IMGUI font
#define FONT_SCALE 2

class DebugWindow {
public:
    DebugWindow(SDL_Window* window, SDL_GLContext gl_context, Emulator* emulator, PixelBuffer* pixelBuffer);
    ~DebugWindow();

    void Update(int window_width, int window_height);

    //functions to further abstract all the different pages
    void ppuDebugInfo();
    void cpuDebugInfo();

    //variables to track and change things about the debug window 

    //circular buffer for graphing frametimes
    std::vector<float> frametimes;
    const unsigned int MAX_FRAMETIMES = 100;

    bool show_debug_window = true;

    //variable to track what page of debug screen to show, to save space
    int debugPage = 0;

private:
    SDL_Window* window;
    SDL_GLContext gl_context;
    Emulator* emulator;
    PixelBuffer* pixelBuffer;
};