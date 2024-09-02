/**
Arduino Electronic Safe

Copyright (C) 2020, Uri Shaked.
Released under the MIT License.
*/

#include <Arduino.h>
#include "icons.h"

const byte smiley[8] PROGMEM = {
	0b00000,
	0b01010,
	0b01010,
	0b00000,
	0b10001,
	0b01110,
	0b00000,
	0b00000
};

const byte locked[8] PROGMEM = {
	0b01110,
	0b10001,
	0b10001,
	0b11111,
	0b11011,
	0b11011,
	0b11111,
};

const byte unlocked[8] PROGMEM = {
	0b01110,
	0b10000,
	0b10000,
	0b11111,
	0b11011,
	0b11011,
	0b11111,
};

void init_icons(LiquidCrystal &lcd) {
	byte icon[8];
	memcpy_P(icon, smiley, sizeof(icon));
	lcd.createChar(ICON_SMILEY, icon);
	memcpy_P(icon, locked, sizeof(icon));
	lcd.createChar(ICON_LOCKED_CHAR, icon);
	memcpy_P(icon, unlocked, sizeof(icon));
	lcd.createChar(ICON_UNLOCKED_CHAR, icon);
}
