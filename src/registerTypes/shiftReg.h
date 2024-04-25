#include <cstdint>
#include "../Definitions.h"

class SHIFTREG {
    public:
        SHIFTREG();
        void shift();

        //shift type is either a for attribute or p for pattern
        void loadNextTile(TileInfo next_tile, char shiftType);

        //shift registers used in PPU

        uint16_t lowWord;
        uint16_t highWord;
};