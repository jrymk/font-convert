# font-convert
Convert fonts to mono bitmap fonts in C files for embedded systems.

Copy FreeType source to root. Edit CMakeLists.txt accordingly if it is not ./freetype-2.9

Pure SFML without other UI libraries. Simple load the CMake project, it will fetch SFML automatically.

![image](https://github.com/jrymk/font-convert/assets/39593345/a92dbc20-1809-466d-8846-2b96e216efb3)

![image](https://github.com/jrymk/font-convert/assets/39593345/8d38001f-4da5-4bb8-ad10-1c8bb164fc42)

I tried using LVGLs nice font converter, but the browser version hangs when converting CJK fonts, and the LVGL font files are not really intended for use outside of LVGL, with tons of LVGL specific defines and dependencies. Therefore I decided to just write my own.\
For reference, HarmonyOS Sans TC Thin rendered at 20 pixels tall takes 846KB of space for all the 14753 glyphs. There is no compression, but LVGL also doesn't have it for 1bpp fonts.

### Glyph reading
```cpp
for (auto ch: str) {
    // binary search the character map range
    auto subtableDesc = std::lower_bound(output.subtableDescriptors, output.subtableDescriptors + output.subtableNum, ch,
                                         [](const sq::FontSubtableDescriptor& desc, const uint16_t& ch) {
                                             return ch > desc.unicodeStart + desc.unicodeNum - 1;
                                         });
    // replace with "missing glyph" (usally a square or a question mark) if not found
    if (subtableDesc == output.subtableDescriptors + output.subtableNum || subtableDesc->unicodeStart > ch) {
        ch = 0;
        subtableDesc = output.subtableDescriptors;
    }
    auto offset = ch - subtableDesc->unicodeStart;
    uint32_t glyphAddr =
            subtableDesc->subtableAddress + 2 * subtableDesc->unicodeNum + output.glyphSubtable[subtableDesc->subtableAddress + 2 * offset] * 256 +
            output.glyphSubtable[subtableDesc->subtableAddress + 2 * offset + 1];
    // draw the bitmap
    int width = output.glyphSubtable[glyphAddr + 2];
    int rows = output.glyphSubtable[glyphAddr + 3];
    int pitch = (width + 7) / 8;
    const uint8_t* bitmap = output.glyphSubtable + glyphAddr + 6;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < width; x++) {
            display[x0 + int8_t(output.glyphSubtable[glyphAddr + 0]) + x][y0 + -int8_t(output.glyphSubtable[glyphAddr + 1]) + y] = (
                    (bitmap[x >> 3] >> (7 - (x & 0b111))) & 0b1);
        }
        bitmap += pitch;
    }
    x0 += int8_t(output.glyphSubtable[glyphAddr + 4]);
    y0 += int8_t(output.glyphSubtable[glyphAddr + 5]);
}
```

### Output example
```cpp
#ifndef BARLOW_20_H
#define BARLOW_20_H

#include "font.h"

const sq::FontSubtableDescriptor barlow_20_desc[] = {
	{0x0000,     1, 0x00000000},
	{0x000D,     1, 0x00000013},
	{0x0020,    95, 0x0000001B},
	{0x00A0,   223, 0x000006CB},
	{0x018F,     4, 0x00001809},
	{0x01A0,     2, 0x00001834},
	{0x01AF,     2, 0x00001866},
	{0x01CD,     2, 0x00001897},
	{0x01D4,     1, 0x000018C0},
    ...
	{0x215B,     4, 0x00002710},
	{0x2202,     5, 0x0000278C},
	{0x220F,    16, 0x000027C4},
	{0x222B,     1, 0x00002866},
	{0x2248,     1, 0x0000287D},
	{0x2260,     6, 0x0000288A},
	{0x25CA,     1, 0x000028C1},
	{0x27E9,     1, 0x000028D6},
	{0xFB01,     2, 0x000028EA},
};

const uint8_t barlow_20_glyph[] = {
    0x00, 0x00, 0x00, 0x0B, 0x06, 0x0B, 0x06, 0x00, 0xFC, 0xCC, 0xB4, 0xBC, 0xF4, 0xFC, 0xEC, 0xFC,
    0xFC, 0xEC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
    0x80, 0x44, 0x78, 0x20, 0x20, 0x30, 0x02, 0x0E, 0x07, 0x0E, 0x0A, 0x00, 0x28, 0x10, 0x00, 0xFE,
    0x80, 0x80, 0x80, 0x80, 0xF8, 0x80, 0x80, 0x80, 0x80, 0xFE, 0x01, 0x0B, 0x06, 0x0B, 0x08, 0x00,
    0x28, 0x10, 0x00, 0x78, 0x84, 0x84, 0xFC, 0x80, 0x80, 0x44, 0x78, 0x01, 0x0E, 0x08, 0x0E, 0x0A,
    0x00, 0x10, 0x28, 0x00, 0x3C, 0x42, 0x81, 0x80, 0x80, 0x8F, 0x81, 0x81, 0x81, 0x42, 0x3C, 0x01,
    0x0B, 0x06, 0x0E, 0x08, 0x00, 0x10, 0x68, 0x00, 0x7C, 0xC4, 0x84, 0x84, 0x84, 0x84, 0xC4, 0x7C,
    0x04, 0x04, 0x38, 0x01, 0x0E, 0x08, 0x0E, 0x0A, 0x00, 0x24, 0x18, 0x00, 0x3C, 0x42, 0x81, 0x80,
    0x80, 0x8F, 0x81, 0x81, 0x81, 0x42, 0x3C, 0x01, 0x0B, 0x06, 0x0E, 0x08, 0x00, 0x48, 0x30, 0x00,
    ...
    0x8F, 0x81, 0x81, 0x81, 0x42, 0x3C, 0x00, 0x10, 0x10, 0x01, 0x0B, 0x06, 0x0E, 0x08, 0x00, 0x10,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x0D, 0x04, 0x0D, 0x04, 0x00, 0xF0, 0x00, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x0A, 0x04, 0x0A, 0x03, 0x00,
    0xF0, 0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x0B, 0x03, 0x0B, 0x03, 0x00,
    0xFC, 0x00, 0x00, 0x01, 0x0C, 0x06, 0x0D, 0x08, 0x00, 0x20, 0x30, 0x50, 0x48, 0x48, 0x88, 0x84,
    0x84, 0x88, 0x48, 0x50, 0x30, 0x20, 0x00, 0x00, 0x02, 0x0B, 0x03, 0x0C, 0x06, 0x00, 0x80, 0x80,
    0x80, 0x40, 0x40, 0x20, 0x20, 0x40, 0x40, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x11, 0x01, 0x0B,
    0x07, 0x0B, 0x0A, 0x00, 0x72, 0x42, 0x40, 0xF2, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x01,
    0x0B, 0x07, 0x0B, 0x09, 0x00, 0x72, 0x42, 0x42, 0xF2, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
};

const sq::Font barlow_20 = {
	50,
	barlow_20_desc,
	barlow_20_glyph
};

#endif

```
