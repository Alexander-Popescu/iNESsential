#include "shiftReg.h"

SHIFTREG::SHIFTREG()
{
    lowWord = 0;
    highWord = 0;
}

void SHIFTREG::shift()
{
    lowWord = lowWord << 1;
    highWord = highWord << 1;
}

void SHIFTREG::loadNextTile(TileInfo next_tile, char shiftType)
{
    if (shiftType == 'p') {
        lowWord |= next_tile.lsb;
        highWord |= next_tile.msb;
    } else if (shiftType == 'a') {
        //expands attribute bit to full byte
        lowWord  |= ((next_tile.attribute & 0b01) ? 0xFF : 0x00);
        highWord  |= ((next_tile.attribute & 0b10) ? 0xFF : 0x00);
    }
}