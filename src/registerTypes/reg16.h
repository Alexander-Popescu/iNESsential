#include <cstdint>

class REG16 {
    public:
        REG16();

        //reg8 but for the vramAddress(s)

        uint16_t getValueRange(uint8_t lsb, uint8_t msb);
        void setValueRange(uint8_t lsb, uint8_t msb, uint16_t value);
        uint16_t getValue();
        void setValue(uint16_t value);

        //functions used in the ppu emulation
        void syncAddress(REG16 tempVramAddress, char axis);
        void incrementAddress(char axis);
    
    private:
        uint16_t value;
};