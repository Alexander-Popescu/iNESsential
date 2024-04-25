#include "reg8.h"

REG8::REG8()
{
    value = 0;
}

uint8_t REG8::getValueRange(uint8_t msb, uint8_t lsb)
{
    //specify start and end bit, returns a uint8_t of all zeros but with the range being the value of the register
    uint8_t data = 0;
    for(int i = lsb; i <= msb; i++)
    {
        data |= (value & (1 << i));
    }
    return data;
}

void REG8::setValueRange(uint8_t msb, uint8_t lsb, uint8_t setValue)
{
    //same idea as the getValueRange but setting the range, input is assummed to be in the specified range of the input
    for(int i = lsb; i <= msb; i++)
    {
        value &= ~(1 << i);
    }
    for (int i = lsb; i <= msb; i++)
    {
        value |= (setValue & (1 << i));
    }
}

uint8_t REG8::getValue()
{
    return value;
}

void REG8::setValue(uint8_t setValue)
{
    value = setValue;
}
