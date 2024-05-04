#include "SDL.h"
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl3.h"
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
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
    SDL_Window* window = SDL_CreateWindow("NES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DEFAULT_WIDTH * WINDOW_SCALE_FACTOR, DEFAULT_HEIGHT * WINDOW_SCALE_FACTOR, window_flags);

    //OpenGL context
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    //Renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    
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

    DebugWindow* debugWindow = new DebugWindow(window, gl_context, emulator, pixelBuffer);

    while(!done)
    {
        //calculate time
        int start_time = SDL_GetTicks();

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
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_v)
            {
                //toggle vsync, just to test uncapped performance
                SDL_GL_SetSwapInterval(SDL_GL_GetSwapInterval() == 1 ? 0 : 1);
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

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        emulator->runUntilBreak(-1);

        //texture should update when emulator breaks, but this still gets run when paused
        pixelBuffer->update(emulator->realtime);


        if(debugWindow->show_debug_window) {
            debugWindow->update(window_width, window_height);
        }

        //render texture
        int display_w, display_h;
        SDL_GetWindowSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, pixelBuffer->getTexture(), NULL, NULL);
        SDL_GL_SwapWindow(window);

        //calculate time for since last frame
        int frametime = (SDL_GetTicks() - start_time);
        debugWindow->frametimes.push_back(frametime);

        //circulate frametime buffer
        if (debugWindow->frametimes.size() > debugWindow->MAX_FRAMETIMES) {
            debugWindow->frametimes.erase(debugWindow->frametimes.begin());
        }
    }
    
    //Cleanup
    delete debugWindow;
    delete pixelBuffer;
    delete emulator;
}