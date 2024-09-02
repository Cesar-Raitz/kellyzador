/**
   Arduino Electronic Safe

   Copyright (C) 2020, Uri Shaked.
   Released under the MIT License.
*/

#ifndef ICONS_H
#define ICONS_H

#include <LiquidCrystal.h>

enum {
   ICON_SMILEY,
   ICON_LOCKED_CHAR,
   ICON_UNLOCKED_CHAR,
   ICON_RIGHT_ARROW = 126,
};

void init_icons(LiquidCrystal &lcd);

#endif // ICONS_H