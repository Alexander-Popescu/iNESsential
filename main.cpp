#include "SDL.h"
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl3.h"
#include <vector>
#include "src/PixelBuffer.h"
#include "src/Emulator.h"


#define DEFAULT_WIDTH 256
#define DEFAULT_HEIGHT 240

//IMGUI font
#define FONT_SCALE 2

//change for larger / smaller window size
#define WINDOW_SCALE_FACTOR 4

//ansii terminal color codes
#define RED "\x1b[31m"
#define YELLOW "\x1b[33m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

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

    // Setup ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //make font larger
    io.FontGlobalScale = FONT_SCALE;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Setup style
    ImGui::StyleColorsLight();
    
    // Main loop
    bool done = false; //used to exit main loop
    bool show_debug_window = true;
    bool pause = false;//stops calling the pixelbuffers update function

    //store window resolution for quick access
    int window_width;
    int window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    //pixel buffer, really just a texture with some extra stuff
    PixelBuffer* pixelBuffer = new PixelBuffer(renderer, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    //circular buffer for graphing frametimes
    std::vector<float> frametimes;
    const int MAX_FRAMETIMES = 100;

    //emulator pointer for easy reset
    Emulator *emulator = new Emulator();

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
                show_debug_window = !show_debug_window;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_p)
            {
                pause = !pause;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_h)
            {
                //fill buffer with white and update
                for(int i = 0; i < DEFAULT_WIDTH * DEFAULT_HEIGHT; i++) {
                    pixelBuffer->writeBufferPixelIndex(i, rand() % 0xFFFFFFFF);
                }
                pixelBuffer->update(pause);
            }
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

        pixelBuffer->update(pause);

        if(show_debug_window) {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(window);
            ImGui::NewFrame();

            ImGui::Begin("Debug Window");

            //emulation state variables
            ImGui::Text("Current Emulation State (Toggle P): %s", pause ? "Paused" : "Running"); 
            ImGui::Separator();

            //pixelbuffer debug info
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.9f, 1.0f));
            ImGui::Text("PixelBuffer Info:");
            ImGui::PopStyleColor();
            ImGui::Separator();
            
            ImGui::Text("Window Resolution: %d x %d", window_width, window_height);
            ImGui::Text("Window Aspect Ratio: %f", (float)window_width / (float)window_height);
            ImGui::Text("Texture Resolution: %d x %d", DEFAULT_HEIGHT, DEFAULT_WIDTH);
            ImGui::Text("Texture Aspect Ratio: %f", (float)DEFAULT_HEIGHT / (float)DEFAULT_WIDTH);
            ImGui::Text("Actual FPS: %f", ImGui::GetIO().Framerate);
            ImGui::PlotLines("FrameTimes (ms)", &frametimes[0], frametimes.size(), 0, NULL, 0.0f, 100.0f, ImVec2(0, 80));
            ImGui::Text("Graph is from 0 - 100 ms, measures time since last frame");
            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.9f, 1.0f));
            ImGui::Text("Emulator Debug Info:");
            ImGui::PopStyleColor();
            ImGui::Separator();

            if (ImGui::Button("Reset")) {
                //reset emulator
                printf(YELLOW "Main: Emulator Reset\n" RESET);
                if (emulator) {
                    delete emulator;
                }
                emulator = new Emulator();
                emulator->reset();
            }

            if (emulator->cartridgeLoaded == true) {
                //load only once cart is loaded to avoid segfault, since this data (shouldnt) exist untill then

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
                ImGui::Text("Cartridge Loaded");
                ImGui::PopStyleColor();

                ImGui::Text("Pattern Tables:");

                ImGui::Image(pixelBuffer->getPatternTableTexture(0), ImVec2(128 * 2, 128 * 2));
                ImGui::SameLine();
                ImGui::Image(pixelBuffer->getPatternTableTexture(1), ImVec2(128 * 2, 128 * 2));

            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("Cartridge Not Loaded");
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.9f, 1.0f));
            ImGui::Text("Press Space to hide this window");
            ImGui::PopStyleColor();

            ImGui::End();

            // Rendering
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
        frametimes.push_back(frametime);

        //circulate frametime buffer
        if (frametimes.size() > MAX_FRAMETIMES) {
            frametimes.erase(frametimes.begin());
        }
    }
    
    // Cleanup
    delete pixelBuffer;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}