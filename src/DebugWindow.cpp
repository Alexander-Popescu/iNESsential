#include "DebugWindow.h"
#include <stdio.h>
#include <time.h>

DebugWindow::DebugWindow(SDL_Window* window, SDL_GLContext gl_context, Emulator* emulator, PixelBuffer* pixelBuffer) {

    this->window = window;
    this->gl_context = gl_context;
    this->emulator = emulator;
    this->pixelBuffer = pixelBuffer;

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

    //open logfile
    char filename[36];
    //unique filename and timestamp
    sprintf(filename, "../logs/iNESsential_%ld.log", time(NULL));
    emulator->logFile = fopen(filename, "w");
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
    ImGui::Text("Current Emulation State (Toggle P): %s", emulator->realtime ? "Realtime" : "Paused"); 
    ImGui::Text("Vsync (Toggle V): %s", SDL_GL_GetSwapInterval() == 1 ? "On" : "Off");
    ImGui::Text("Refresh Main Texture (H Key)");
    ImGui::Separator();

    //pixelbuffer debug info
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.9f, 1.0f));
    ImGui::Text("PixelBuffer Info:");
    ImGui::PopStyleColor(1);
    ImGui::Separator();
    
    ImGui::Text("Window Resolution: %d x %d", window_width, window_height);
    ImGui::Text("Window Aspect Ratio: %f", (float)window_width / (float)window_height);
    ImGui::Text("Texture Resolution: %d x %d", DEFAULT_WIDTH, DEFAULT_HEIGHT);
    ImGui::Text("Texture Aspect Ratio: %f", (float)DEFAULT_HEIGHT / (float)DEFAULT_WIDTH);
    ImGui::Text("Actual FPS: %f", ImGui::GetIO().Framerate);
    ImGui::PlotLines("FrameTimes (ms)", &frametimes[0], frametimes.size(), 0, NULL, 0.0f, 100.0f, ImVec2(0, 80));
    ImGui::Text("Graph is from 0 - 100 ms, measures time since last frame");
    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.9f, 1.0f));
    ImGui::Text("Emulator Debug Info:");

    //buttons for changing the debug page
    ImGui::SameLine();
    if (ImGui::Button("CPU Page")) {
        debugPage = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("PPU Page")) {
        debugPage = 1;
    }

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

    ImGui::SameLine();
    if (ImGui::Button("Run Single instruction")) {
        //says instruction but no opcodes implemented so its cycles for now
        emulator->runUntilBreak(1);
        //update pixelbuffer to see new state
        pixelBuffer->update(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Toggle Logging")) {
        emulator->logging = !emulator->logging;
    }
    ImGui::SameLine();
    bool logging = emulator->logging;
    ImGui::PushStyleColor(ImGuiCol_Text, logging ? ImVec4(0.1f, 0.9f, 0.1f, 1.0f) : ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
    ImGui::Text("Log: %s", logging ? "True" : "False");
    ImGui::PopStyleColor(1);

    ImGui::Text("Instruction Count: %i | Cycle Count: %i", emulator->instructionCount, *emulator->getCycleCount());

    if (debugPage == 0) {
        cpuDebugInfo();
    }
    else if (debugPage == 1) {
        ppuDebugInfo();
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

void DebugWindow::ppuDebugInfo() {
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
}

void DebugWindow::cpuDebugInfo() {

    CpuState *state = emulator->getCpuState();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.8f, 1.0f));
    ImGui::Text("CPU Debug Info: ");
    ImGui::PopStyleColor(1);
    
    ImGui::Text("Registers: A: %i , X; %i , Y: %i", state->accumulator, state->x_register, state->y_register);
    ImGui::Text("PC: 0x%04X, SP: 0x%02X", state->program_counter, state->stack_pointer);

    //status registers with color coding
    for (int i = 0; i < 8; ++i) {
        char flagName[2] = { flagNames[i], '\0' };
        //red or green based on 0 / 1
        ImGui::TextColored(state->status_register & (1 << i) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), flagName);
        ImGui::SameLine();
    }

    ImGui::Separator();
}