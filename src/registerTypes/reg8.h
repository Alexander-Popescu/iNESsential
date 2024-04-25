#include <cstdint>

class REG8 {
    public:
        REG8();

        //bassically I want to have struct-level access but also read / write to the ppu registers so heres this wrapper thing

        uint8_t getValueRange(uint8_t msb, uint8_t lsb);
        void setValueRange(uint8_t msb, uint8_t lsb, uint8_t value);
        uint8_t getValue();
        void setValue(uint8_t value);
    
    private:
        uint8_t value;
};