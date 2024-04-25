project('NES', 'cpp')

sdl2_dep = dependency('sdl2', required : true)
gl_dep = dependency('gl', required : true)
glfw_dep = dependency('glfw3', required : true)

# imgui static lib
imgui_src = files('imgui/imgui.cpp', 'imgui/imgui_draw.cpp', 'imgui/imgui_widgets.cpp', 'imgui/imgui_impl_sdl2.cpp', 'imgui/imgui_impl_opengl3.cpp', 'imgui/imgui_tables.cpp', 'imgui/imgui_impl_glfw.cpp')
imgui_inc = include_directories('imgui')
imgui_lib = static_library('imgui', imgui_src, include_directories : imgui_inc, dependencies : [sdl2_dep, gl_dep, glfw_dep])

executable('NES', 'main.cpp', 'src/frontend/PixelBuffer.cpp', 'src/Emulator.cpp', 'src/components/CPU.cpp', 'src/components/Cartridge.cpp', 'src/components/PPU.cpp', 'src/frontend/DebugWindow.cpp', 'src/registerTypes/reg8.cpp', 'src/registerTypes/reg16.cpp', 'src/registerTypes/shiftReg.cpp', dependencies : [sdl2_dep, gl_dep, glfw_dep], link_with : imgui_lib)