#include "PPU.h"
#include "../Emulator.h"
#include <cstdlib>
#include <string.h>

PPU::PPU(Emulator *emulator) {
    this->emulator = emulator;
    cycle = 0;
    scanline = 0;
}

PPU::~PPU() {
    
}

void PPU::reset() {
    cycle = 0;
    scanline = -1;

    for (int i = 0; i < 1024; i++) {
        nameTables[0][i] = 0;
        nameTables[1][i] = 0;
    }
    for (int i = 0; i < 32; i++) {
        palettes[i] = 0;
    }
	for (int i = 0; i < 64; i++) {
		OAM[i] = {0,0,0,0};
	}
	OAMADDR = 0;

	PPUCTRL.setValue(0);
	PPUMASK.setValue(0);
	PPUSTATUS.setValue(0);

	vramAddress.setValue(0);
	tempVramAddress.setValue(0);

	fineXScroll = 0x00;

	writeToggle = 0;
	readBuffer = 0x00;

	next_tile = {0,0,0,0};
	
	shiftPattern.lowWord = 0;
	shiftPattern.highWord = 0;
	shiftAttribute.lowWord = 0;
	shiftAttribute.highWord = 0;
}

bool PPU::clock() {
    //returns true if frame is ready to be updated on the screen
	if (scanline == -1 && cycle == 1)
	{
		//clear vblank
		PPUSTATUS.setValueRange(7,7,0);

		//sprite overflow flag
		PPUSTATUS.setValueRange(5,5,0);
		
		//sprite zero hit
		PPUSTATUS.setValueRange(6,6,0);

		for (int i = 0; i < 8; i++)
		{
			spriteLowShiftReg[i] = 0;
			spriteHighShiftReg[i] = 0;
		}
	}

	if (scanline >= -1 && scanline < 240)
	{
		//visible section of rendering, 8 pixel tile info loop
		visiblePixelInfoCycle();
	}

	//foreground
	if (cycle == 257 && scanline >= 0)
	{
		//get number of pixels, clear oam variables, etc
		prepareScanlineSpriteInfo();
	}

	if (cycle == 340)
	{
		//prepares sprite info for rendering
		updateSpriteShiftRegs();
	}

	if (scanline == 241 && cycle == 1)
	{
		//vblank
		PPUSTATUS.setValueRange(7,7,1 << 7);

		if (PPUCTRL.getValueRange(7,7)) 
			emulator->cpuNMI();
	}
	
	if (scanline >= 0 && scanline < 240 && cycle < 256) {
		//calculate background pixel color and render it
		uint8_t backgroundPixelIndex = 0;
		uint8_t backgroundPaletteIndex = 0;
		getBackgroundPixelColor(&backgroundPixelIndex, &backgroundPaletteIndex);

		uint8_t foregroundPixelIndex = 0;
		uint8_t foregroundPaletteIndex = 0;
		uint8_t foregroundPriority = 0;
		getForegroundPixelColor(&foregroundPixelIndex, &foregroundPaletteIndex, &foregroundPriority);

		uint8_t pixel = backgroundPixelIndex;
		uint8_t palette = backgroundPaletteIndex;

		//if foreground isnt transparent, render that
		if (foregroundPixelIndex > 0) {
			pixel = foregroundPixelIndex;
			palette = foregroundPaletteIndex;
		}

		emulator->pixelBuffer->writeBufferPixel(cycle - 1, scanline, paletteTranslationTable[emulator->ppuBusRead(0x3F00 + (palette << 2) + pixel)]);
	}

	cycle++;
	if (cycle >= 341)
	{
		cycle = 0;
		scanline++;
		if (scanline == 1)
		{
			//hardware bug
			cycle = 1;
		}
		if (scanline >= 261)
		{
			scanline = -1;
			emulator->frameCount++;
			return true;
		}
	}


    return false;
}

void PPU::getBackgroundPixelColor(uint8_t *index, uint8_t *palette) {
	if (PPUMASK.getValueRange(3,3))
	{
		uint16_t mask = 1 << (15 - fineXScroll);

		//mask shift data to get pixel and palette for this cycle
		*index = ((bool)(mask & shiftPattern.highWord) << 1) | (bool)(mask & shiftPattern.lowWord);
		*palette = ((bool)(mask & shiftAttribute.highWord) << 1) | (bool)(mask & shiftAttribute.lowWord);
	}
}

void PPU::getForegroundPixelColor(uint8_t *foregroundPixelIndex, uint8_t *foregroundPaletteIndex, uint8_t *foregroundPriority) {
	if ((PPUMASK.getValueRange(4,4) >> 4))
	{
		//check all sprites
		for (uint8_t i = 0; i < spriteCount; i++)
		{
			//this means its time to render
			if (spriteInfoBuffer[i].x == 0) 
			{
				*foregroundPixelIndex = (((spriteHighShiftReg[i] & 0x80) >> 7) << 1) | ((spriteLowShiftReg[i] & 0x80) >> 7);
				*foregroundPaletteIndex = (spriteInfoBuffer[i].attributes & 0x03) + 0x04;
				*foregroundPriority = (spriteInfoBuffer[i].attributes & 0x20) == 0;

				if (*foregroundPixelIndex != 0)
				{
					if (i == 0) PPUSTATUS.setValueRange(6,6, 1 << 6);
					break;
				}				
			}
		}		
	}
}

void PPU::visiblePixelInfoCycle() {
	//the pull 8 bits worth of render information cycle that occurs during visible (and prerender) scanalines

	if ((cycle >= 2 && cycle < 258) || (cycle >= 321 && cycle < 338))
	{
		//shift registers
		if (PPUMASK.getValueRange(3,3))
		{
			shiftPattern.shift();
			shiftAttribute.shift();

			if ((PPUMASK.getValueRange(4,4) >> 4) && cycle >= 1 && cycle < 258)
			{
				for (int i = 0; i < spriteCount; i++)
				{
					spriteInfoBuffer[i].x > 0 ? spriteInfoBuffer[i].x-- : (spriteLowShiftReg[i] <<= 1, spriteHighShiftReg[i] <<= 1);
				}
			}
		}
		//different values of the nextTile information are loaded depending on how deep we are in the loop
		loadTileInfo((cycle - 1) % 8);
	}

	if (cycle == 256)
	{
		//rendering check
		if (PPUMASK.getValueRange(3,3) || PPUMASK.getValueRange(4,4)) {
			//scanline is done so move y
			vramAddress.incrementAddress('y');
		}
	}

	if (cycle == 257)
	{
		//load next tile information into shift regs
		shiftPattern.loadNextTile(next_tile,'p');
		shiftAttribute.loadNextTile(next_tile,'a');

		//if rendering is enabled (For  background or for sprites)
		if (PPUMASK.getValueRange(3,3) || PPUMASK.getValueRange(4,4)) {
			//hblank, update v register
			vramAddress.syncAddress(tempVramAddress, 'x');
		}
	}

	if (cycle == 338 || cycle == 340)
	{
		next_tile.id = emulator->ppuBusRead(0x2000 | (vramAddress.getValue() & 0x0FFF));
	}

	if (scanline == -1 && cycle >= 280 && cycle < 305)
	{
		//if rendering is enabled
		if (PPUMASK.getValueRange(3,3) || PPUMASK.getValueRange(4,4)) {
			//update y information
			vramAddress.syncAddress(tempVramAddress, 'y');
		}
	}
}

void PPU::prepareScanlineSpriteInfo() {
	//reset sprite variables
	for (int i = 0; i < 8; i++)
	{
		spriteInfoBuffer[i] = {0,0,0,0};
	}

	spriteCount = 0;

	for (uint8_t i = 0; i < 8; i++)
	{
		spriteLowShiftReg[i] = 0;
		spriteHighShiftReg[i] = 0;
	}

	for (uint8_t i = 0; i < 64; i++)
	{
		//max sprite overflow
		if (spriteCount > 8) break;

		//check if sprite is between scanline and scanline + height of sprite
		if ((scanline - OAM[i].y) >= 0 && (scanline - OAM[i].y) < ((PPUCTRL.getValueRange(5,5) >> 5) ? 16 : 8))
		{
			spriteInfoBuffer[spriteCount] = OAM[i];
			spriteCount++;
		}
	}

	PPUSTATUS.setValueRange(5,5,(spriteCount > 8));
}

void PPU::updateSpriteShiftRegs() {
	uint8_t spriteIndex = 0;
    while (spriteIndex < spriteCount) {
        OAMentry &currentSprite = spriteInfoBuffer[spriteIndex];

		//calculate where current sprite info is
        uint16_t spriteLowAddress = ((PPUCTRL.getValueRange(3,3) >> 3) << 12) | (currentSprite.spriteIndex << 4) | ((currentSprite.attributes & 0x80) ? 7 - (scanline - currentSprite.y) : scanline - currentSprite.y);
        uint16_t spriteHighAddress = spriteLowAddress + 8;
		
		//load into shifters
        spriteLowShiftReg[spriteIndex] = emulator->ppuBusRead(spriteLowAddress);
        spriteHighShiftReg[spriteIndex] = emulator->ppuBusRead(spriteHighAddress);

		//flip if attribute says so
        if (currentSprite.attributes & 0x40) {
            uint8_t lowFlip = 0;
            uint8_t highFlip = 0;
            for (int i = 0; i < 8; i++) {
                lowFlip |= ((spriteLowShiftReg[spriteIndex] >> i) & 1) << (7 - i);
                highFlip |= ((spriteHighShiftReg[spriteIndex] >> i) & 1) << (7 - i);
            }
            spriteLowShiftReg[spriteIndex] = lowFlip;
            spriteHighShiftReg[spriteIndex] = highFlip;
        }

        spriteIndex++;
    }
}

void PPU::loadTileInfo(uint8_t step) {
	if (step == 0) {
		//load namertable byte

		//load next tile information into shift regs
		shiftPattern.loadNextTile(next_tile,'p');
		shiftAttribute.loadNextTile(next_tile,'a');

		next_tile.id = emulator->ppuBusRead(0x2000 | (vramAddress.getValue() & 0x0FFF));

	} else if (step == 2) {

		//load attribute table byte
		next_tile.attribute = emulator->ppuBusRead(0x23C0 | (vramAddress.getValue() & 0x0C00) | ((vramAddress.getValue() & 0x0380) >> 4) | ((vramAddress.getValue() & 0x001C) >> 2));
		next_tile.attribute >>= ((vramAddress.getValueRange(5,9) & 0x40) ? 4 : 0);
		next_tile.attribute >>= ((vramAddress.getValueRange(0,4) & 0x02) ? 2 : 0);
		next_tile.attribute &= 0b11;

	} else if (step == 4) {

		//load background pattern table lsb
		next_tile.lsb = emulator->ppuBusRead(((PPUCTRL.getValueRange(4,4) >> 4) * 0x1000) + (next_tile.id << 4) + (vramAddress.getValueRange(12,14) >> 12));
	
	} else if (step == 6) {

		//background pattern table msb
		next_tile.msb = emulator->ppuBusRead(((PPUCTRL.getValueRange(4,4) >> 4) * 0x1000) + (next_tile.id << 4) + (vramAddress.getValueRange(12,14) >> 12) + 8);
	
	} else if (step == 7) {

		//rendering enable check
		if (PPUMASK.getValueRange(3,3) || PPUMASK.getValueRange(4,4)) {
			//move to next tile
			vramAddress.incrementAddress('x');
		}
	}
}

uint8_t PPU::readRegisters(uint16_t address) {
    uint8_t data = 0x00;
    switch (address)
		{
		case 0x2000: break;
			//cant read PPUCTRL
		case 0x2001: break;
			//cant read PPUMASK
		case 0x2002:
			// Status
            data = PPUSTATUS.getValue();

            //status write side effects 
			PPUSTATUS.setValueRange(7,7,1 << 7);
			writeToggle = 0;
			break;
		case 0x2003: break;
			//cant read OAM Address
		case 0x2004: break;
			data = *((uint8_t*)OAM + OAMADDR);
			// OAM Data
		case 0x2005: break;
			//cant read PPUSCROLL
		case 0x2006: break;
			//cant read PPUADDR
		case 0x2007: 
			// PPU D ata
			data = readBuffer;
			readBuffer = emulator->ppuBusRead(vramAddress.getValue());
			if (vramAddress.getValue() >= 0x3F00) data = readBuffer;

            //documented increment breaks emulator, no idea why, so skipping
			break;
		}
    return data;
}

void PPU::writeRegisters(uint16_t address, uint8_t data) {
    switch (address)
	{
	case 0x2000:
        //PPUCTRL
		PPUCTRL.setValue(data);
		tempVramAddress.setValueRange(10,10,PPUCTRL.getValueRange(0,0) << 10);
		tempVramAddress.setValueRange(11,11,PPUCTRL.getValueRange(1,1) << 10);
		break;
	case 0x2001:
        //PPUMASK
		PPUMASK.setValue(data);
		break;
	case 0x2002:
        //PPUSTATUS
		break;
	case 0x2003:
        //OAMADDR
		OAMADDR = data;
		break;
	case 0x2004:
        //OAMDATA
		*((uint8_t*)OAM + OAMADDR) = data;
		break;
	case 0x2005:
        //PPUSCROLL
		if (writeToggle == 0)
		{
			fineXScroll = data & 0x07;
			tempVramAddress.setValueRange(0,4,data >> 3);
			writeToggle = 1;
		}
		else
		{
			tempVramAddress.setValueRange(12,14, (data & 0x07) << 12);
			tempVramAddress.setValueRange(5,9,(data >> 3) << 5);
			writeToggle = 0;
		}
		break;
	case 0x2006:
        //PPUADDR
		if (writeToggle == 0)
		{
			tempVramAddress.setValue((uint16_t)((data & 0x3F) << 8) | (tempVramAddress.getValue() & 0x00FF));
			writeToggle = 1;
		}
		else
		{
			tempVramAddress.setValue((tempVramAddress.getValue() & 0xFF00) | data);
            vramAddress.setValue(tempVramAddress.getValue());
            writeToggle = 0;
		}
		break;
	case 0x2007:
        //PPUDATA
		emulator->ppuBusWrite(vramAddress.getValue(), data);
		vramAddress.setValue(vramAddress.getValue() + (PPUCTRL.getValueRange(2,2) ? 32 : 1));
		break;
	}
}