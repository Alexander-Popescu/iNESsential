#include "reg16.h"

REG16::REG16()
{
    value = 0;
}

uint16_t REG16::getValueRange(uint8_t lsb, uint8_t msb)
{
    //specify start and end bit, returns a uint8_t of all zeros but with the range being the value of the register
    uint16_t data = 0;
    for(int i = lsb; i <= msb; i++)
    {
        data |= (value & (1 << i));
    }
    return data;
}

void REG16::setValueRange(uint8_t lsb, uint8_t msb, uint16_t setValue)
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

uint16_t REG16::getValue()
{
    return value;
}

void REG16::setValue(uint16_t setValue)
{
    value = setValue;
}

void REG16::syncAddress(REG16 tempVramAddress, char axis)
{
    //syncs different parts of the PPU reg16s
    if(axis == 'x')
    {
        //sync coarse_x and nametable x
        setValueRange(10,10,tempVramAddress.getValueRange(10,10));
		setValueRange(0,4,tempVramAddress.getValueRange(0,4));
    }
    else if(axis == 'y')
    {
        //same for y, includes fine y
        setValueRange(12,14,tempVramAddress.getValueRange(12,14));
        setValueRange(11,11,tempVramAddress.getValueRange(11,11));
        setValueRange(5,9,tempVramAddress.getValueRange(5,9));
    }
}

void REG16::incrementAddress(char axis)
{
    //increments the address, if it overflows, it wraps around
    if(axis == 'x')
    {
        if (getValueRange(0,4) == 31)
        {
            //handle overflow
            setValueRange(0,4,0);
            setValueRange(10,10,~getValueRange(10,10));
        }
        else
        {
            //otherwise easy increment for coarseX
            setValueRange(0,4,getValueRange(0,4) + 1);
        }
    }
    else if (axis == 'y')
    {
        if ((getValueRange(12,14) >> 12) < 7)
			{
                //no overflow case
				setValueRange(12,14,getValueRange(12,14) + (1 << 12));
			}
			else
			{
				setValueRange(12,14,0);
                
                //increment coarse y
				if ((getValueRange(5,9) >> 5) == 29)
				{
					setValueRange(5,9,0);
					setValueRange(11,11,~getValueRange(11,11));
				}
				else
				{
                    //coarse increment
					setValueRange(5,9,getValueRange(5,9) + (1 << 5));
				}
			}
		}
}