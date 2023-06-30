#ifndef SQUARE1EMULATOR_FONT_H
#define SQUARE1EMULATOR_FONT_H

#include <cstdint>

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

#endif //SQUARE1EMULATOR_FONT_H
