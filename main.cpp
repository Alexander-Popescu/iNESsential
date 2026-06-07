#include "SDL.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include <vector>
#include "src/frontend/PixelBuffer.h"
#include "src/Emulator.h"
#include "src/frontend/DebugWindow.h"
#include "src/Definitions.h"

int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("NES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DEFAULT_WIDTH * WINDOW_SCALE_FACTOR, DEFAULT_HEIGHT * WINDOW_SCALE_FACTOR, window_flags);

    //Renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    
    // Main loop
    bool done = false;

    //store window resolution for quick access
    int window_width;
    int window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    //pixel buffer, really just a texture with some extra stuff for nes debugging
    PixelBuffer* pixelBuffer = new PixelBuffer(renderer, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    //emulator pointer for easy reset
    Emulator *emulator = new Emulator(pixelBuffer);

    DebugWindow* debugWindow = new DebugWindow(window, renderer, emulator, pixelBuffer);

    Uint64 target_ticks = SDL_GetPerformanceFrequency() / 60;

    while(!done)
    {
        //calculate time
        Uint64 start_time = SDL_GetPerformanceCounter();

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if(event.type == SDL_QUIT)
            {
                done = true;
            }
            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
            {
                done = true;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                done = true;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)
            {
                //toggle debug window
                debugWindow->show_debug_window = !debugWindow->show_debug_window;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_p)
            {
                emulator->realtime = !emulator->realtime;
            }


            //controller
            emulator->controller1 = 0;
            const Uint8 *state = SDL_GetKeyboardState(NULL);
            if (state[SDL_SCANCODE_RIGHT]) emulator->controller1 |= 0x80;
            if (state[SDL_SCANCODE_LEFT]) emulator->controller1 |= 0x40;
            if (state[SDL_SCANCODE_DOWN]) emulator->controller1 |= 0x20;
            if (state[SDL_SCANCODE_UP]) emulator->controller1 |= 0x10;
            if (state[SDL_SCANCODE_S]) emulator->controller1 |= 0x08;
            if (state[SDL_SCANCODE_A]) emulator->controller1 |= 0x04;
            if (state[SDL_SCANCODE_X]) emulator->controller1 |= 0x02;
            if (state[SDL_SCANCODE_Z]) emulator->controller1 |= 0x01;
            
            //resize window event
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
            {   
                window_width = (event.window.data2 * DEFAULT_WIDTH) / DEFAULT_HEIGHT;
                window_height = event.window.data2;
                SDL_SetWindowSize(window, window_width, window_height);
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (emulator->cartridgeLoaded) {
            emulator->runUntilBreak(-1);

            //texture should update when emulator breaks, but this still gets run when paused
            pixelBuffer->update(emulator->realtime);
        }

        SDL_RenderCopy(renderer, pixelBuffer->getTexture(), NULL, NULL);

        if(debugWindow->show_debug_window) {
            debugWindow->update(window_width, window_height);
        }

        SDL_RenderPresent(renderer);

        //calculate time for since last frame
        int frametime = (int)((double)(SDL_GetPerformanceCounter() - start_time) * 1000.0 / (double)SDL_GetPerformanceFrequency());
        debugWindow->frametimes.push_back(frametime);

        //circulate frametime buffer
        if (debugWindow->frametimes.size() > debugWindow->MAX_FRAMETIMES) {
            debugWindow->frametimes.erase(debugWindow->frametimes.begin());
        }

        // Frame rate limiting to 60 FPS
        Uint64 end_time = SDL_GetPerformanceCounter();
        Uint64 elapsed = end_time - start_time;
        if (elapsed < target_ticks) {
            double delay_ms = (double)(target_ticks - elapsed) * 1000.0 / (double)SDL_GetPerformanceFrequency();
            if (delay_ms > 1.0) {
                SDL_Delay((Uint32)(delay_ms - 1.0));
            }
            while (SDL_GetPerformanceCounter() - start_time < target_ticks) {
                SDL_Delay(0);
            }
        }
    }
    
    //Cleanup
    delete debugWindow;
    delete pixelBuffer;
    delete emulator;
}