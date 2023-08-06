#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <json.h>

//6502
//A: Accumulator
//X: Register
//Y: Register
//PC: Program Counter
//SP: Stack Pointer
//P: Status Register

//cpu method:
// 1 read byte at PC
// 2 -> index array to find addressing mode and cycle count
// 3 read any additional bytes
// 4 execute instruction
// 5 wait, count clock cycles, until complete

uint8_t check_flag(uint8_t flag);
void set_flag(uint8_t flag, bool value);
void interrupt_request();
void updateFrame();

uint32_t instruction_count = 0;

//CPU 6502, globals ah!

//Accumulator: A
uint8_t accumulator = 0x00;

//Registers: X, Y
uint8_t x_register = 0x00;
uint8_t y_register = 0x00;

//Program Counter: PC
uint16_t program_counter = 0x8000;

//Stack Pointer: SP, descending stack
uint8_t stack_pointer = 0xFD;

//Status Register: P
uint8_t status_register = 0x00;

//Flags:
static uint8_t C_flag = 0b00000001; // C carry bit flag
static uint8_t Z_flag = 0b00000010; // Z zero flag
static uint8_t I_flag = 0b00000100; // I interrupt disable
static uint8_t D_flag = 0b00001000; // D decimal mode
static uint8_t B_flag = 0b00010000; // B break command
static uint8_t U_flag = 0b00100000; // U unused
static uint8_t V_flag = 0b01000000; // V overflow flag
static uint8_t N_flag = 0b10000000; // N negative flag

//related variables
uint8_t current_opcode = 0x00;
uint8_t cycles = 0x00;

//variable to link the addressing modes with the opcodes
uint8_t data_at_absolute = 0x00;

bool accumulator_mode = false;

uint16_t absolute_address = 0x0000;
uint16_t relative_address = 0x0000;


uint8_t cpuRam[0x0800] = {0};
uint8_t cpuTestRam[0xFFFF] = {0};
bool cpu_test_mode = false;

//CPU BUS SPECIFICATIONS:

//0x0000-0x07FF: 2KB Ram
//0x0800-0x0FFF: Mirrors of 0x0000-0x07FF
//0x1000-0x17FF: Mirrors of 0x0000-0x07FF
//0x1800-0x1FFF: Mirrors of 0x0000-0x07FF
//0x2000-0x2007: PPU registers
//0x2008-0x3FFF: Mirrors of PPU registers
//0x4000-0x4017: APU and I/O registers
//0x4018-0x401F: APU and I/O functionality that is normally disabled
//0x4020-0xFFFF: Cartridge space: PRG ROM, PRG RAM, and mapper registers

//for debug output
FILE* fp;

//total cycles elapsed
uint64_t total_cycles = 0;

//PPU BUS SPECIFICATIONS:

//0x0000-0x0FFF: Pattern table 0
//0x1000-0x1FFF: Pattern table 1
//0x2000-0x23FF: Nametable 0
//0x2400-0x27FF: Nametable 1
//0x2800-0x2BFF: Nametable 2
//0x2C00-0x2FFF: Nametable 3
//0x3000-0x3EFF: Mirrors of 0x2000-0x2EFF
//0x3F00-0x3F1F: Palette RAM indexes
//0x3F20-0x3FFF: Mirrors of 0x3F00-0x3F1F

//32 color palette
uint8_t ppu_palette[32] = {0};
uint8_t selected_palette = 0x00;

//256byte object attribute memory
uint8_t ppu_oam[0x100] = {0};

// PPU registers


//bitfield control register
typedef union {
    struct {
        uint8_t nametable_x : 1;
        uint8_t nametable_y : 1;
        uint8_t increment_mode : 1;
        uint8_t pattern_sprite : 1;
        uint8_t pattern_background : 1;
        uint8_t sprite_size : 1;
        uint8_t slave_mode : 1;
        uint8_t enable_nmi : 1;
    };
    uint8_t reg;
} PPUCTRL;

PPUCTRL ppu_ctrl;

//bitfield mask register
typedef union {
    struct {
        uint8_t grayscale : 1;
        uint8_t render_background_left : 1;
        uint8_t render_sprites_left : 1;
        uint8_t render_background : 1;
        uint8_t render_sprites : 1;
        uint8_t enhance_red : 1;
        uint8_t enhance_green : 1;
        uint8_t enhance_blue : 1;
    };
    uint8_t reg;
} PPUMASK;

PPUMASK ppu_mask;

//bitfield status register
typedef union {
    struct {
        uint8_t unused : 5;
        uint8_t sprite_overflow : 1;
        uint8_t sprite_zero_hit : 1;
        uint8_t vertical_blank : 1;
    };
    uint8_t reg;
} PPUSTATUS;

PPUSTATUS ppu_status;

uint8_t OAMADDR;    // 0x2003
uint8_t OAMDATA;    // 0x2004
uint8_t PPUSCROLL;  // 0x2005
uint8_t PPUADDR;    // 0x2006
uint8_t PPUDATA;    // 0x2007

//PPU helpers
uint8_t address_latch = 0x00;
uint8_t ppu_data_buffer = 0x00;
uint32_t frame_count = 0x00;

//loopy register
typedef union {
    struct {
        uint16_t coarse_x : 5;
        uint16_t coarse_y : 5;
        uint16_t nametable_x : 1;
        uint16_t nametable_y : 1;
        uint16_t fine_y : 3;
        uint16_t unused : 1;
    };
    uint8_t reg;
} LOOPY;

LOOPY vram_address;
LOOPY tram_address;

uint8_t fine_x;

//rom file vars
uint8_t mapper = 0x00;
char* mirror_mode = 0x00;
uint8_t prg_banks = 0x00;
uint8_t chr_banks = 0x00;

//cartridge
uint8_t* PRG_ROM;
uint8_t* CHR_ROM;

//sizes
uint32_t prg_rom_size;
uint32_t chr_rom_size;

//Nametables 2kb
uint8_t nametables[2][1024] = {0};

//rendering variables
uint8_t bg_next_tile_id = 0x00;
uint8_t bg_next_tile_attrib = 0x00;
uint8_t bg_next_tile_lsb = 0x00;
uint8_t bg_next_tile_msb = 0x00;

uint16_t bg_shifter_pattern_lo = 0x0000;
uint16_t bg_shifter_pattern_hi = 0x0000;
uint16_t bg_shifter_attrib_lo = 0x0000;
uint16_t bg_shifter_attrib_hi = 0x0000;

//nmi bool
bool nmi = false;

//emulation modes
bool fullspeed = false;
bool run_single_frame = false;
bool run_single_instruction = false;
bool run_single_cycle = false;


//SDL globals
#define WIDTH 256
#define HEIGHT 240

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Texture* debug_texture;

TTF_Font* font;
SDL_Window* debug_window;
SDL_Renderer* debug_renderer;
TTF_Font* font;
SDL_Color White = {255, 255, 255};
SDL_Rect Message_rect;
SDL_Texture* Message;

bool debug_window_flag = true;

uint8_t r[WIDTH * HEIGHT];
uint8_t g[WIDTH * HEIGHT];
uint8_t b[WIDTH * HEIGHT];

bool mapper_0_cpu_read(uint16_t address, uint32_t* mapped_address)
{
    if (address >= 0x8000 && address <= 0xFFFF)
    {
        if (prg_rom_size == 0x4000)
        {
            *mapped_address = address & 0x3FFF;
            return true;
        }
        else if (prg_rom_size == 0x8000)
        {
            *mapped_address = address & (prg_banks > 1 ? 0x7FFF : 0x3FFF);
            return true;
        }
    }

    return false;
        
}

bool mapper_0_cpu_write(uint16_t address, uint32_t* mapped_address)
{
    if (address >= 0x8000 && address <= 0xFFFF)
    {
        *mapped_address = address & (prg_banks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

bool mapper_0_ppu_read(uint16_t address, uint32_t* mapped_address)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        *mapped_address = address;
        return true;
    }
    return false;
}

bool mapper_0_ppu_write(uint16_t address, uint32_t* mapped_address)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        if (chr_banks == 0)
        {
            //ram mode
            *mapped_address = address;
            return true;
        }
    }
    return false;
}


bool cartridgeBus_ppu_read(uint16_t address, uint8_t* data)
{
    uint32_t mapped_address = 0;
    if (mapper_0_ppu_read(address, &mapped_address))
    {
        *data = CHR_ROM[mapped_address];
        return true;
    }
    else
    {
        return false;
    }
}


bool cartridgeBus_ppu_write(uint16_t address, uint8_t data)
{
    uint32_t mapped_address = 0;
    if (mapper_0_ppu_write(address, &mapped_address))
    {
        CHR_ROM[mapped_address] = data;
        return  true;
    }
    else
    {
        return false;
    }
}

bool cartridgeBus_cpu_read(uint16_t address, uint8_t* data)
{
    uint32_t mapped_address = 0;
    if (mapper_0_cpu_read(address, &mapped_address))
    {
        *data = PRG_ROM[mapped_address];
        return  true;
    }
    else
    {
        return false;
    }
}

bool cartridgeBus_cpu_write(uint16_t address, uint8_t data)
{
    uint32_t mapped_address = 0;
    if (mapper_0_cpu_write(address, &mapped_address))
    {
        PRG_ROM[mapped_address] = data;
        return  true;
    }
    else
    {
        return false;
    }
}

uint8_t ppuBus_read(uint16_t address)
{
    uint8_t data = 0x00;
    if (cartridgeBus_ppu_read(address, &data))
    {
        return data;
    }
    if (address > 0x0000 & address < 0x0FFF)
    {
        //pattern table 1
        data = CHR_ROM[address];
    }

    if (address > 0x1000 & address < 0x1FFF)
    {
        //pattern table 2
        data = CHR_ROM[address];
    }

    if (address > 0x2000 & address < 0x23BF)
    {
        //nametable 0
        data = nametables[0][address & 0x03FF];
    }

    if (address > 0x23C0 & address < 0x23FF)
    {
        //attribute table 0
        data = nametables[0][address & 0x03FF];
    }

    if (address > 0x2400 & address < 0x27BF)
    {
        //nametable 1
        data = nametables[1][address & 0x03FF];
    }

    if (address > 0x27C0 & address < 0x27FF)
    {
        //attribute table 1
        data = nametables[1][address & 0x03FF];
    }

    if (address > 0x2800 & address < 0x2BBF)
    {
        //nametable 2
        printf("nametable 2 ppuBus_read\n");
        data = 0x00;
    }

    if (address > 0x2BC0 & address < 0x2BFF)
    {
        //attribute table 2
        printf("attribute table 2 ppuBus_read\n");
        data = 0x00;
    }

    if (address > 0x2C00 & address < 0x2FBF)
    {
        //nametable 3
        printf("nametable 3 ppuBus_read\n");
        data = 0x00;
    }

    if (address > 0x2FC0 & address < 0x2FFF)
    {
        //attribute table 3
        printf("attribute table 3 ppuBus_read\n");
        data = 0x00;
    }

    if (address > 0x3000 & address < 0x3EFF)
    {
        //mirror of 0x2000 - 0x2EFF
        data = ppuBus_read(address & 0x2EFF);
    }

    if (address > 0x3F00 & address < 0x3F1F)
    {
        //palette ram indexes
        data = ppu_palette[address & 0x001F];
    }

    if (address > 0x3F20 & address < 0x3FFF)
    {
        //mirror of 0x3F00 - 0x3F1F
        data = ppuBus_read(address & 0x3F1F);
    }
    return data;

}

void ppuBus_write(uint16_t address, uint8_t data)
{
    if (cartridgeBus_ppu_write(address, data))
    {
        return;
    }
    if (address > 0x0000 && address < 0x2000)
    {
        //CHR ROM
        CHR_ROM[address] = data;
    }
    else if (address > 0x2000 && address < 0x3F00)
    {
        //nametables and attribute tables
        uint8_t table_index = (address >> 10) & 0x03;
        uint16_t table_address = address & 0x03FF;
        nametables[table_index][table_address] = data;
    }
    else if (address > 0x3F00 && address < 0x3F20)
    {
        //palette ram indexes
        uint8_t palette_index = address & 0x001F;
        ppu_palette[palette_index] = data;
    }
    else if (address > 0x3F20 && address < 0x4000)
    {
        //mirror of 0x3F00 - 0x3F1F
        ppuBus_write(address & 0x3F1F, data);
    }
}

uint8_t cpuBus_read(uint16_t address)
{
    uint8_t data = 0x00;
    //branch for testing cpu
    if (cpu_test_mode)
    {
        return cpuTestRam[address];
    }
    if (cartridgeBus_cpu_read(address, &data))
    {
        return data;
    }
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        data = cpuRam[address & 0x07FF];
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        //ppu registers
        switch (address & 0x0007)
        {
            case 0x0000:
                //PPUCTRL
                break;
            case 0x0001:
                //PPUMASK
                break;
            case 0x0002:
                //PPUSTATUS
                data = (ppu_status.reg & 0xE0) | (ppu_data_buffer & 0x1F);
                ppu_status.vertical_blank = 0;
                address_latch = 0;
                break;
            case 0x0003:
                //OAMADDR
                break;
            case 0x0004:
                //OAMDATA
                break;
            case 0x0005:
                //PPUSCROLL
                break;
            case 0x0006:
                //PPUADDR
                break;
            case 0x0007:
                //PPUDATA
                data = ppu_data_buffer;
                ppu_data_buffer = ppuBus_read(vram_address.reg);
                if (vram_address.reg >= 0x3F00)
                {
                    data = ppu_data_buffer;
                }
                vram_address.reg += (ppu_ctrl.increment_mode ? 32 : 1);
                break;
            default:
                // handle invalid address
                break;
        }
    }
    else if (address == 0x4015)
    {
        //apu register
    }
    else if (address == 0x4016)
    {
        //controller 1
    }
    else if (address == 0x4017)
    {
        //controller 2
    }
    else if (address >= 0x4017 && address <= 0x401F)
    {
        //APU and IO registers
    }
    else if (address >= 0x8000 && address <= 0xFFFF)
    {
        //cartridge space
        data = PRG_ROM[address - 0x8000];
    }
    else
    {
        printf("cpuBus_read: address out of range: %04X\n", address);
        data = 0x00;
    }

    return data;
}

void cpuBus_write(uint16_t address, uint8_t data)
{
    //branch for testing cpu
    if (cpu_test_mode)
    {
        cpuTestRam[address] = data;
        return;
    }
    if (cartridgeBus_cpu_write(address, data))
    {
        return;
    }
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        cpuRam[address & 0x07FF] = data;
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        switch (address & 0x0007)
        {
            case 0x0000:
                //PPUCTRL
                ppu_ctrl.reg = data;
                tram_address.nametable_x = ppu_ctrl.nametable_x;
                tram_address.nametable_y = ppu_ctrl.nametable_y;
                break;
            case 0x0001:
                //PPUMASK
                ppu_mask.reg = data;
                break;
            case 0x0002:
                //PPUSTATUS
                break;
            case 0x0003:
                //OAMADDR
                break;
            case 0x0004:
                //OAMDATA
                break;
            case 0x0005:
                //PPUSCROLL
                if (address_latch == 0)
                {
                    fine_x = data & 0x07;
                    tram_address.coarse_x = data >> 3;
                    address_latch = 1;
                } else {
                    tram_address.fine_y = data & 0x07;
                    tram_address.coarse_y = data >> 3;
                    address_latch = 0;
                }
                break;
            case 0x0006:
                //PPUADDR
                if (address_latch == 0)
                {
                    tram_address.reg = (tram_address.reg & 0x00FF) | ((uint16_t)data << 8);
                    address_latch = 1;
                } else {
                    tram_address.reg = (tram_address.reg & 0xFF00) | (uint16_t)data;
                    vram_address = tram_address;
                    address_latch = 0;
                }
                break;
            case 0x0007:
                //PPUDATA
                ppuBus_write(vram_address.reg, data);
                vram_address.reg += (ppu_ctrl.increment_mode ? 32 : 1);
                break;
            default:
                // handle invalid address
                break;
        }
    }
    else if (address == 0x4015)
    {
        //apu register
    }
    else if (address == 0x4016)
    {
        //controller 1
    }
    else if (address == 0x4017)
    {
        //controller 2
    }
    else if (address >= 0x4017 && address <= 0x401F)
    {
        //APU and IO registers
    }
    else if (address >= 0x8000 && address <= 0xFFFF)
    {
        //cartridge space
        PRG_ROM[address - 0x8000] = data;
    }
    else
    {
        printf("cpuBus_write: address out of range: %04X\n", address);
    }
}

//addressing modes
uint8_t IMP()
{
    //implied
    data_at_absolute = accumulator;
    return 0;
}

uint8_t IMM()
{
    //immediate
    absolute_address = program_counter + 1;
    return 0;
}

uint8_t ZP0()
{
    //zero page
    absolute_address = cpuBus_read(program_counter);
    program_counter++;
    absolute_address &= 0x00FF;
    return 0;
}

uint8_t ZPX()
{
    //zero page with offset from X register
    absolute_address = (cpuBus_read(program_counter) + x_register);
    program_counter++;
    absolute_address &= 0x00FF;
    return 0;
}

uint8_t ZPY()
{
    //zero page with offset from Y register
    absolute_address = (cpuBus_read(program_counter) + y_register);
    program_counter++;
    absolute_address &= 0x00FF;
    return 0;
}

uint8_t REL()
{
    //relative
    relative_address = cpuBus_read(program_counter);
    program_counter++;
    if (relative_address & 0x80)
    {
        relative_address |= 0xFF00;
    }
    return 0;
}

uint8_t ABS()
{
    //absolute
    //read first byte
    uint16_t low = cpuBus_read(program_counter);
    program_counter++;
    //second
    uint16_t high = cpuBus_read(program_counter) << 8;
    program_counter++;

    absolute_address = high | low;
    return 0;
}

uint8_t ABX()
{
    //absolute with offset, offsets by X register after bytes are read

    //read first byte
    uint16_t low = cpuBus_read(program_counter);
    program_counter++;
    //second
    uint16_t high = cpuBus_read(program_counter) << 8;
    program_counter++;

    absolute_address = high | low;
    absolute_address += x_register;

    //this is one where if the memory page changes we need more cycles
    uint16_t first_byte = absolute_address & 0xFF00;

    return (first_byte != high) ? 1 : 0;
}

uint8_t ABY()
{
    //absolute with offset

    //read first byte
    uint16_t low = cpuBus_read(program_counter);
    program_counter++;
    //second
    uint16_t high = cpuBus_read(program_counter) << 8;
    program_counter++;

    absolute_address = high | low;
    absolute_address += y_register;

    //this is one where if the memory page changes we need more cycles
    uint16_t first_byte = absolute_address & 0xFF00;

    return (first_byte != high) ? 1 : 0;

    return 0;
}

uint8_t IND()
{
    uint16_t low = cpuBus_read(program_counter);
    program_counter++;
    uint16_t high = cpuBus_read(program_counter);
    program_counter++;

    uint16_t address = (high << 8) | low;

    if (low == 0x00FF)
    {
        //page crossing
        absolute_address = (cpuBus_read(address & 0xFF00) << 8) | cpuBus_read(address);
    }
    else
    {
        //normal case
        absolute_address = (cpuBus_read(address + 1) << 8) | cpuBus_read(address);
    }

    return 0;
}

uint8_t IZX()
{

    uint16_t argument = cpuBus_read(program_counter);
    program_counter++;

    uint16_t low = cpuBus_read(((uint16_t)(argument + (uint16_t)x_register)) & 0x00FF);
    uint16_t high = cpuBus_read(((uint16_t)(argument + (uint16_t)x_register + 1)) & 0x00FF);

    absolute_address = (high << 8) | low;

    return 0;
}

uint8_t IZY()
{
    //indirect indexed
    uint16_t input = cpuBus_read(program_counter);
    program_counter++;

    //get actual address and offset final by y, but also check for page change

    uint16_t low = cpuBus_read(input & 0x00FF);
    uint16_t high = cpuBus_read((input + 1) & 0x00FF) << 8;

    absolute_address = high | low;
    absolute_address += y_register;

    //check for page change
    uint16_t first_byte = absolute_address & 0xFF00;
    
    return first_byte != high ? 1 : 0;
}

//helper function to avoid writing !IMP for every memory address
uint8_t update_absolute_data()
{
    if (accumulator_mode == true)
    {
        data_at_absolute = accumulator;
        return data_at_absolute;
    }
    data_at_absolute = cpuBus_read(absolute_address);
    return data_at_absolute;
}

uint8_t ACC()
{
    //accumulator
    accumulator_mode = true;
    program_counter++;
    return 0;
}

//opcode functions
uint8_t ADC()
{
    //addition
    update_absolute_data();

    uint16_t tmp = (uint16_t)accumulator + (uint16_t)data_at_absolute + (uint16_t)check_flag(C_flag);

    //conditions for flags to be set
    set_flag(C_flag, tmp > 255);
    set_flag(Z_flag, (tmp & 0x00FF) == 0);
    set_flag(N_flag, tmp & 0x80);
    set_flag(V_flag, (~(accumulator ^ data_at_absolute) & (accumulator ^ tmp)) & 0x0080);

    accumulator = tmp & 0x00FF;

    program_counter++;
    return 1;
}

uint8_t AND()
{
    //get new data
    update_absolute_data();

    //perform opcode
    accumulator = accumulator & data_at_absolute;

    //update flags
    set_flag(Z_flag, accumulator == 0x00);
    set_flag(N_flag, accumulator & 0x80);

    //return 1 if extra clock cycle is possible, it will check with addressing mode function
    program_counter++;
    return 1;
}

uint8_t ASL()
{
    update_absolute_data();

    uint16_t tmp = (uint16_t)data_at_absolute << 1;

    set_flag(C_flag, (tmp & 0xFF00) > 0);
    set_flag(Z_flag, (tmp & 0x00FF) == 0x00);
    set_flag(N_flag, tmp & 0x80);

    if (accumulator_mode == true)
    {
        accumulator = tmp & 0x00FF;
    }
    else
    {
        cpuBus_write(absolute_address, tmp & 0x00FF);
    }
    accumulator_mode = false;

    return 0;
}

uint8_t BCC()
{
    if(check_flag(C_flag) == 0)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        program_counter++;
        return 1;
    }

    program_counter++;
    return 1;
}

uint8_t BCS()
{
    if(check_flag(C_flag) == true)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;

        program_counter++;
        return 1;
    }
    program_counter++;
    return 1;
}

uint8_t BEQ()
{
    if (check_flag(Z_flag) == true)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        
        program_counter++;
        return 0;
    }
    program_counter++;
    return 0;
}

uint8_t BIT()
{
    //get new data
    update_absolute_data();

    //perform opcode
    uint8_t tmp = accumulator & data_at_absolute;

    //update flags
    set_flag(Z_flag, tmp == 0x00);

    //set to data from memory locations
    set_flag(N_flag, data_at_absolute & 0x80);
    set_flag(V_flag, data_at_absolute & 0x40);

    program_counter++;
    return 0;
}

uint8_t BMI()
{
    if(check_flag(N_flag) == 1)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
    }
    program_counter++;
    return 1;
}

uint8_t BNE()
{
    if(check_flag(Z_flag) == 0)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        program_counter++;
        return 1;
    }
    program_counter++;
    return 1;
}

uint8_t BPL()
{
    if(check_flag(N_flag) == 0)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        program_counter++;
        return 1;
    }
    program_counter++;
    return 1;
}

uint8_t BRK()
{
    program_counter++;

    set_flag(I_flag, 1);
    cpuBus_write(0x0100 + stack_pointer, (program_counter >> 8) & 0x00FF);
    stack_pointer--;
    cpuBus_write(0x0100 + stack_pointer, program_counter & 0x00FF);
    stack_pointer--;

    set_flag(B_flag, 1);
    cpuBus_write(0x0100 + stack_pointer, status_register);
    stack_pointer--;
    set_flag(B_flag, 0);

    program_counter = (uint16_t)cpuBus_read(0xFFFE) | ((uint16_t)cpuBus_read(0xFFFF) << 8);
    return 0;
}

uint8_t BVC()
{
    if(check_flag(V_flag) == 0)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        program_counter++;
        return 1;
    }
    program_counter++;
    return 1;
}

uint8_t BVS()
{
    if(check_flag(V_flag) == 1)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        program_counter++;
        return 1;
    }
    program_counter++;
    return 1;
}

uint8_t CLC()
{
    set_flag(C_flag, 0);
    program_counter++;
    return 0;
}

uint8_t CLD()
{
    set_flag(D_flag, 0);
    program_counter++;
    return 0;
}

uint8_t CLI()
{
    set_flag(I_flag, 0);
    program_counter++;
    return 0;
}

uint8_t CLV()
{
    set_flag(V_flag, 0);
    program_counter++;
    return 0;
}

uint8_t CMP()
{
    //get new data
    update_absolute_data();

    uint16_t temp = (uint16_t)accumulator - (uint16_t)data_at_absolute;

    set_flag(C_flag, accumulator >= data_at_absolute);
    set_flag(Z_flag, (temp & 0x00FF) == 0x0000);
    set_flag(N_flag, temp & 0x0080);

    program_counter++;
    return 1;
}

uint8_t CPX()
{
    //get new data
    update_absolute_data();

    if (x_register >= data_at_absolute)
    {
        set_flag(C_flag, 1);
    }
    else
    {
        set_flag(C_flag, 0);
    }

    if (x_register == data_at_absolute)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if ((x_register - data_at_absolute) & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    program_counter++;
    return 0;
}

uint8_t CPY()
{
    //get new data
    update_absolute_data();

    if (y_register >= data_at_absolute)
    {
        set_flag(C_flag, 1);
    }
    else
    {
        set_flag(C_flag, 0);
    }

    if (y_register == data_at_absolute)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if ((y_register - data_at_absolute) & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    program_counter++;
    return 0;
}

uint8_t DEC()
{
    update_absolute_data();
    cpuBus_write(absolute_address, data_at_absolute - 1);
    update_absolute_data();

    set_flag(Z_flag, data_at_absolute == 0x00);
    set_flag(N_flag, data_at_absolute & 0x80);

    program_counter++;
    return 0;
}

uint8_t DEX()
{
    x_register--;

    //set flags
    if (x_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (x_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    } 

    program_counter++;
    return 0;
}

uint8_t DEY()
{
    y_register--;

    //set flags
    if (y_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (y_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }
    
    program_counter++;
    return 0;
}

uint8_t EOR()
{
    update_absolute_data();
    accumulator = accumulator ^ data_at_absolute;

    //set flags
    if (accumulator == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (accumulator & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    program_counter++;
    return 1;
}

uint8_t INC()
{
    update_absolute_data();
    cpuBus_write(absolute_address, data_at_absolute + 1);
    update_absolute_data();

    set_flag(Z_flag, data_at_absolute == 0x00);
    set_flag(N_flag, data_at_absolute & 0x80);

    program_counter++;
    return 0;
}

uint8_t INX()
{
    x_register++;

    //set flags
    if (x_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (x_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    program_counter++;
    return 0;
}

uint8_t INY()
{
    y_register++;

    //set flags
    if (y_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (y_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    program_counter++;
    return 0;
}

uint8_t JMP()
{
    program_counter = absolute_address;

    program_counter++;
    return 0;
}

uint8_t JSR()
{

    program_counter--;
    uint16_t second_half = cpuBus_read(program_counter - 1);
    uint16_t first_half = cpuBus_read(program_counter) << 8;

    uint16_t target = first_half | second_half;

    //push program counter to stack
    cpuBus_write(0x0100 + stack_pointer, program_counter >> 8);
    stack_pointer--;
    cpuBus_write(0x0100 + stack_pointer, program_counter & 0x00FF);
    stack_pointer--;

    //set program counter
    program_counter = target;

    program_counter++;
    return 0;
}

uint8_t LDA()
{
    update_absolute_data();
    accumulator = data_at_absolute;

    //set flags
    set_flag(Z_flag, accumulator == 0x00);
    set_flag(N_flag, accumulator & 0x80);

    program_counter++;
    return 1;
}

uint8_t LDX()
{

    update_absolute_data();
    x_register = data_at_absolute;

    //set flags
    if (x_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (x_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    program_counter++;
    return 1;
}

uint8_t LDY()
{

    update_absolute_data();
    y_register = data_at_absolute;

    //set flags
    if (y_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (y_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    return 1;
}

uint8_t LSR()
{
    update_absolute_data();

    uint8_t temp = data_at_absolute >> 1;

    //set flags
    set_flag(C_flag, data_at_absolute & 0x01);
    set_flag(Z_flag, temp == 0x00);
    set_flag(N_flag, temp & 0x80);

    accumulator_mode ? accumulator = temp : cpuBus_write(absolute_address, temp);
    accumulator_mode = false;

    program_counter++;
    return 0;
}

uint8_t NOP()
{
    if ((current_opcode == 0x1C) || (current_opcode == 0x3C) || (current_opcode == 0x5C) || (current_opcode == 0x7C) || (current_opcode == 0xDC) || (current_opcode == 0xFC))
    {
        return 1;
    }
    {
        return 1;
    }

    program_counter++;
    return 0;
}

uint8_t ORA()
{

    update_absolute_data();
    accumulator = accumulator | data_at_absolute;

    //set flags
    if (accumulator == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (accumulator & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    program_counter++;
    return 1;
}

uint8_t PHA()
{
    //stack is descending and fills the 1 page
    cpuBus_write(0x0100 + stack_pointer, accumulator);
    stack_pointer--;
    return 0;
}

uint8_t PHP()
{
    //flags must be set before push
    set_flag(B_flag, 1);
    cpuBus_write(0x0100 + stack_pointer, status_register); 
    set_flag(B_flag, 0);
    set_flag(U_flag, 1);
    stack_pointer--;

    program_counter++;
    return 0;
}

uint8_t PLA()
{
    stack_pointer++;
    accumulator = cpuBus_read(0x0100 + stack_pointer);
    if (accumulator == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (accumulator & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    program_counter++;
    return 0;
}

uint8_t PLP()
{
    
    stack_pointer++;
    status_register = cpuBus_read(0x0100 + stack_pointer);
    set_flag(U_flag, 1);
    set_flag(B_flag, 0);

    program_counter++;
    return 0;
}

uint8_t ROL()
{
    update_absolute_data();

    uint16_t tmp = (uint16_t)(data_at_absolute << 1) | check_flag(C_flag);

    //set flags
    set_flag(C_flag, tmp & 0xFF00);
    set_flag(Z_flag, (tmp & 0x00FF) == 0x0000);
    set_flag(N_flag, tmp & 0x0080);

    if (accumulator_mode == true)
    {
        accumulator = tmp & 0x00FF;
    }
    else
    {
        cpuBus_write(absolute_address, tmp & 0x00FF);
    }

    accumulator_mode = false;

    program_counter++;
    return 0;
}

uint8_t ROR()
{

    update_absolute_data();

    uint16_t tmp = (uint16_t)(data_at_absolute >> 1) | (check_flag(C_flag) << 7);

    //set flags
    set_flag(C_flag, data_at_absolute & 0x01);
    set_flag(Z_flag, tmp == 0x00);
    set_flag(N_flag, tmp & 0x80);

    if (accumulator_mode == true)
    {
        accumulator = tmp & 0x00FF;
    }
    else
    {
        cpuBus_write(absolute_address, tmp & 0x00FF);
    }

    accumulator_mode = false;

    program_counter++;
    return 0;
}

uint8_t RTI()
{
    //returns from an inturrupt

    //read status
    stack_pointer++;
    status_register = cpuBus_read(0x0100 + stack_pointer);
    set_flag(U_flag, 1);
    set_flag(B_flag, 0);

    stack_pointer++;
    uint16_t second_half = (uint16_t)cpuBus_read(0x0100 + stack_pointer);
    stack_pointer++;
    uint16_t first_half = cpuBus_read(0x0100 + stack_pointer) << 8;

    program_counter = first_half | second_half;

    program_counter++;
    return 0;
}

uint8_t RTS()
{

    stack_pointer++;
    program_counter = cpuBus_read(0x0100 + stack_pointer);
    stack_pointer++;
    program_counter = program_counter | ((uint16_t)cpuBus_read(0x0100 + stack_pointer) << 8);
    //go to next instruction
    program_counter++;

    program_counter++;
    return 0;
}

uint8_t SBC()
{
    //16bit because of overflow
    update_absolute_data();

    uint16_t tmp = (uint16_t)accumulator + ((uint16_t)data_at_absolute ^ 0x00FF) + (uint16_t)check_flag(C_flag);

    set_flag(C_flag, tmp & 0xFF00);
    set_flag(Z_flag, (tmp & 0x00FF) == 0);
    set_flag(N_flag, tmp & 0x80);
    set_flag(V_flag, (tmp ^ (uint16_t)accumulator) & (tmp ^ ((uint16_t)data_at_absolute ^ 0x00FF)) & 0x0080);

    accumulator = tmp & 0x00FF;

    program_counter++;
    return 1;
}

uint8_t SEC()
{
    set_flag(C_flag, true);

    program_counter++;
    return 0;
}

uint8_t SED()
{
    set_flag(D_flag, true);

    program_counter++;
    return 0;
}

uint8_t SEI()
{
    set_flag(I_flag, true);

    program_counter++;
    return 0;
}

uint8_t STA()
{
    update_absolute_data();
    

    cpuBus_write(absolute_address, accumulator);

    program_counter++;
    return 0;
}

uint8_t STX()
{
    cpuBus_write(absolute_address, x_register);
    //absolute addressing

    program_counter++;
    return 0;
}

uint8_t STY()
{
    cpuBus_write(absolute_address, y_register);

    program_counter++;
    return 0;
}

uint8_t TAX()
{
    x_register = accumulator;

    set_flag(Z_flag, x_register == 0x00);
    set_flag(N_flag, x_register & 0x80);

    program_counter++;
    return 0;
}

uint8_t TAY()
{
    y_register = accumulator;

    set_flag(Z_flag, y_register == 0x00);
    set_flag(N_flag, y_register & 0x80);

    program_counter++;
    return 0;
}

uint8_t TSX()
{
    x_register = stack_pointer;

    set_flag(Z_flag, x_register == 0x00);
    set_flag(N_flag, x_register & 0x80);

    program_counter++;
    return 0;
}

uint8_t TXA()
{
    accumulator = x_register;

    set_flag(Z_flag, accumulator == 0x00);
    set_flag(N_flag, accumulator & 0x80);

    program_counter++;
    return 0;
}

uint8_t TXS()
{
    stack_pointer = x_register;

    program_counter++;
    return 0;
}

uint8_t TYA()
{
    accumulator = y_register;

    set_flag(Z_flag, accumulator == 0x00);
    set_flag(N_flag, accumulator & 0x80);

    program_counter++;
    return 0;
}

uint8_t XXX()
{
    printf("\nINVALID OPCODE\n");
    printf("opcode: %02X\n", current_opcode);
    program_counter++;
    return 0;
}

//illegal opcodes

uint8_t LAX()
{
    LDA();
    LDX();
    if ((current_opcode == 0xB3 || (current_opcode == 0xB7)))
    {
        return 1;
    }
    return 0;
}

uint8_t SAX()
{
    cpuBus_write(absolute_address, accumulator & x_register);
    return 0;
}

uint8_t DCP()
{
    DEC();
    CMP();
    return 0;
}

uint8_t ISB()
{
    INC();
    SBC();
    return 0;
}

uint8_t SLO()
{
    ASL();
    ORA();
    return 0;
}

uint8_t RLA()
{
    ROL();
    AND();
    return 0;
}

uint8_t SRE()
{
    LSR();
    EOR();
    return 0;
}

uint8_t RRA()
{
    ROR();
    ADC();
    return 0;
}

//opcode matrix stuff

typedef struct opcode
{
    char *name;
    uint8_t (*opcode)(void);
    uint8_t (*addressing_mode)(void);
    uint8_t byte_size;
    uint8_t cycle_count;
} opcode;

opcode opcode_matrix[16][16] = {
//   0                         1                         2                         3                         4                         5                         6                         7                         8                         9                         A                         B                         C                         D                         E                         F
    {{ "BRK", BRK, IMP, 2, 7 },{"ORA", ORA, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"SLO", SLO, IZX, 2, 8 },{"NOP", NOP, ZP0, 0, 3 },{"ORA", ORA, ZP0, 2, 3 },{"ASL", ASL, ZP0, 2, 5 },{"SLO", SLO, ZP0, 2, 5 },{"PHP", PHP, IMP, 1, 3 },{"ORA", ORA, IMM, 2, 2 },{"ASL", ASL, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"NOP", NOP, ABS, 3, 4 },{"ORA", ORA, ABS, 3, 4 },{"ASL", ASL, ABS, 3, 6 },{"SLO", SLO, ABS, 3, 6 }},
    {{ "BPL", BPL, REL, 2, 2 },{"ORA", ORA, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"SLO", SLO, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"ORA", ORA, ZPX, 2, 4 },{"ASL", ASL, ZPX, 2, 6 },{"SLO", SLO, ZPX, 2, 6 },{"CLC", CLC, IMP, 1, 2 },{"ORA", ORA, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"SLO", SLO, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"ORA", ORA, ABX, 3, 4 },{"ASL", ASL, ABX, 3, 7 },{"SLO", SLO, ABX, 3, 7 }},
    {{ "JSR", JSR, ABS, 3, 6 },{"AND", AND, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"RLA", RLA, IZX, 2, 8 },{"BIT", BIT, ZP0, 2, 3 },{"AND", AND, ZP0, 2, 3 },{"ROL", ROL, ZP0, 2, 5 },{"RLA", RLA, ZP0, 2, 5 },{"PLP", PLP, IMP, 1, 4 },{"AND", AND, IMM, 2, 2 },{"ROL", ROL, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"BIT", BIT, ABS, 3, 4 },{"AND", AND, ABS, 3, 4 },{"ROL", ROL, ABS, 3, 6 },{"RLA", RLA, ABS, 3, 6 }},
    {{ "BMI", BMI, REL, 2, 2 },{"AND", AND, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"RLA", RLA, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"AND", AND, ZPX, 2, 4 },{"ROL", ROL, ZPX, 2, 6 },{"RLA", RLA, ZPX, 2, 6 },{"SEC", SEC, IMP, 1, 2 },{"AND", AND, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"RLA", RLA, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"AND", AND, ABX, 3, 4 },{"ROL", ROL, ABX, 3, 7 },{"RLA", RLA, ABX, 3, 7 }},
    {{ "RTI", RTI, IMP, 1, 6 },{"EOR", EOR, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"SRE", SRE, IZX, 2, 8 },{"NOP", NOP, ZP0, 2, 3 },{"EOR", EOR, ZP0, 2, 3 },{"LSR", LSR, ZP0, 2, 5 },{"SRE", SRE, ZP0, 2, 5 },{"PHA", PHA, IMP, 1, 3 },{"EOR", EOR, IMM, 2, 2 },{"LSR", LSR, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"JMP", JMP, ABS, 3, 3 },{"EOR", EOR, ABS, 3, 4 },{"LSR", LSR, ABS, 3, 6 },{"SRE", SRE, ABS, 3, 6 }},
    {{ "BVC", BVC, REL, 2, 2 },{"EOR", EOR, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"SRE", SRE, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"EOR", EOR, ZPX, 2, 4 },{"LSR", LSR, ZPX, 2, 6 },{"SRE", SRE, ZPX, 2, 6 },{"CLI", CLI, IMP, 1, 2 },{"EOR", EOR, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"SRE", SRE, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"EOR", EOR, ABX, 3, 4 },{"LSR", LSR, ABX, 3, 7 },{"SRE", SRE, ABX, 3, 7 }},
    {{ "RTS", RTS, IMP, 1, 6 },{"ADC", ADC, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"RRA", RRA, IZX, 2, 8 },{"NOP", NOP, ZP0, 2, 3 },{"ADC", ADC, ZP0, 2, 3 },{"ROR", ROR, ZP0, 2, 5 },{"RRA", RRA, ZP0, 2, 5 },{"PLA", PLA, IMP, 1, 4 },{"ADC", ADC, IMM, 2, 2 },{"ROR", ROR, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"JMP", JMP, IND, 3, 5 },{"ADC", ADC, ABS, 3, 4 },{"ROR", ROR, ABS, 3, 6 },{"RRA", RRA, ABS, 3, 6 }},
    {{ "BVS", BVS, REL, 2, 2 },{"ADC", ADC, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"RRA", RRA, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"ADC", ADC, ZPX, 2, 4 },{"ROR", ROR, ZPX, 2, 6 },{"RRA", RRA, ZPX, 2, 6 },{"SEI", SEI, IMP, 1, 2 },{"ADC", ADC, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"RRA", RRA, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"ADC", ADC, ABX, 3, 4 },{"ROR", ROR, ABX, 3, 7 },{"RRA", RRA, ABX, 3, 7 }},
    {{ "NOP", NOP, IMM, 2, 2 },{"STA", STA, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"SAX", SAX, IZX, 2, 6 },{"STY", STY, ZP0, 2, 3 },{"STA", STA, ZP0, 2, 3 },{"STX", STX, ZP0, 2, 3 },{"SAX", SAX, ZP0, 2, 3 },{"DEY", DEY, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"TXA", TXA, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"STY", STY, ABS, 3, 4 },{"STA", STA, ABS, 3, 4 },{"STX", STX, ABS, 3, 4 },{"SAX", SAX, ABS, 3, 4 }},
    {{ "BCC", BCC, REL, 2, 2 },{"STA", STA, IZY, 2, 6 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"STY", STY, ZPX, 2, 4 },{"STA", STA, ZPX, 2, 4 },{"STX", STX, ZPY, 2, 4 },{"SAX", SAX, ZPY, 2, 4 },{"TYA", TYA, IMP, 1, 2 },{"STA", STA, ABY, 3, 5 },{"TXS", TXS, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"STA", STA, ABX, 3, 5 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 }},
    {{ "LDY", LDY, IMM, 2, 2 },{"LDA", LDA, IZX, 2, 6 },{"LDX", LDX, IMM, 2, 2 },{"LAX", LAX, IZX, 2, 6 },{"LDY", LDY, ZP0, 2, 3 },{"LDA", LDA, ZP0, 2, 3 },{"LDX", LDX, ZP0, 2, 3 },{"LAX", LAX, ZP0, 2, 3 },{"TAY", TAY, IMP, 1, 2 },{"LDA", LDA, IMM, 2, 2 },{"TAX", TAX, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"LDY", LDY, ABS, 3, 4 },{"LDA", LDA, ABS, 3, 4 },{"LDX", LDX, ABS, 3, 4 },{"LAX", LAX, ABS, 3, 4 }},
    {{ "BCS", BCS, REL, 2, 2 },{"LDA", LDA, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"LAX", LAX, IZY, 2, 5 },{"LDY", LDY, ZPX, 2, 4 },{"LDA", LDA, ZPX, 2, 4 },{"LDX", LDX, ZPY, 2, 4 },{"LAX", LAX, ZPY, 2, 4 },{"CLV", CLV, IMP, 1, 2 },{"LDA", LDA, ABY, 3, 4 },{"TSX", TSX, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"LDY", LDY, ABX, 3, 4 },{"LDA", LDA, ABX, 3, 4 },{"LDX", LDX, ABY, 3, 4 },{"LAX", LAX, ABY, 3, 4 }},
    {{ "CPY", CPY, IMM, 2, 2 },{"CMP", CMP, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"DCP", DCP, IZX, 2, 8 },{"CPY", CPY, ZP0, 2, 3 },{"CMP", CMP, ZP0, 2, 3 },{"DEC", DEC, ZP0, 2, 5 },{"DCP", DCP, ZP0, 2, 5 },{"INY", INY, IMP, 1, 2 },{"CMP", CMP, IMM, 2, 2 },{"DEX", DEX, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"CPY", CPY, ABS, 3, 4 },{"CMP", CMP, ABS, 3, 4 },{"DEC", DEC, ABS, 3, 6 },{"DCP", DCP, ABS, 3, 6 }},
    {{ "BNE", BNE, REL, 2, 2 },{"CMP", CMP, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"DCP", DCP, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"CMP", CMP, ZPX, 2, 4 },{"DEC", DEC, ZPX, 2, 6 },{"DCP", DCP, ZPX, 2, 6 },{"CLD", CLD, IMP, 1, 2 },{"CMP", CMP, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"DCP", DCP, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"CMP", CMP, ABX, 3, 4 },{"DEC", DEC, ABX, 3, 7 },{"DCP", DCP, ABX, 3, 7 }},
    {{ "CPX", CPX, IMM, 2, 2 },{"SBC", SBC, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"ISB", ISB, IZX, 2, 8 },{"CPX", CPX, ZP0, 2, 3 },{"SBC", SBC, ZP0, 2, 3 },{"INC", INC, ZP0, 2, 5 },{"ISB", ISB, ZP0, 2, 5 },{"INX", INX, IMP, 1, 2 },{"SBC", SBC, IMM, 2, 2 },{"NOP", NOP, IMP, 1, 2 },{"SBC", SBC, IMM, 2, 2 },{"CPX", CPX, ABS, 3, 4 },{"SBC", SBC, ABS, 3, 4 },{"INC", INC, ABS, 3, 6 },{"ISB", ISB, ABS, 3, 6 }},
    {{ "BEQ", BEQ, REL, 2, 2 },{"SBC", SBC, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"ISB", ISB, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"SBC", SBC, ZPX, 2, 4 },{"INC", INC, ZPX, 2, 6 },{"ISB", ISB, ZPX, 2, 6 },{"SED", SED, IMP, 1, 2 },{"SBC", SBC, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"ISB", ISB, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"SBC", SBC, ABX, 3, 4 },{"INC", INC, ABX, 3, 7 },{"ISB", ISB, ABX, 3, 7 }},
};

opcode get_opcode(uint8_t input) {
    //split into nibble halfs
    uint8_t opcode_first_half = input >> 4;
    uint8_t opcode_second_half = input & 0x0F;

    //get opcode from matrix
    opcode opcode_obj = opcode_matrix[opcode_first_half][opcode_second_half];

    return opcode_obj;
}

uint8_t palette_colors[64][3] = {
    { 84, 84, 84 },
    { 0, 30, 116 },
    { 8, 16, 144 },
    { 48, 0, 136 },
    { 68, 0, 100 },
    { 92, 0, 48 },
    { 84, 4, 0 },
    { 60, 24, 0 },
    { 32, 42, 0 },
    { 8, 58, 0 },
    { 0, 64, 0 },
    { 0, 60, 0 },
    { 0, 50, 60 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 152, 150, 152 },
    { 8, 76, 196 },
    { 48, 50, 236 },
    { 92, 30, 228 },
    { 136, 20, 176 },
    { 160, 20, 100 },
    { 152, 34, 32 },
    { 120, 60, 0 },
    { 84, 90, 0 },
    { 40, 114, 0 },
    { 8, 124, 0 },
    { 0, 118, 40 },
    { 0, 102, 120 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 236, 238, 236 },
    { 76, 154, 236 },
    { 120, 124, 236 },
    { 176, 98, 236 },
    { 228, 84, 236 },
    { 236, 88, 180 },
    { 236, 106, 100 },
    { 212, 136, 32 },
    { 160, 170, 0 },
    { 116, 196, 0 },
    { 76, 208, 32 },
    { 56, 204, 108 },
    { 56, 180, 204 },
    { 60, 60, 60 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 236, 238, 236 },
    { 168, 204, 236 },
    { 188, 188, 236 },
    { 212, 178, 236 },
    { 236, 174, 236 },
    { 236, 174, 212 },
    { 236, 180, 176 },
    { 228, 196, 144 },
    { 204, 210, 120 },
    { 180, 222, 120 },
    { 168, 226, 144 },
    { 152, 226, 180 },
    { 160, 214, 228 },
    { 160, 162, 160 },
    { 0, 0, 0 },
    { 0, 0, 0 }
};


uint8_t* getRGBvaluefromPalette(uint8_t palette, uint8_t pixel)
{
    return palette_colors[ppu_palette[palette * 4 + pixel]];
}

uint8_t clock_print_flag = 0;
uint8_t single_instruction_latch = 0;

uint16_t ppu_cycle = 0;
int ppu_scanline = 0;
bool frame_complete = false;

void setMainPixel(uint8_t x, uint8_t y, uint8_t red, uint8_t green, uint8_t blue)
{
    r[x + (y * 256)] = red;
    g[x + (y * 256)] = green;
    b[x + (y * 256)] = blue;
}

void ppu_update_shifters()
{
    if (ppu_mask.render_background)
    {
        bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
        bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;

        bg_shifter_attrib_lo = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
        bg_shifter_attrib_hi = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
    }
}

void ppu_load_background_shifters()
{
    bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
    bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;

    bg_shifter_attrib_lo = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
    bg_shifter_attrib_hi = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
}

void ppu_increment_scroll_x()
{
    if (ppu_mask.render_background || ppu_mask.render_sprites)
    {
        if (vram_address.coarse_x == 31)
        {
            vram_address.coarse_x = 0;
            vram_address.nametable_x = ~vram_address.nametable_x;
        }
        else
        {
            vram_address.coarse_x++;
        }
    }
}

void ppu_increment_scroll_y()
{
    if (ppu_mask.render_background || ppu_mask.render_sprites)
    {
        if (vram_address.fine_y < 7)
        {
            vram_address.fine_y++;
        }
        else
        {
            vram_address.fine_y = 0;

            if (vram_address.coarse_y == 29)
            {
                vram_address.coarse_y = 0;
                vram_address.nametable_y = ~vram_address.nametable_y;
            }
            else if (vram_address.coarse_y == 31)
            {
                vram_address.coarse_y = 0;
            }
            else
            {
                vram_address.coarse_y++;
            }
        }
    }
}

void ppu_transfer_address_x()
{
    if (ppu_mask.render_background || ppu_mask.render_sprites)
    {
        vram_address.nametable_x = tram_address.nametable_x;
        vram_address.coarse_x = tram_address.coarse_x;
    }
}

void ppu_transfer_address_y()
{
    if (ppu_mask.render_background || ppu_mask.render_sprites)
    {
        vram_address.fine_y = tram_address.fine_y;
        vram_address.nametable_y = tram_address.nametable_y;
        vram_address.coarse_y = tram_address.coarse_y;
    }
}

void ppu_clock()
{
    if (ppu_scanline >= -1 && ppu_scanline <= 240)
    {
        if (ppu_scanline == -1 && ppu_cycle == 1)
        {
            ppu_status.vertical_blank = 0;
        }

        if ((ppu_cycle >= 2 && ppu_cycle < 258) || (ppu_cycle >= 321 && ppu_cycle < 338))
        {
            ppu_update_shifters();

            switch ((ppu_cycle - 1) % 8)
            {
                case 0:
                    ppu_load_background_shifters();
                    bg_next_tile_id = ppuBus_read(0x2000 | (vram_address.reg & 0x0FFF));
                    break;
                case 2:
                    bg_next_tile_attrib = ppuBus_read(0x23C0 | (vram_address.nametable_y << 11) | (vram_address.nametable_x << 10) | ((vram_address.coarse_y >> 2) << 3) | (vram_address.coarse_x >> 2));
                    if (vram_address.coarse_y & 0x02) bg_next_tile_attrib >>= 4;
                    if (vram_address.coarse_x & 0x02) bg_next_tile_attrib >>= 2;
                    bg_next_tile_attrib &= 0x03;
                    break;
                case 4:
                    bg_next_tile_lsb = ppuBus_read((ppu_ctrl.pattern_background << 12) + ((uint16_t)bg_next_tile_id << 4) + (vram_address.fine_y) + 0);
                    break;
                case 6:
                    bg_next_tile_msb = ppuBus_read((ppu_ctrl.pattern_background << 12) + ((uint16_t)bg_next_tile_id << 4) + (vram_address.fine_y) + 8);
                    break;
                case 7:
                    ppu_increment_scroll_x();
                    break;
            }
        }

        if (ppu_cycle == 256)
        {
            ppu_increment_scroll_y();
        }
        if (ppu_cycle == 257)
        {
            ppu_transfer_address_x();
        }

        if (ppu_scanline == -1 && ppu_cycle >= 280 && ppu_cycle < 305)
        {
            ppu_transfer_address_y();
        }

    }

    if (ppu_scanline == 241 && ppu_cycle == 1)
    {
        ppu_status.vertical_blank = 1;
        if (ppu_ctrl.enable_nmi)
        {
            nmi = true;
        }
    }

    uint8_t bg_pixel = 0x00;
    uint8_t bg_palette = 0x00;

    if (ppu_mask.render_background)
    {
        uint16_t bit_mux = 0x8000 >> fine_x;

        uint8_t p0_pixel = (bg_shifter_pattern_lo & bit_mux) > 0;
        uint8_t p1_pixel = (bg_shifter_pattern_hi & bit_mux) > 0;
        bg_pixel = (p1_pixel << 1) | p0_pixel;

        uint8_t bg_pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
        uint8_t bg_pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
        bg_palette = (bg_pal1 << 1) | bg_pal0;

        uint8_t* rgb_pixel = getRGBvaluefromPalette(bg_palette, bg_pixel);
        
        if (ppu_scanline < 240 && ppu_cycle < 256)
        {
            setMainPixel(ppu_cycle - 1, ppu_scanline, rgb_pixel[0], rgb_pixel[1], rgb_pixel[2]);
        }


    }

    ppu_cycle++;
    if (ppu_cycle >= 341)
    {
        ppu_cycle = 0;
        ppu_scanline++;
        if (ppu_scanline >= 261)
        {
            ppu_scanline = -1;
            frame_complete = true;
        }
    }
}

void reset()
{
    accumulator = 0x00;
    x_register = 0x00;
    y_register = 0x00;
    stack_pointer = 0xFD;
    status_register = 0x00 | (1 << 5);

    program_counter = cpuBus_read(0xFFFC) | (cpuBus_read(0xFFFD) << 8);

    data_at_absolute = 0x00;
    absolute_address = 0x0000;
    relative_address = 0x00;
    cycles = 0;
    total_cycles = 7;
    //nestest specific
    status_register = 0x24;

    //ppu
    ppu_cycle = -1;
    ppu_scanline = 0;
    ppu_data_buffer = 0x00;
    fine_x = 0x00;
    bg_next_tile_id = 0x00;
    bg_next_tile_attrib = 0x00;
    bg_next_tile_lsb = 0x00;
    bg_next_tile_msb = 0x00;
    bg_shifter_pattern_lo = 0x0000;
    bg_shifter_pattern_hi = 0x0000;
    bg_shifter_attrib_lo = 0x0000;
    bg_shifter_attrib_hi = 0x0000;
    ppu_data_buffer = 0x00;
    ppu_scanline = 0;
    ppu_cycle = 0;
}

void interrupt_request()
{
    if (check_flag(I_flag) == 0)
    {
        //pc to stack
        cpuBus_write(0x0100 + stack_pointer, (program_counter >> 8) & 0x00FF);
        stack_pointer--;
        cpuBus_write(0x0100 + stack_pointer, program_counter & 0x00FF);
        stack_pointer--;

        //set flags
        set_flag(B_flag, false);
        set_flag(U_flag, true);
        set_flag(I_flag, true);

        //save flags to stack
        cpuBus_write(0x0100 + stack_pointer, status_register);

        //new pc hard coded
        absolute_address = 0xFFFE;
        uint8_t low = cpuBus_read(absolute_address);
        uint8_t high = cpuBus_read(absolute_address + 1) << 8;

        //combine
        program_counter = high | low;

        cycles = 7;

    }
}

void non_maskable_interrupt()
{
    //pc to stack
    cpuBus_write(0x0100 + stack_pointer, (program_counter >> 8) & 0x00FF);
    stack_pointer--;
    cpuBus_write(0x0100 + stack_pointer, program_counter & 0x00FF);
    stack_pointer--;

    //set flags
    set_flag(B_flag, false);
    set_flag(U_flag, true);
    set_flag(I_flag, true);

    //save flags to stack
    cpuBus_write(0x0100 + stack_pointer, status_register);

    //new pc hard coded
    absolute_address = 0xFFFA;
    uint8_t low = cpuBus_read(absolute_address);
    uint8_t high = cpuBus_read(absolute_address + 1);

    //combine
    program_counter = (high << 8) | low;

    cycles = 8;
}

void set_flag(uint8_t flag, bool value)
{
    if (value == true)
    {
        if (check_flag(flag) == 0)
        {
            status_register += flag;
        }
    }
    else
    {
        if (check_flag(flag) == 1)
        {
            status_register -= flag;
        }
    }
}
uint8_t check_flag(uint8_t flag)
{
    //0000 0000
    //0000 0000
    //NVUB DIZC
    if ((status_register & flag) == flag)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//initializes cpu
void initialize_cpu()
{
    //sets cpu stuff to zero and ram as well
    uint8_t accumulator = 0x00;
    uint8_t x_register = 0x00;
    uint8_t y_register = 0x00;

    uint16_t program_counter = 0x0000;
    uint8_t stack_pointer = 0xFD;
    uint8_t status_register = 0x00;

    uint8_t current_opcode = 0x00;
    uint8_t cycles = 0x00;

    for (int i = 0; i < sizeof(cpuRam); i++)
    {
        //make sure ram is zerod
        cpuRam[i] = 0x00;
    }

}

void load_rom(char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("Error: Could not open file %s\n", filename);
        exit(1);
    }

    // read iNES header
    unsigned char header[16] = {0};
    fread(header, sizeof(unsigned char), 16, file);

    // check for valid iNES header
    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A)
    {
        printf("Error: Invalid iNES header\n");
        exit(1);
    }
    printf("iNES header found and valid\n");

    //mirroring mode
    if ((header[6] & 0x01) == 0x01)
    {
        mirror_mode = "VERTICAL";
    }
    else
    {
        mirror_mode = "HORIZONTAL";
    }

    // extract PRG ROM data
    prg_rom_size = header[4] * 16384;
    prg_banks = header[4];
    PRG_ROM = malloc(prg_rom_size);
    fread(PRG_ROM, sizeof(unsigned char), prg_rom_size, file);

    printf("PRG ROM size: %d bytes\n", prg_rom_size);

    // extract CHR ROM data
    chr_rom_size = header[5] * 8192;
    chr_banks = header[5];
    CHR_ROM = malloc(chr_rom_size);
    fread(CHR_ROM, sizeof(unsigned char), chr_rom_size, file);

    printf("CHR ROM size: %d bytes\n", chr_rom_size);

    // load nametables from CHR ROM into array
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 1024; j++)
        {
            int index = i * 1024 + j;
            nametables[i][j] = CHR_ROM[index];
        }
    }

    //dump CHR ROM to file
    FILE* chr_rom_file = fopen("chr_rom_dump.bin", "wb");
    fwrite(CHR_ROM, sizeof(unsigned char), chr_rom_size, chr_rom_file);
    fclose(chr_rom_file);

    //dump PRG ROM to file
    FILE* prg_rom_file = fopen("prg_rom_dump.bin", "wb");
    fwrite(PRG_ROM, sizeof(unsigned char), prg_rom_size, prg_rom_file);
    fclose(prg_rom_file);

    //dump nametable 0
    FILE* nametable_0_file = fopen("nametable_0.bin", "wb");
    fwrite(nametables[0], sizeof(unsigned char), 1024, nametable_0_file);
    fclose(nametable_0_file);

    //dump nametable 1
    FILE* nametable_1_file = fopen("nametable_1.bin", "wb");
    fwrite(nametables[1], sizeof(unsigned char), 1024, nametable_1_file);
    fclose(nametable_1_file);


    //since palettes are broken, just for visuals
    for (int i = 0; i < 32; i++)
    {
        ppu_palette[i] = 32 - i;
    }


    fclose(file);
}

void print_ram_state(int depth, int start_position)
{
    depth = depth + start_position;
    for (int i = start_position; i < depth; i++)
    {
        printf("cpuBus[0x%x]: 0x%x\n", i, cpuRam[i]);
    }
}

void updateFrame() {

    //create textures for 2 128x128 pattern tables
    SDL_Texture* pattern_table_0 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 128, 128);
    SDL_Texture* pattern_table_1 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 128, 128);

    //create rgb data for pattern tables seperately for each color channel
    uint8_t pattern_table_0_r[128 * 128] = {0};
    uint8_t pattern_table_0_g[128 * 128] = {0};
    uint8_t pattern_table_0_b[128 * 128] = {0};
    
    uint8_t pattern_table_1_r[128 * 128] = {0};
    uint8_t pattern_table_1_g[128 * 128] = {0};
    uint8_t pattern_table_1_b[128 * 128] = {0};

    //fill in rgb data for pattern table 0

    for (uint16_t y_tile = 0; y_tile < 16; y_tile++)
    {
        for (uint16_t x_tile = 0; x_tile < 16; x_tile++)
        {
            //loop over 8x8 pixels in each tile
            uint16_t offset = y_tile * 256 + x_tile * 16;

            for (uint16_t row = 0; row < 8; row++)
            {
                uint8_t tile_least_sig = ppuBus_read(offset + row);
                uint8_t tile_most_sig = ppuBus_read(offset + row + 8);

                for (uint16_t col = 8; col > 0; col--)
                {
                    uint8_t pixel = (tile_least_sig & 0x01) + (tile_most_sig & 0x01);
                    tile_least_sig >>= 1;
                    tile_most_sig >>= 1;

                    //example palette
                    uint8_t* pixel_rgb = getRGBvaluefromPalette(selected_palette, pixel);

                    pattern_table_0_r[(y_tile * 8 + row) * 128 + x_tile * 8 + col] = pixel_rgb[0];
                    pattern_table_0_g[(y_tile * 8 + row) * 128 + x_tile * 8 + col] = pixel_rgb[1];
                    pattern_table_0_b[(y_tile * 8 + row) * 128 + x_tile * 8 + col] = pixel_rgb[2];
                }
            }
        }
    }

    //fill in rgb data for pattern table 1
    for (uint16_t y_tile = 0; y_tile < 16; y_tile++)
    {
        for (uint16_t x_tile = 0; x_tile < 16; x_tile++)
        {
            //loop over 8x8 pixels in each tile
            uint16_t offset = y_tile * 256 + x_tile * 16;

            for (uint16_t row = 0; row < 8; row++)
            {
                uint8_t tile_least_sig = ppuBus_read(0x1000 + offset + row);
                uint8_t tile_most_sig = ppuBus_read(0x1000 +offset + row + 8);

                for (uint16_t col = 8; col > 0; col--)
                {
                    uint8_t pixel = (tile_least_sig & 0x01) + (tile_most_sig & 0x01);
                    tile_least_sig >>= 1;
                    tile_most_sig >>= 1;

                    //example palette
                    uint8_t* pixel_rgb = getRGBvaluefromPalette(selected_palette, pixel);

                    pattern_table_1_r[(y_tile * 8 + row) * 128 + x_tile * 8 + col] = pixel_rgb[0];
                    pattern_table_1_g[(y_tile * 8 + row) * 128 + x_tile * 8 + col] = pixel_rgb[1];
                    pattern_table_1_b[(y_tile * 8 + row) * 128 + x_tile * 8 + col] = pixel_rgb[2];
                }
            }
        }
    }

    //create rgb data for pattern tables by overlaying RGB channels
    uint8_t pattern_table_0_data[128 * 128 * 3] = {0};
    for (int i = 0; i < 128 * 128; i++) {
        pattern_table_0_data[i * 3] = pattern_table_0_r[i + 1];
        pattern_table_0_data[i * 3 + 1] = pattern_table_0_g[i + 1];
        pattern_table_0_data[i * 3 + 2] = pattern_table_0_b[i + 1];
    }

    uint8_t pattern_table_1_data[128 * 128 * 3] = {0};
    for (int i = 0; i < 128 * 128; i++) {
        pattern_table_1_data[i * 3] = pattern_table_1_r[i + 1];
        pattern_table_1_data[i * 3 + 1] = pattern_table_1_g[i + 1];
        pattern_table_1_data[i * 3 + 2] = pattern_table_1_b[i + 1];
    }

    //update pattern table textures with combined pixel data
    SDL_UpdateTexture(pattern_table_0, NULL, pattern_table_0_data, 128 * 3);
    SDL_UpdateTexture(pattern_table_1, NULL, pattern_table_1_data, 128 * 3);

    //main texture

    //update main texture data using r g and b arrays
    uint8_t main_texture_data[256 * 240 * 3] = {0};
    for (int i = 0; i < 256 * 240; i++) {
        main_texture_data[i * 3] = r[i];
        main_texture_data[i * 3 + 1] = g[i];
        main_texture_data[i * 3 + 2] = b[i];
    }

    //update main texture with new data
    SDL_UpdateTexture(texture, NULL, main_texture_data, 256 * 3);

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Get window size
    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    // Calculate aspect ratio
    float aspect_ratio = (float)WIDTH / (float)HEIGHT;

    // Calculate size of rectangle to render RGB data
    int render_width = window_width * 2 / 3;
    int render_height = window_height * 2 / 3;

    if (render_height > window_height * 2 / 3) {
        render_height = window_height * 2 / 3;
        render_width = (int)(render_height * aspect_ratio);
    }

    // Calculate position of rectangle to render RGB data
    int x = 0;
    int y = 0;

    // Render RGB data to screen
    SDL_Rect rect = {x, y, render_width, render_height};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    // Render border around rectangle
    SDL_Rect border_rect = {x - 2, y - 2, render_width + 4, render_height + 4};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &border_rect);

    // Render 8 small rectangles beneath RGB data with spacing
    int small_rect_width = render_width / 16;
    int small_rect_height = small_rect_width;
    int small_rect_y = y + render_height + 10;
    int spacing = small_rect_width / 2;
    for (int i = 0; i < 4; i++) {

        int small_rect_x = x + i * small_rect_width * 4 + spacing * (i + 1);

        //palette first row

        //first rectangle
        SDL_Rect rect = {small_rect_x, small_rect_y, small_rect_height, small_rect_height};
        //get color
        uint8_t *color = getRGBvaluefromPalette(i, 0);
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect);

        //second rectangle
        SDL_Rect rect2 = {small_rect_x + small_rect_height, small_rect_y, small_rect_height, small_rect_height};
        color = getRGBvaluefromPalette(i, 1);
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect2);

        //third rectangle
        SDL_Rect rect3 = {small_rect_x + small_rect_height * 2, small_rect_y, small_rect_height, small_rect_height};
        color = getRGBvaluefromPalette(i, 2);
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect3);

        //fourth rectangle
        SDL_Rect rect4 = {small_rect_x + small_rect_height * 3, small_rect_y, small_rect_height, small_rect_height};
        color = getRGBvaluefromPalette(i, 3);
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect4);

        //white border
        if (selected_palette == i) {
            SDL_Rect small_rect = {small_rect_x, small_rect_y, small_rect_width * 4, small_rect_height};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &small_rect);
        }
    }
    small_rect_y += small_rect_height + 10;
    for (int i = 4; i < 8; i++) {

        int small_rect_x = x + (i - 4) * small_rect_width * 4 + spacing * ((i - 4) + 1);

        //render palette second row

        //first rectangle
        SDL_Rect rect = {small_rect_x, small_rect_y, small_rect_height, small_rect_height};
        uint8_t *color = getRGBvaluefromPalette(i, 0);
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect);

        //second rectangle
        SDL_Rect rect2 = {small_rect_x + small_rect_height, small_rect_y, small_rect_height, small_rect_height};
        color = getRGBvaluefromPalette(i, 1);
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect2);

        //third rectangle
        SDL_Rect rect3 = {small_rect_x + small_rect_height * 2, small_rect_y, small_rect_height, small_rect_height};
        color = getRGBvaluefromPalette(i, 2);
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect3);

        //fourth rectangle
        SDL_Rect rect4 = {small_rect_x + small_rect_height * 3, small_rect_y, small_rect_height, small_rect_height};
        color = getRGBvaluefromPalette(i, 3);
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect4);

        //render white border
        if (selected_palette == i && selected_palette >= 4)
        {
            SDL_Rect small_rect = {small_rect_x, small_rect_y, small_rect_width * 4, small_rect_height};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &small_rect);
        }
    }

    // Render pattern tables to the right of RGB data
    int pattern_table_width = render_width / 2.5;
    int pattern_table_height = render_height / 2.5;
    int pattern_table_x = x + render_width + 10;
    int pattern_table_y = y + (render_height - pattern_table_height) / 4;

    SDL_Rect pattern_table_0_rect = {pattern_table_x, pattern_table_y, pattern_table_width, pattern_table_height};
    SDL_RenderCopy(renderer, pattern_table_0, NULL, &pattern_table_0_rect);

    SDL_Rect pattern_table_1_rect = {pattern_table_x, pattern_table_y + pattern_table_height + 10, pattern_table_width, pattern_table_height};
    SDL_RenderCopy(renderer, pattern_table_1, NULL, &pattern_table_1_rect);

    // Render to screen
    SDL_RenderPresent(renderer);

    //free
    SDL_DestroyTexture(pattern_table_0);
    SDL_DestroyTexture(pattern_table_1);
}

void updateDebugWindow()
{
    // Clear screen
    SDL_Rect clear_rect = {0, 0, 640, 480};
    SDL_SetRenderDrawColor(debug_renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(debug_renderer, &clear_rect);

    //render instruction count, cpu cycle, ppu scanlin, ppu cycle, all registers
    char instruction_count_string[1000] = {0};
    char cpu_cycle_string[1000] = {0};
    char ppu_data_string[1000] = {0};
    char register_string[1000] = {0};

    if (sprintf(instruction_count_string, "Instruction:%i |", instruction_count) < 0) {
        fprintf(stderr, "Error: Failed to format instruction_count_string\n");
        return;
    }
    if (sprintf(cpu_cycle_string, "CPU Cycle:%d |", total_cycles) < 0) {
        fprintf(stderr, "Error: Failed to format cpu_cycle_string\n");
        return;
    }
    if (sprintf(ppu_data_string, "PPU: Scan:%d | Cyc:%d", ppu_scanline, ppu_cycle) < 0) {
        fprintf(stderr, "Error: Failed to format ppu_data_string\n");
        return;
    }
    if (sprintf(register_string, "A:%02X X:%02X Y:%02X P:%02X SP:%02X | PC: %04X | Frame: %i", accumulator, x_register, y_register, status_register, stack_pointer, program_counter, frame_count) < 0) {
        fprintf(stderr, "Error: Failed to format register_string\n");
        return;
    }

    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, instruction_count_string, White);
    if (surfaceMessage == NULL) {
        fprintf(stderr, "Error: Failed to create surfaceMessage for instruction_count_string\n");
        return;
    }
    SDL_Texture* Message = SDL_CreateTextureFromSurface(debug_renderer, surfaceMessage);
    if (Message == NULL) {
        fprintf(stderr, "Error: Failed to create Message for instruction_count_string\n");
        SDL_FreeSurface(surfaceMessage);
        return;
    }
    SDL_Rect Message_rect = (SDL_Rect){0, 0, 200, 25};

    //append other texts to the texture
    SDL_Surface* surfaceMessage2 = TTF_RenderText_Solid(font, cpu_cycle_string, White);
    if (surfaceMessage2 == NULL) {
        fprintf(stderr, "Error: Failed to create surfaceMessage for cpu_cycle_string\n");
        SDL_DestroyTexture(Message);
        SDL_FreeSurface(surfaceMessage);
        return;
    }
    SDL_Texture* Message2 = SDL_CreateTextureFromSurface(debug_renderer, surfaceMessage2);
    if (Message2 == NULL) {
        fprintf(stderr, "Error: Failed to create Message for cpu_cycle_string\n");
        SDL_DestroyTexture(Message);
        SDL_FreeSurface(surfaceMessage);
        SDL_FreeSurface(surfaceMessage2);
        return;
    }
    SDL_Rect Message_rect2 = (SDL_Rect){200, 0, 200, 25};

    SDL_Surface* surfaceMessage3 = TTF_RenderText_Solid(font, ppu_data_string, White);
    if (surfaceMessage3 == NULL) {
        fprintf(stderr, "Error: Failed to create surfaceMessage for ppu_data_string\n");
        SDL_DestroyTexture(Message);
        SDL_DestroyTexture(Message2);
        SDL_FreeSurface(surfaceMessage);
        SDL_FreeSurface(surfaceMessage2);
        return;
    }
    SDL_Texture* Message3 = SDL_CreateTextureFromSurface(debug_renderer, surfaceMessage3);
    if (Message3 == NULL) {
        fprintf(stderr, "Error: Failed to create Message for ppu_data_string\n");
        SDL_DestroyTexture(Message);
        SDL_DestroyTexture(Message2);
        SDL_FreeSurface(surfaceMessage);
        SDL_FreeSurface(surfaceMessage2);
        SDL_FreeSurface(surfaceMessage3);
        return;
    }
    SDL_Rect Message_rect3 = (SDL_Rect){405, 0, 225, 25};

    SDL_Surface* surfaceMessage4 = TTF_RenderText_Solid(font, register_string, White);
    if (surfaceMessage4 == NULL) {
        fprintf(stderr, "Error: Failed to create surfaceMessage for register_string\n");
        SDL_DestroyTexture(Message);
        SDL_DestroyTexture(Message2);
        SDL_DestroyTexture(Message3);
        SDL_FreeSurface(surfaceMessage);
        SDL_FreeSurface(surfaceMessage2);
        SDL_FreeSurface(surfaceMessage3);
        return;
    }
    SDL_Texture* Message4 = SDL_CreateTextureFromSurface(debug_renderer, surfaceMessage4);
    if (Message4 == NULL) {
        fprintf(stderr, "Error: Failed to create Message for register_string\n");
        SDL_DestroyTexture(Message);
        SDL_DestroyTexture(Message2);
        SDL_DestroyTexture(Message3);
        SDL_FreeSurface(surfaceMessage);
        SDL_FreeSurface(surfaceMessage2);
        SDL_FreeSurface(surfaceMessage3);
        SDL_FreeSurface(surfaceMessage4);
        return;
    }
    SDL_Rect Message_rect4 = (SDL_Rect){0, 25, 450, 30};

    SDL_RenderCopy(debug_renderer, Message, NULL, &Message_rect);
    SDL_RenderCopy(debug_renderer, Message2, NULL, &Message_rect2);
    SDL_RenderCopy(debug_renderer, Message3, NULL, &Message_rect3);
    SDL_RenderCopy(debug_renderer, Message4, NULL, &Message_rect4);

    SDL_SetRenderDrawColor(debug_renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(debug_renderer, 0, 25, 640, 25);
    SDL_RenderDrawLine(debug_renderer, 0, 55, 640, 55);
    SDL_RenderDrawLine(debug_renderer, 0, 450, 640, 450);

    //render controls at the bottom
    char control_string[100] = {0};
    if (sprintf(control_string, "1: Realtime | 2: Instruction | 3: Frame | 4: Cycle | P: palette | Space: PPU REG| 0: Toggle Log %d", clock_print_flag) < 0) {
        fprintf(stderr, "Error: Failed to format control_string\n");
        SDL_DestroyTexture(Message);
        SDL_DestroyTexture(Message2);
        SDL_DestroyTexture(Message3);
        SDL_DestroyTexture(Message4);
        SDL_FreeSurface(surfaceMessage);
        SDL_FreeSurface(surfaceMessage2);
        SDL_FreeSurface(surfaceMessage3);
        SDL_FreeSurface(surfaceMessage4);
        return;
    }
    SDL_Surface* surfaceMessage5 = TTF_RenderText_Solid(font, control_string, White);
    if (surfaceMessage5 == NULL) {
        fprintf(stderr, "Error: Failed to create surfaceMessage for control_string\n");
        SDL_DestroyTexture(Message);
        SDL_DestroyTexture(Message2);
        SDL_DestroyTexture(Message3);
        SDL_DestroyTexture(Message4);
        SDL_FreeSurface(surfaceMessage);
        SDL_FreeSurface(surfaceMessage2);
        SDL_FreeSurface(surfaceMessage3);
        SDL_FreeSurface(surfaceMessage4);
        return;
    }
    SDL_Texture* Message5 = SDL_CreateTextureFromSurface(debug_renderer, surfaceMessage5);
    if (Message5 == NULL) {
        fprintf(stderr, "Error: Failed to create Message for control_string\n");
        SDL_DestroyTexture(Message);
        SDL_DestroyTexture(Message2);
        SDL_DestroyTexture(Message3);
        SDL_DestroyTexture(Message4);
        SDL_FreeSurface(surfaceMessage);
        SDL_FreeSurface(surfaceMessage2);
        SDL_FreeSurface(surfaceMessage3);
        SDL_FreeSurface(surfaceMessage4);
        SDL_FreeSurface(surfaceMessage5);
        return;
    }
    SDL_Rect Message_rect5 = (SDL_Rect){0, 450, 640, 25};

    SDL_RenderCopy(debug_renderer, Message5, NULL, &Message_rect5);

    // Render to screen
    SDL_RenderCopy(debug_renderer, debug_texture, NULL, NULL);
    SDL_RenderPresent(debug_renderer);

    //free
    SDL_FreeSurface(surfaceMessage);
    SDL_FreeSurface(surfaceMessage2);
    SDL_FreeSurface(surfaceMessage3);
    SDL_FreeSurface(surfaceMessage4);
    SDL_FreeSurface(surfaceMessage5);
    SDL_DestroyTexture(Message);
    SDL_DestroyTexture(Message2);
    SDL_DestroyTexture(Message3);
    SDL_DestroyTexture(Message4);
    SDL_DestroyTexture(Message5);
}

void print_ppu_registers()
{
    printf("PPUCTRL: 0x%x\n", ppu_ctrl.reg);
    printf("PPUMASK: 0x%x\n", ppu_mask.reg);
    printf("PPUSTATUS: 0x%x\n", ppu_status.reg);
    printf("OAMADDR: 0x%x\n", OAMADDR);
    printf("OAMDATA: 0x%x\n", OAMDATA);
    printf("PPUSCROLL: 0x%x\n", PPUSCROLL);
    printf("PPUADDR: 0x%x\n", PPUADDR);
    printf("PPUDATA: 0x%x\n", PPUDATA);
}

void printPalettes() {
    // Iterate over palette data in PPU memory

    //0x3F00 - background color
    //0x3F01 - 0x3F03 - background palette 0
    //0x3F05 - 0x3F07 - background palette 1
    //0x3F09 - 0x3F0B - background palette 2
    //0x3F0D - 0x3F0F - background palette 3
    //0x3F11 - 0x3F13 - sprite palette 0
    //0x3F15 - 0x3F17 - sprite palette 1
    //0x3F19 - 0x3F1B - sprite palette 2
    //0x3F1D - 0x3F1F - sprite palette 3
    for (int i = 0; i < 32; i++) {
        uint8_t value = ppu_palette[i];
        printf("Palette[%d], ppuBus[%X] = %02X\n", i, 0x3F00 + i, value);
    }
}

void clock()
{
    //needs to be run at the proper clock cycle to be accurate
    if (cycles == 0) {
        if (run_single_instruction == true && single_instruction_latch == 1)
        {
            single_instruction_latch = 0;
        }
        if (run_single_instruction == true && single_instruction_latch == 0)
        {
            run_single_instruction = false;
            debug_window_flag = true;
            updateFrame();
        }

        instruction_count++;
        //get next opcode
        current_opcode = cpuBus_read(program_counter);
        
        //flag set thing
        set_flag(U_flag, 1);

        //increment program counter
        uint16_t old_program_counter = program_counter;

        //cycles

        opcode op_to_execute = get_opcode(current_opcode);
        cycles = op_to_execute.cycle_count;

        if (clock_print_flag == true)
        {
            uint8_t num_args = op_to_execute.byte_size - 1;
            fprintf(fp, "%04X  %02X ", old_program_counter, current_opcode);

            //print arguments
            for (int i = 0; i < num_args; i++) {
                uint8_t arg = cpuBus_read(program_counter + i);
                fprintf(fp, "%02X ", arg);
            }
            
            if (num_args == 0)
            {
                fprintf(fp, "      ");
            }
            if (num_args == 1)
            {
                fprintf(fp, "   ");
            }
            fprintf(fp, " ");
            fprintf(fp, "%s ", op_to_execute.name);
            fprintf(fp, "                  A:%02X ", accumulator);
            fprintf(fp, "X:%02X ", x_register);
            fprintf(fp, "Y:%02X ", y_register);
            fprintf(fp, "P:%02X ", status_register);
            fprintf(fp, "SP:%02X ", stack_pointer);
            if (ppu_scanline == -1)
            {
                ppu_scanline = 261;//for debug matching
            }
            if (ppu_cycle == 0)
            {
                if (ppu_scanline == 0)
                {
                    fprintf(fp, "PPU:%3d,%3d ", 340, 261);
                } else {
                    fprintf(fp, "PPU:%3d,%3d ", 340, ppu_scanline - 1);
                }
            } else {
                fprintf(fp, "PPU:%3d,%3d ", ppu_cycle - 1, ppu_scanline);
            }
            if (ppu_scanline == 261)
            {
                ppu_scanline = -1;
            }
            fprintf(fp, "CYC:%d\n", total_cycles);
        }

        //execute the instruction, keep track if return 1 as that means add cycle
        uint8_t extra_cycle_addressingMode = op_to_execute.addressing_mode();
        uint8_t extra_cycle_opcode = op_to_execute.opcode();

        //add any necessary cycles
        if ((extra_cycle_opcode == 1) && (extra_cycle_addressingMode == 1))
        {
            cycles++;
        }


        //flag set thing
        set_flag(U_flag, 1);

    }
    //decrement a cycle every clock cycle, we don't have to calculate every cycle as long as the clock is synced in the main function
    cycles--;
    total_cycles++;
}

uint32_t system_clock_count = 0x00;

void bus_clock()
{
    ppu_clock();
    if (system_clock_count % 3 == 0)
    {
        clock();
    }
    if (nmi == true)
    {
        nmi = false;
        non_maskable_interrupt();
    }
    if (run_single_cycle == true)
    {
        run_single_cycle = false;
        updateDebugWindow();
    }
    system_clock_count++;
}

void cpu_and_ram_full_reset()
{
    //set all of ram to 0
    for (int i = 0; i < sizeof(cpuTestRam) / 8; i++)
    {
        cpuTestRam[i] = 0;
    }

    //cpu registers to 0
    program_counter = 0x0000;
    stack_pointer = 0x00;
    accumulator = 0x00;
    x_register = 0x00;
    y_register = 0x00;
    status_register = 0x00;
}

bool cpu_test_suite()
{    
    printf("Begin Cpu Test Suite\n");
    //list of opcodes to test
    uint8_t ops_to_test[256] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 
        0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 
        0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 
        0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 
        0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
    };


    //returns true if all the super cpu tests pass, false otherwise

    //loop over all test files.
    //reset cpu and ram state
    //run test
    //compare test, if anything went wrong return false and exit cpu test suite with false
    //if no errors return true

    for (int iteration = 0; iteration < 3; iteration++)
    {
        cpu_and_ram_full_reset();
        uint8_t current_test_opcode = ops_to_test[iteration];
        char *opcode_name = get_opcode(current_test_opcode).name;
        printf("Opcode Name: %s\n", opcode_name);
        if (opcode_name == "???")
        {
            printf("%sSkipping opcode %02X\n%s", "\x1B[33m", current_test_opcode, "\x1B[0m"); // yellow terminal output
            continue;
        }
        printf("Begin Reading CpuTest_%02X\n", current_test_opcode);
        FILE *cpuTest_00;
        static char buffer[1024*1024*6];
        printf("created buffer\n");
        struct json_object *parsed_json;
        char file_str[100];
        sprintf(file_str, "cpu_tests/%02X.json", current_test_opcode);
        fp = fopen(file_str, "r");
        if (fp == NULL)
        {
            printf("%sTest File Not Found For Opcode %02X\n%s", "\x1B[33m", current_test_opcode, "\x1B[0m"); // yellow terminal output
            continue;
        }
        fread(buffer, 1024*1024*6, 1, fp);
        printf("read file\n");
        fclose(fp);

        parsed_json = json_tokener_parse(buffer);

        struct json_object *test_array;
        struct json_object *single_test_object;

        //loops over all tests in the file
        for (int i = 0; i < json_object_array_length(parsed_json); i++) { //change 1 to json_object_array_length(parsed_json) when done testing individual
            printf("Executing CPU test %d for opcode %02X \n", i, current_test_opcode);

            //reset from last test
            cpu_and_ram_full_reset();

            //import single test from large json file
            single_test_object = json_object_array_get_idx(parsed_json, i);
            struct json_object *test_name;
            struct json_object *test_initial_state;
            struct json_object *test_final_state;
            
            test_name = json_object_object_get(single_test_object, "name");
            test_initial_state = json_object_object_get(single_test_object, "initial");
            test_final_state = json_object_object_get(single_test_object, "final");

            //extract initial

            int initial_pc = json_object_get_int(json_object_object_get(test_initial_state, "pc"));
            int initial_sp = json_object_get_int(json_object_object_get(test_initial_state, "s"));
            int initial_a = json_object_get_int(json_object_object_get(test_initial_state, "a"));
            int initial_x = json_object_get_int(json_object_object_get(test_initial_state, "x"));
            int initial_y = json_object_get_int(json_object_object_get(test_initial_state, "y"));
            int initial_p = json_object_get_int(json_object_object_get(test_initial_state, "p"));
            struct json_object *initial_ram = json_object_object_get(test_initial_state, "ram");

            //extract final

            int final_pc = json_object_get_int(json_object_object_get(test_final_state, "pc"));
            int final_sp = json_object_get_int(json_object_object_get(test_final_state, "s"));
            int final_a = json_object_get_int(json_object_object_get(test_final_state, "a"));
            int final_x = json_object_get_int(json_object_object_get(test_final_state, "x"));
            int final_y = json_object_get_int(json_object_object_get(test_final_state, "y"));
            int final_p = json_object_get_int(json_object_object_get(test_final_state, "p"));
            struct json_object *final_ram = json_object_object_get(test_final_state, "ram");

            //run test
            program_counter = initial_pc;
            stack_pointer = initial_sp;
            accumulator = initial_a;
            x_register = initial_x;
            y_register = initial_y;
            status_register = initial_p;

            //initialize ram for test
            for (int i = 0; i < json_object_array_length(initial_ram); i++)
            {
                struct json_object *initial_ram_entry = json_object_array_get_idx(initial_ram, i);
                int initial_ram_entry_address = json_object_get_int(json_object_array_get_idx(initial_ram_entry, 0));
                int initial_ram_entry_value = json_object_get_int(json_object_array_get_idx(initial_ram_entry, 1));
                cpuBus_write(initial_ram_entry_address, initial_ram_entry_value);
            }

            //run opcode
            opcode op_to_execute = get_opcode(current_test_opcode);
            op_to_execute.addressing_mode();
            op_to_execute.opcode();

            //compare against final state
            if (program_counter != final_pc)
            {
                printf("CPU FAILURE, PC: %d, %d\n", program_counter, final_pc);
                printf("Opcode: %s\n", opcode_name);
                printf("\033[38;5;202mTest that caused error:\n %s\033[0m\n", json_object_get_string(single_test_object));
                return false;
            }
            if (stack_pointer != final_sp)
            {
                printf("CPU FAILURE, SP: %d, %d\n", stack_pointer, final_sp);
                printf("Opcode: %s\n", opcode_name);
                printf("\033[38;5;202mTest that caused error:\n %s\033[0m\n", json_object_get_string(single_test_object));
                return false;
            }
            if (accumulator != final_a)
            {
                printf("CPU FAILURE, A: %d, %d\n", accumulator, final_a);
                printf("Opcode: %s\n", opcode_name);
                printf("\033[38;5;202mTest that caused error:\n %s\033[0m\n", json_object_get_string(single_test_object));
                return false;
            }
            if (x_register != final_x)
            {
                printf("CPU FAILURE, X: %d, %d\n", x_register, final_x);
                printf("Opcode: %s\n", opcode_name);
                printf("\033[38;5;202mTest that caused error:\n %s\033[0m\n", json_object_get_string(single_test_object));
                return false;
            }
            if (y_register != final_y)
            {
                printf("CPU FAILURE, Y: %d, %d\n", y_register, final_y);
                printf("Opcode: %s\n", opcode_name);
                printf("\033[38;5;202mTest that caused error:\n %s\033[0m\n", json_object_get_string(single_test_object));
                return false;
            }
            if (status_register != final_p)
            {
                printf("CPU FAILURE, P: %d, %d\n", status_register, final_p);
                printf("Opcode: %s\n", opcode_name);
                printf("\033[38;5;202mTest that caused error:\n %s\033[0m\n", json_object_get_string(single_test_object));
                return false;
            }

            //ram checks
            for (int i = 0; i < json_object_array_length(final_ram); i++)
            {
                struct json_object *final_ram_entry = json_object_array_get_idx(final_ram, i);
                int final_ram_entry_address = json_object_get_int(json_object_array_get_idx(final_ram_entry, 0));
                int final_ram_entry_value = json_object_get_int(json_object_array_get_idx(final_ram_entry, 1));
                if (cpuBus_read(final_ram_entry_address) != final_ram_entry_value)
                {
                    printf("CPU FAILURE, RAM[0x%d]: 0x%d, 0x%d\n", final_ram_entry_address, cpuBus_read(final_ram_entry_address), final_ram_entry_value);
                    printf("Opcode: %s\n", opcode_name);
                    printf("\033[38;5;202mTest that caused error:\n %s\033[0m\n", json_object_get_string(single_test_object));
                    return false;
                }
            }
        }
    }
    return true;
}


int main(int argc, char* argv[])
{
    //open file for debug
    fp = fopen("debug.txt", "w");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }
    //cpu and ram
    initialize_cpu();
    //load rom at 0x8000, default location
    load_rom("nestest.nes");
    reset();
    
    SDL_Init(SDL_INIT_VIDEO);

    TTF_Init();
    font = TTF_OpenFont("Arimo.ttf", 12);

    window = SDL_CreateWindow("Nes Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * 3, HEIGHT * 3, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    debug_window = SDL_CreateWindow("Debug Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    debug_renderer = SDL_CreateRenderer(debug_window, -1, SDL_RENDERER_ACCELERATED);

    

    // Fill RGB data with example values
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        r[i] = 100;
        g[i] = 100;
        b[i] = 100;
    }

    // Create texture from RGB data
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    debug_texture = SDL_CreateTexture(debug_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 240);
    SDL_SetTextureBlendMode(debug_texture, SDL_BLENDMODE_BLEND);

    // Render initial frame
    updateFrame();
    updateDebugWindow();

        // Main loop
    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                // Exit main loop
                return 0;
            }

            // Check for SDL events
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    for (int i = 0; i < WIDTH * HEIGHT; i++) {
                        r[i] = 0;
                        g[i] = 0;
                        b[i] = 0;
                    }
                    //print ppu registers
                    print_ppu_registers();
                    updateFrame();
                }
                if (event.key.keysym.sym == SDLK_p) {
                    printPalettes();
                    selected_palette++;
                    if (selected_palette > 7) {
                        selected_palette = 0;
                    }
                    updateFrame();
                }
                if (event.key.keysym.sym == SDLK_1) {
                    //fullspeed toggle
                    fullspeed = !fullspeed;
                }
                if (event.key.keysym.sym == SDLK_2) {
                    //run single instruction
                    run_single_instruction = true;
                    single_instruction_latch = 1;
                }
                if (event.key.keysym.sym == SDLK_3) {
                    //run single frame
                    run_single_frame = true;
                }
                if (event.key.keysym.sym == SDLK_4) {
                    //run single cycle
                    run_single_cycle = true;
                }
                if (event.key.keysym.sym == SDLK_0) {
                    //toggle log
                    if (clock_print_flag == 1)
                    {
                        clock_print_flag = 0;
                    }
                    else if (clock_print_flag == 0)
                    {
                        clock_print_flag = 1;
                    }
                    updateDebugWindow();
                }
                if (event.key.keysym.sym == SDLK_t) {
                    //execute rigorous cpu tests
                    printf("Start CPU tests\n");
                    cpu_test_mode = true;
                    cpu_test_suite() ? printf("\033[0;32mAll CPU tests passed\033[0m\n") : printf("\033[0;31mCPU tests failed\033[0m\n");
                    cpu_test_mode = false;
                }
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    // Exit main loop
                    return 0;
                }
            }
            //maintain aspect ratio
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                // Calculate new window size while maintaining aspect ratio
                int newWidth = event.window.data1;
                int newHeight = (newWidth * HEIGHT) / WIDTH;
                SDL_SetWindowSize(window, newWidth, newHeight);
            }
        }
        // Run clock and update frame
        if (fullspeed || run_single_cycle || run_single_frame || run_single_instruction)
        {
            bus_clock();
            if (debug_window_flag)
            {
                updateDebugWindow();
                debug_window_flag = false;
            }
            if (frame_complete) {
                updateFrame();
                frame_count++;
                printf("Frame: %d\n", frame_count);
                frame_complete = false;
                run_single_frame = false;
                debug_window_flag = true;
                updateDebugWindow();
            }
        }
    }

    // Clean up resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyWindow(debug_window);
    SDL_DestroyRenderer(debug_renderer);
    TTF_Quit();
    SDL_Quit();

    return 0;
}