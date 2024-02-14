#include "DebugWindow.h"
#include <stdio.h>

DebugWindow::DebugWindow(SDL_Window* window, SDL_GLContext gl_context, Emulator* emulator, PixelBuffer* pixelBuffer)
    : window(window), gl_context(gl_context), emulator(emulator), pixelBuffer(pixelBuffer) {

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
}

DebugWindow::~DebugWindow() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void DebugWindow::Update(int window_width, int window_height) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    ImGui::Begin("Debug Window");

    //emulation state variables
    ImGui::Text("Current Emulation State (Toggle P): %s", emulator->realtime ? "Paused" : "Realtime"); 
    ImGui::Separator();

    //pixelbuffer debug info
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.9f, 1.0f));
    ImGui::Text("PixelBuffer Info:");
    ImGui::PopStyleColor(1);
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
    if (ImGui::Button("Run Single instruction")) {
        //says instruction but no opcodes implemented so its cycles for now
        emulator->runUntilBreak(1);
        //update pixelbuffer to see new state
        pixelBuffer->update(true);
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

        //button to update pattern tables
        if (ImGui::Button("Update Pattern Tables")) {
            printf(YELLOW "Main: Pattern Table Update\n" RESET);
            pixelBuffer->updatePatternTables();
        }

        ImGui::Separator();

        ImGui::Text("Palettes:");
        //remove spacing temporarily
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
        //loop over system palettes
        for (int i = 0; i < 8; i++) {
            ImVec4* palette = pixelBuffer->getPalette(i);
            for (int j = 0; j < 4; j++) {
                //render small box as color
                ImGui::ColorButton("##palette", palette[j], ImGuiColorEditFlags_NoBorder, ImVec2(40, 40));
                ImGui::SameLine();
            }
            //spacing between palettes
            ImGui::Dummy(ImVec2(10, 0));
            if (i % 4 == 3) {
                ImGui::NewLine();
            } else {
                ImGui::SameLine();
            }
            if (i == 7) {
                //button to update palettes
                if (ImGui::Button("Update Palettes")) {
                    printf(YELLOW "Main: Palette Update\n" RESET);
                    pixelBuffer->updatePalettes();
                }
            }
        }
        //revert styling
        ImGui::PopStyleVar();

    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("Cartridge Not Loaded");
        ImGui::PopStyleColor(1);
    }

    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.9f, 1.0f));
    ImGui::Text("Press Space to hide this window");
    ImGui::PopStyleColor(1);

    ImGui::End();

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}