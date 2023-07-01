#ifndef SQUARE1EMULATOR_FONTCONFIG_H
#define SQUARE1EMULATOR_FONTCONFIG_H

#include <cstdint>

#define SQ_FONT_CONCAT(A, B) A ## B
#define SQ_FONT_LARGE_ARRAY_DECLARE(type, name) const type name[]
//#define SQ_FONT_LARGE_ARRAY_DECLARE(type, name) const type name[] PROGMEM
#define SQ_FONT_DESCRIPTOR(font_name) SQ_FONT_LARGE_ARRAY_DECLARE(sq::FontSubtableDescriptor, SQ_FONT_CONCAT(font_name, _desc))
#define SQ_FONT_GLYPH(font_name) SQ_FONT_LARGE_ARRAY_DECLARE(uint8_t, SQ_FONT_CONCAT(font_name, _glyph))
#define SQ_FONT_DECLARE(font_name, subtable_num) const sq::Font font_name = {subtable_num, SQ_FONT_CONCAT(font_name, _desc), SQ_FONT_CONCAT(font_name, _glyph)}

namespace sq {
    typedef uint16_t Unicode;

    struct FontSubtableDescriptor {
        Unicode unicodeStart;
        Unicode unicodeNum;
        uint32_t subtableAddress;
    };

    typedef const FontSubtableDescriptor* FontSubtableDescriptorArray;
    typedef const uint8_t* FontGlyphSubtable;

    struct Font {
        uint16_t subtableNum;
        FontSubtableDescriptorArray subtableDescriptors;
        FontGlyphSubtable glyphSubtable;
    };
}

#endif //SQUARE1EMULATOR_FONTCONFIG_H
