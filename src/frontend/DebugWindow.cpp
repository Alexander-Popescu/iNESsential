#include "DebugWindow.h"
#include <stdio.h>
#include <time.h>

DebugWindow::DebugWindow(SDL_Window* window, SDL_Renderer* renderer, Emulator* emulator, PixelBuffer* pixelBuffer) {

    this->window = window;
    this->renderer = renderer;
    this->emulator = emulator;
    this->pixelBuffer = pixelBuffer;

    // Setup ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Query DPI scale factor
    int window_w, window_h;
    int render_w, render_h;
    SDL_GetWindowSize(window, &window_w, &window_h);
    SDL_GetRendererOutputSize(renderer, &render_w, &render_h);
    float scale = (float)render_w / (float)window_w;
    if (scale < 1.0f) scale = 1.0f;

    // Load fonts relative to build / root directory fallbacks
    const char* mainFontPaths[] = {
        "imgui/misc/fonts/Roboto-Medium.ttf",
        "../imgui/misc/fonts/Roboto-Medium.ttf",
        "iNESsential/imgui/misc/fonts/Roboto-Medium.ttf",
        "../iNESsential/imgui/misc/fonts/Roboto-Medium.ttf"
    };
    const char* monoFontPaths[] = {
        "imgui/misc/fonts/Cousine-Regular.ttf",
        "../imgui/misc/fonts/Cousine-Regular.ttf",
        "iNESsential/imgui/misc/fonts/Cousine-Regular.ttf",
        "../iNESsential/imgui/misc/fonts/Cousine-Regular.ttf"
    };

    for (int i = 0; i < 4; i++) {
        FILE* f = fopen(mainFontPaths[i], "rb");
        if (f) {
            fclose(f);
            mainFont = io.Fonts->AddFontFromFileTTF(mainFontPaths[i], 14.0f * scale);
            break;
        }
    }
    
    for (int i = 0; i < 4; i++) {
        FILE* f = fopen(monoFontPaths[i], "rb");
        if (f) {
            fclose(f);
            monoFont = io.Fonts->AddFontFromFileTTF(monoFontPaths[i], 13.0f * scale);
            break;
        }
    }

    if (!mainFont) {
        io.Fonts->AddFontDefault();
        io.FontGlobalScale = FONT_SCALE;
    } else {
        io.FontGlobalScale = 1.0f / scale;
    }

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Apply custom modern dark slate NES-retro style
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.ScrollbarSize = 15.0f;
    style.GrabMinSize = 10.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.08f, 0.08f, 0.10f, 0.98f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.12f, 0.12f, 0.14f, 0.60f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.10f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.12f, 0.12f, 0.15f, 0.50f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.28f, 0.35f, 0.48f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.35f, 0.45f, 0.60f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.25f, 0.35f, 0.55f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.10f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);

    loadRecentRoms();
    if (emulator->cartridgeLoaded) {
        addRecentRom(emulator->cartName);
    }
}

DebugWindow::~DebugWindow() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void DebugWindow::update(int window_width, int window_height) {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    // Set a good default window size for vertical dashboard layout
    ImGui::SetNextWindowSize(ImVec2(800, 700), ImGuiCond_FirstUseEver);
    ImGui::Begin("NES Emulation Dashboard / Debugger");

    // ================= STATS CARD =================
    ImGui::BeginChild("StatsCard", ImVec2(0, 195), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
    ImGui::Text("SYSTEM STATISTICS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    
    ImGui::Columns(2, "StatsCols", false);
    ImGui::SetColumnWidth(0, 320);
    ImGui::SetColumnWidth(1, 440);

    ImGui::Text("Status: %s", emulator->realtime ? "Running (Realtime)" : "Paused");
    ImGui::Text("Vsync: On");
    ImGui::Text("Actual FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Window Resolution: %d x %d", window_width, window_height);
    ImGui::Text("Viewport Aspect Ratio: %.2f", (float)window_width / (float)window_height);
    ImGui::Text("NES Resolution: %d x %d (%.2f)", DEFAULT_WIDTH, DEFAULT_HEIGHT, (float)DEFAULT_WIDTH / (float)DEFAULT_HEIGHT);

    ImGui::NextColumn();
    ImGui::PlotLines("Frame Time (ms)", &frametimes[0], frametimes.size(), 0, NULL, 0.0f, 100.0f, ImVec2(0, 70));
    ImGui::Text("Scale: 0 - 100 ms");
    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // ================= CONTROLS CARD =================
    ImGui::BeginChild("ControlsCard", ImVec2(0, 115), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
    ImGui::Text("EMULATOR CONTROLS");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (ImGui::Button("Reset")) {
        //reset emulator
        printf(YELLOW "Main: Emulator Reset\n" RESET);
        emulator->reset();
    }

    ImGui::SameLine();
    if (ImGui::Button("Step Cycle")) {
        emulator->runSingleCycle();
        //update pixelbuffer to see new state
        pixelBuffer->update(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Step Instruction")) {
        //says instruction but no opcodes implemented so its cycles for now
        emulator->runUntilBreak(1);
        //update pixelbuffer to see new state
        pixelBuffer->update(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Step Frame")) {
        emulator->runSingleFrame();
        //update pixelbuffer to see new state
        pixelBuffer->update(true);
    }

    if (ImGui::Button("NESTEST (8991 cycles)")) {
        //says instruction but no opcodes implemented so its cycles for now
        emulator->runUntilBreak(8991);
        //update pixelbuffer to see new state
        pixelBuffer->update(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Toggle Logging")) {
        if (emulator->logging == false) {
            //open logfile
            //unique filename and timestamp
            const char* logPaths[] = {
                "logs/",
                "../logs/",
                "iNESsential/logs/",
                "../iNESsential/logs/"
            };
            
            emulator->logFile = NULL;
            for (int i = 0; i < 4; i++) {
                sprintf(emulator->filename, "%siNESsential_%ld.log", logPaths[i], time(NULL));
                emulator->logFile = fopen(emulator->filename, "w");
                if (emulator->logFile != NULL) {
                    break;
                }
            }
        }
        emulator->logging = !emulator->logging;
    }

    ImGui::SameLine();
    bool logging = emulator->logging;
    ImGui::TextColored(logging ? ImVec4(0.1f, 0.9f, 0.1f, 1.0f) : ImVec4(0.9f, 0.1f, 0.1f, 1.0f), 
                       "Log: %s", logging ? "Active" : "Off");

    ImGui::Text("Instr Count: %i | CPU Cycles: %i | Ticks: %i", 
                emulator->instructionCount, *emulator->getCycleCount(), emulator->emulationTicks);
    ImGui::EndChild();

    ImGui::Spacing();

    cpuDebugInfo();

    ImGui::Spacing();

    ppuDebugInfo();
    memoryHexViewer();

    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::Text("Press Space to toggle dashboard visibility");
    ImGui::PopStyleColor(1);

    ImGui::End();

    // Rendering
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
}

void DebugWindow::ppuDebugInfo() {
    // ROM Loader Card
    ImGui::BeginChild("ROMLoaderCard", ImVec2(0, 95), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
    ImGui::Text("CARTRIDGE LOAD & ROMS");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (ImGui::Button("Load Cartridge")) {
        printf(YELLOW "Debug: Cartridge Load\n" RESET);
        if (emulator->loadCartridge(emulator->cartName)) {
            addRecentRom(emulator->cartName);
        }
        emulator->reset();
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(250);
    ImGui::InputText("ROM Name", emulator->cartName, sizeof(emulator->cartName));
    ImGui::PopItemWidth();

    if (!recentRoms.empty()) {
        ImGui::Text("Recent:");
        for (const auto& rom : recentRoms) {
            ImGui::SameLine();
            char buttonLabel[64];
            sprintf(buttonLabel, "%s##recent", rom.c_str());
            if (ImGui::Button(buttonLabel)) {
                strcpy(emulator->cartName, rom.c_str());
                if (emulator->loadCartridge(emulator->cartName)) {
                    addRecentRom(emulator->cartName);
                }
                emulator->reset();
            }
        }
    }
    ImGui::EndChild();

    ImGui::Spacing();

    if (emulator->cartridgeLoaded == true) {
        // Automatically update pattern tables and palettes in real-time
        emulator->updatePatternTables();
        emulator->updatePalettes();

        // Calculate PPU card height dynamically to prevent clipping or scrolling under high-DPI
        float textLineHeight = ImGui::GetTextLineHeightWithSpacing();
        float itemSpacingY = ImGui::GetStyle().ItemSpacing.y;
        float imageSize = 128.0f * PATTERN_TABLE_SCALING_VALUE;
        float ppuCardHeight = ImGui::GetFrameHeightWithSpacing() * 2.0f 
                            + textLineHeight * 3.0f 
                            + imageSize 
                            + 2.0f * (30.0f + itemSpacingY) 
                            + 50.0f; // Padding buffer

        ImGui::BeginChild("PPUCard", ImVec2(0, ppuCardHeight), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
        ImGui::Text("PPU GRAPHICS & PALETTES");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::Text("Mapper: %i | PRG Banks: %i | CHR Banks: %i | PPU Cycle: %i | Scanline: %i", 
                    emulator->cartridge->mapper, 
                    emulator->cartridge->PRGsize / PRG_ROM_BANKSIZE, 
                    emulator->cartridge->CHRsize / CHR_ROM_BANKSIZE,
                    emulator->getPPUcycle(), 
                    emulator->getPPUscanline());

        ImGui::Spacing();

        ImGui::Text("Pattern Tables:");
        ImGui::Image(pixelBuffer->getPatternTableTexture(0), ImVec2(128 * PATTERN_TABLE_SCALING_VALUE, 128 * PATTERN_TABLE_SCALING_VALUE));
        ImGui::SameLine();
        ImGui::Image(pixelBuffer->getPatternTableTexture(1), ImVec2(128 * PATTERN_TABLE_SCALING_VALUE, 128 * PATTERN_TABLE_SCALING_VALUE));

        ImGui::Spacing();

        ImGui::Text("Palettes:");
        //remove spacing temporarily
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
        //loop over system palettes
        for (int i = 0; i < 8; i++) {
            ImVec4* palette = pixelBuffer->getPalette(i);
            for (int j = 0; j < 4; j++) {
                //render small box as color
                ImGui::ColorButton("##palette", palette[j], ImGuiColorEditFlags_NoBorder, ImVec2(30, 30));
                ImGui::SameLine();
            }
            //spacing between palettes
            ImGui::Dummy(ImVec2(10, 0));
            if (i % 4 == 3) {
                ImGui::NewLine();
            } else {
                ImGui::SameLine();
            }
        }
        //revert styling
        ImGui::PopStyleVar();

        ImGui::EndChild();
    } else {
        ImGui::BeginChild("PPUCard", ImVec2(0, 80), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("PPU Graphics Unavailable (Cartridge Not Loaded)");
        ImGui::PopStyleColor(1);
        ImGui::EndChild();
    }
}

void DebugWindow::cpuDebugInfo() {
    CpuState *state = emulator->getCpuState();

    ImGui::BeginChild("CPUStateCard", ImVec2(0, 130), true, ImGuiWindowFlags_NoScrollbar);
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
    ImGui::Text("CPU REGISTERS & FLAGS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Align status flags
    ImGui::Text("Flags: ");
    ImGui::SameLine();
    for (int i = 0; i < 8; ++i) {
        char flagName[2] = { flagNames[i], '\0' };
        //red or green based on 0 / 1
        ImGui::TextColored(state->status_register & (1 << i) ? ImVec4(0.1f, 0.9f, 0.1f, 1.0f) : ImVec4(0.9f, 0.1f, 0.1f, 1.0f), "%s", flagName);
        if (i < 7) {
            ImGui::SameLine();
            ImGui::Text(" ");
            ImGui::SameLine();
        }
    }
    ImGui::Spacing();

    // Interactive register inputs in Hex
    ImGui::Text("Edit: ");
    ImGui::SameLine();
    ImGui::PushItemWidth(45);
    ImGui::InputScalar("A", ImGuiDataType_U8, &state->accumulator, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::InputScalar("X", ImGuiDataType_U8, &state->x_register, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::InputScalar("Y", ImGuiDataType_U8, &state->y_register, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::PushItemWidth(65);
    ImGui::InputScalar("PC", ImGuiDataType_U16, &state->program_counter, NULL, NULL, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::PushItemWidth(45);
    ImGui::InputScalar("SP", ImGuiDataType_U8, &state->stack_pointer, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::InputScalar("P", ImGuiDataType_U8, &state->status_register, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::PopItemWidth();

    ImGui::EndChild();
}

void DebugWindow::loadRecentRoms() {
    recentRoms.clear();
    FILE* fp = fopen("recent_roms.txt", "r");
    if (!fp) {
        fp = fopen("../recent_roms.txt", "r");
    }
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            size_t len = strlen(line);
            while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
                line[len-1] = '\0';
                len--;
            }
            if (len > 0) {
                recentRoms.push_back(line);
            }
        }
        fclose(fp);
    }
}

void DebugWindow::saveRecentRoms() {
    FILE* fp = fopen("recent_roms.txt", "w");
    if (fp) {
        for (const auto& rom : recentRoms) {
            fprintf(fp, "%s\n", rom.c_str());
        }
        fclose(fp);
    }
}

void DebugWindow::addRecentRom(const std::string& romName) {
    if (romName.empty()) return;
    for (auto it = recentRoms.begin(); it != recentRoms.end(); ++it) {
        if (*it == romName) {
            recentRoms.erase(it);
            break;
        }
    }
    recentRoms.insert(recentRoms.begin(), romName);
    if (recentRoms.size() > 5) {
        recentRoms.pop_back();
    }
    saveRecentRoms();
}

void DebugWindow::memoryHexViewer() {
    ImGui::Spacing();
    
    ImGui::BeginChild("HexViewerCard", ImVec2(0, 185), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
    ImGui::Text("CPU RAM VIEWER ($0000 - $07FF)");
    ImGui::PopStyleColor();
    ImGui::Separator();
    
    if (monoFont) ImGui::PushFont(monoFont);
    if (ImGui::BeginChild("HexViewerChild", ImVec2(0, 135), false, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        
        ImGui::Text("Addr   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  |  ASCII");
        ImGui::Separator();
        
        for (int row = 0; row < 128; row++) {
            uint16_t addr = row * 16;
            
            char rowText[128];
            int offset = sprintf(rowText, "%04X:  ", addr);
            
            for (int col = 0; col < 16; col++) {
                uint8_t val = emulator->getRamValue(addr + col);
                offset += sprintf(rowText + offset, "%02X ", val);
            }
            
            offset += sprintf(rowText + offset, " |  ");
            
            for (int col = 0; col < 16; col++) {
                uint8_t val = emulator->getRamValue(addr + col);
                char c = (val >= 32 && val <= 126) ? (char)val : '.';
                offset += sprintf(rowText + offset, "%c", c);
            }
            
            ImGui::Text("%s", rowText);
        }
        ImGui::PopStyleVar();
    }
    ImGui::EndChild(); // End of HexViewerChild
    if (monoFont) ImGui::PopFont();
    ImGui::EndChild();  // End of HexViewerCard
}