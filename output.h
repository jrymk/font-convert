#ifndef FONTCONVERT_OUTPUT_H
#define FONTCONVERT_OUTPUT_H

#include <iostream>
#include <fstream>
#include <fcntl.h>

#include "unicode.h"
#include "ft.h"

void printBinary(uint8_t b) {
    for (int i = 0; i < 8; i++) {
        std::cout << ((b & 0b10000000) ? '1' : '0');
        b <<= 1;
    }
    std::cout << " ";
}

void printBinary(uint32_t b) {
    for (int i = 0; i < 32; i++) {
        std::cout << ((b & 0b10000000000000000000000000000000) ? '1' : '0');
        b <<= 1;
    }
    std::cout << " ";
}

struct SubtableDescriptor {
    uint16_t unicodeStart;
    uint16_t unicodeNum;
    uint32_t subtableAddr;

    SubtableDescriptor(uint16_t unicodeStart,
                       uint16_t unicodeNum,
                       uint32_t subtableAddr) : unicodeStart(unicodeStart), unicodeNum(unicodeNum), subtableAddr(subtableAddr) {}
};

void writeFile(const std::string& fontPath, int pixelHeight) {
    std::ofstream file;
    file.open("output.bin", std::ios::out | std::ios::binary);

    FT_Face face;
    auto error = FT_New_Face(library, fontPath.c_str(), 0, &face);
    if (error) std::cerr << "FT_New_Face error: " << getErrorString(error) << "\n";
    error = FT_Set_Pixel_Sizes(face, 0, pixelHeight);
    if (error) std::cerr << "FT_New_Face error: " << getErrorString(error) << "\n";

    uint32_t ch = 0x0000;
    uint16_t cmapSubtableNum = 0;
    std::vector<uint8_t> glyphs;
    uint16_t cmapSubtableUnicodeStart, cmapSubtableUnicodeNum;
    std::vector<uint8_t> cmapSubtable;
    glyphs.reserve(65536);
    std::vector<uint8_t> cmapSubtableDesc;
    std::vector<SubtableDescriptor> cmapSubtableDescs;
    std::vector<uint8_t> cmapSubtables;
    uint32_t cmapAddrRel = 0;

    while (true) {
        // advance to next non-0 glyph
        auto prevCh = ch - 1;
        unsigned int glyphIndex = 0;
        while (ch <= 0xFFFF) {
            if (ch == 0)
                break; // unicode 0 is now the missing glyph
            glyphIndex = FT_Get_Char_Index(face, ch);
            if (glyphIndex) break;
            ch++;
        }
        if (ch > 0xFFFF) {
            cmapSubtableDesc.emplace_back(cmapSubtableUnicodeStart >> 8);
            cmapSubtableDesc.emplace_back(cmapSubtableUnicodeStart);
            cmapSubtableDesc.emplace_back(cmapSubtableUnicodeNum >> 8);
            cmapSubtableDesc.emplace_back(cmapSubtableUnicodeNum);
            cmapSubtableDesc.emplace_back(cmapAddrRel >> 24);
            cmapSubtableDesc.emplace_back(cmapAddrRel >> 16);
            cmapSubtableDesc.emplace_back(cmapAddrRel >> 8);
            cmapSubtableDesc.emplace_back(cmapAddrRel);
            cmapSubtableDescs.emplace_back(cmapSubtableUnicodeStart, cmapSubtableUnicodeNum, cmapAddrRel);
            cmapSubtables.insert(cmapSubtables.end(), cmapSubtable.begin(), cmapSubtable.end());
            cmapSubtables.insert(cmapSubtables.end(), glyphs.begin(), glyphs.end());
            break;
        }

        // render glyph
        error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
        if (error) std::cerr << "FT_Load_Glyph error: " << getErrorString(error) << "\n";
        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
        if (error) std::cerr << "FT_Render_Glyph error: " << getErrorString(error) << "\n";
        size_t chGlyphSize = 6 + (face->glyph->bitmap.width + 7) / 8 * face->glyph->bitmap.rows;
        // decide new cmap subtable or pad existing
        if (cmapSubtableNum == 0 || glyphs.size() + chGlyphSize > 65536 || (ch - prevCh - 1) > 4) {
            // new cmap subtable
            if (cmapSubtableNum > 0) {
                std::cerr << ((cmapSubtableUnicodeStart) & 0xFF) << ", " << cmapSubtableUnicodeNum << "\n";
                cmapSubtableDesc.push_back((cmapSubtableUnicodeStart >> 8) & 0xFF);
                cmapSubtableDesc.push_back((cmapSubtableUnicodeStart) & 0xFF);
                cmapSubtableDesc.push_back((cmapSubtableUnicodeNum >> 8) & 0xFF);
                cmapSubtableDesc.push_back((cmapSubtableUnicodeNum) & 0xFF);
                cmapSubtableDesc.push_back((cmapAddrRel >> 24) & 0xFF);
                cmapSubtableDesc.push_back((cmapAddrRel >> 16) & 0xFF);
                cmapSubtableDesc.push_back((cmapAddrRel >> 8) & 0xFF);
                cmapSubtableDesc.push_back((cmapAddrRel) & 0xFF);
                cmapSubtableDescs.emplace_back(cmapSubtableUnicodeStart, cmapSubtableUnicodeNum, cmapAddrRel);
                cmapSubtables.insert(cmapSubtables.end(), cmapSubtable.begin(), cmapSubtable.end());
                cmapSubtables.insert(cmapSubtables.end(), glyphs.begin(), glyphs.end());
            }
            cmapSubtableNum++;
            cmapSubtableUnicodeStart = ch;
            cmapSubtableUnicodeNum = 1;
            cmapAddrRel += cmapSubtable.size() + glyphs.size();
            cmapSubtable.clear();
            glyphs.clear();
        } else {
            // pad existing cmap subtable
            cmapSubtableUnicodeNum += ch - prevCh;
            cmapSubtable.resize(cmapSubtable.size() + (ch - prevCh - 1) * 2, 0xFF);
        }
        // write glyph
        size_t startAddr = glyphs.size();
        glyphs.resize(glyphs.size() + chGlyphSize, 0);
        glyphs[startAddr + 0] = face->glyph->bitmap_left;
        glyphs[startAddr + 1] = face->glyph->bitmap_top;
        glyphs[startAddr + 2] = face->glyph->bitmap.width;
        glyphs[startAddr + 3] = face->glyph->bitmap.rows;
        glyphs[startAddr + 4] = (face->glyph->advance.x + 32) / 64; // round?
        glyphs[startAddr + 5] = (face->glyph->advance.y + 32) / 64;

        auto ftBuffer = face->glyph->bitmap.buffer;
//        size_t writtenPixels = 0;
        for (int y = 0; y < face->glyph->bitmap.rows; y++) {
            for (int b = 0; b < face->glyph->bitmap.pitch; b++) {
                glyphs[startAddr + 6 + (face->glyph->bitmap.width + 7) / 8 * y + b] = *(ftBuffer + b);
                /// TODO: bit pack
//                size_t startBit = 8 * (startAddr + 6) + y * face->glyph->bitmap.width + b * 8;
//                size_t writeLength = (face->glyph->bitmap.width - (b * 8));
//                if (writeLength > 8) writeLength = 8;
//                size_t bitOffset = 7 - ((writtenPixels - 1) & 0b111);
//                auto d = (uint32_t*) (glyphs.data() + (startBit + writeLength - 1 + bitOffset) / 8);
//                printBinary(*d);
//                std::cerr << startBit + writeLength - 1 + bitOffset << "\n";
//                std::cout << " -> ";
//                *d |= uint32_t(*(ftBuffer + b) & ~((1 << (8 - writeLength)) - 1)) << bitOffset << 24;
//                printBinary(*d);
//                std::cout << "\n";
//                writtenPixels += writeLength;
            }
            ftBuffer += face->glyph->bitmap.pitch;
        }
//        std::cout << "pitch: " << face->glyph->bitmap.pitch << "\n";
//        for (int b = 0; b < face->glyph->bitmap.pitch * face->glyph->bitmap.rows; b++)
//            printBinary(face->glyph->bitmap.buffer[b]);
//        std::cout << "\n";
//
//        for (int b = 6; b < chGlyphSize; b++)
//            printBinary(glyphs[startAddr + b]);
//        std::cout << "\n";

        // add address reference to cmap
        cmapSubtable.push_back(startAddr >> 8);
        cmapSubtable.push_back(startAddr);
        ch++;
    }

    std::cerr << "Created " << cmapSubtableNum << " cmap subtables\n";
    file << (uint8_t) (cmapSubtableNum >> 8);
    file << (uint8_t) cmapSubtableNum;
    for (auto b: cmapSubtableDesc)
        file << b;
    for (auto b: cmapSubtables)
        file << b;
    file.close();
    std::cerr << "Size: " << 2 + cmapSubtableDesc.size() + cmapSubtables.size() << "B = " << (2 + cmapSubtableDesc.size() + cmapSubtables.size() + 1023) / 1024
              << "KB\n";

    std::ofstream cpp;
    cpp.open("output.cpp", std::ios::out);
    std::ifstream bin;
    bin.open("output.bin", std::ios::in | std::ios::binary);
    const char* charSet = "0123456789ABCDEF";

    cpp << "#ifndef OUTPUT_H" << "\n";
    cpp << "#define OUTPUT_H" << "\n";
    cpp << "" << "\n";
    cpp << "#include \"font.h\"" << "\n";
    cpp << "" << "\n";
    cpp << "const sq::FontSubtableDescriptor output_desc[] = {\n";
    for (auto& desc: cmapSubtableDescs) {
        cpp << "\t{0x"
            << charSet[(desc.unicodeStart >> 12) & 0xF]
            << charSet[(desc.unicodeStart >> 8) & 0xF]
            << charSet[(desc.unicodeStart >> 4) & 0xF]
            << charSet[desc.unicodeStart & 0xF]
            << ", " << std::setw(5) << int(desc.unicodeNum) << ", 0x"
            << charSet[(desc.subtableAddr >> 28) & 0xF]
            << charSet[(desc.subtableAddr >> 24) & 0xF]
            << charSet[(desc.subtableAddr >> 20) & 0xF]
            << charSet[(desc.subtableAddr >> 16) & 0xF]
            << charSet[(desc.subtableAddr >> 12) & 0xF]
            << charSet[(desc.subtableAddr >> 8) & 0xF]
            << charSet[(desc.subtableAddr >> 4) & 0xF]
            << charSet[desc.subtableAddr & 0xF] << "},\n";
    }
    cpp << "};\n\n";
    cpp << "const uint8_t output_glyph[] = {\n\t";
    size_t idx = 0;
    for (auto byte: cmapSubtables) {
        idx++;
        cpp << "0x" << charSet[(byte >> 4) & 0xF] << charSet[byte & 0xF] << (idx % 16 == 0 ? ",\n\t\t" : ", ");
    }
    cpp << "\n};\n\n";
    cpp << "const sq::Font output = {" << "\n\t";
    cpp << int(cmapSubtableNum) << ",\n"
        << "\toutput_desc,\n\toutput_glyph\n};";
    cpp << "\n\n";
    cpp << "#endif" << "\n";

    cpp.close();
    bin.close();
}

#endif //FONTCONVERT_OUTPUT_H
