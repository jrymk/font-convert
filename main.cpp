#include "ft.h"
#include "ui.h"
#include "output.h"

#include <iostream>
#include <string>
#include <fcntl.h>

#define UI_FONT "uifont.ttf"
#define DEFAULT_FONT_PATH "./fonts"

int main(int argc, char* argv[]) {
    UI ui;

    auto error = FT_Init_FreeType(&library);
    if (error) {
        std::cerr << "FreeType init error " << getErrorString(error) << "\n";
        return -1;
    }
    if (!ui.loadFont(UI_FONT))
        std::cerr << "SFML failed to load UI font, make sure " << UI_FONT << " exists!\n";

    static std::string fontsPath = DEFAULT_FONT_PATH;
    std::vector<std::pair<std::string, std::string>> fonts;
    scanFontFiles(fontsPath, fonts);

    while (ui.isOpen()) {
        int mouseWheelDelta = 0;
        bool leftMouseDown = false, rightMouseDown = false;
        sf::Event event;
        while (ui.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                ui.close();
            if (event.type == sf::Event::Resized) {
                ui.setSize(sf::Vector2u(event.size.width, event.size.height));
                sf::View view = ui.getView();
                view.setSize(vec(event.size.width, event.size.height));
                view.setCenter(vec(event.size.width / 2., event.size.height / 2.));
                ui.setView(view);
            }
            if (event.type == sf::Event::MouseWheelScrolled) {
                mouseWheelDelta = event.mouseWheelScroll.delta;
            }
            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Button::Left)
                    leftMouseDown = true;
                if (event.mouseButton.button == sf::Mouse::Button::Right)
                    rightMouseDown = true;
            }
        }

        ui.clear(bgColor);


        FT_Face face;
        std::string errorMessage = "";
        static bool validOutputSetting = false;
        static bool displaySelectedFont = false;
        static sf::Font sfSelectedFont;
        static int outputSize = 16;
        static std::wstring previewString = L"待辦事項 (192)";
        static bool reloadFace = false;
        float fontSectionHeaderHeight = ui.getSize().y * 0.65;
        float fontSectionWidth = ui.getSize().x * 0.3;

        ui.cursor.x = 16;
        ui.cursor.y = 12;

        if (validOutputSetting) {
            /// FONT INFO
            ui.drawText(ui.cursor, "family", fgLight, 18);
            ui.advanceCursor();
            if (displaySelectedFont) {
                sf::Text text;
                text.setFont(sfSelectedFont);
                text.setPosition(ui.cursor);
                text.setString(face->family_name);
                text.setFillColor(fgColor);
                text.setCharacterSize(36);
                ui.draw(text);
            } else
                ui.drawText(ui.cursor, face->family_name, fgColor, 36);
            ui.cursor.y = 86;
            ui.drawText(ui.cursor, "style", fgLight, 18);
            ui.advanceCursor();
            ui.drawText(ui.cursor, face->style_name, fgColor, 28);

            ui.cursor.x += 200;
            ui.cursor.y = 86;

            ui.drawText(ui.cursor, "output size", fgLight, 18);
            ui.advanceCursor();
            ui.drawText(ui.cursor, format("%d", outputSize), fgColor, 28);
            ui.cursor.x += 64;
            ui.cursor.y += 4;
            bool hover = ui.button(ui.cursor, "Edit", bgColor, buttonOn, 20);
            auto old = outputSize;
            if (hover) {
                ui.tooltip(
                        "Output glyph size in pixels\nHover on this button and scroll to increment, or click to enter value in terminal (this window will become unresponsive)");
                outputSize += mouseWheelDelta;

                if (leftMouseDown) {
                    std::cout << "Output size: ";
                    std::cin >> outputSize;
                }
                outputSize = std::max(outputSize, 1);
            }
            if (old != outputSize) {
                error = FT_Set_Pixel_Sizes(face, 0, outputSize);
                if (error)
                    errorMessage += std::string("FT_New_Face error: " + getErrorString(error) + "\n");
            }

            /// PREVIEW
            ui.cursor.x = 16;
            ui.advanceCursor();
            ui.advanceCursor(16);
            ui.drawText(ui.cursor, "preview", fgLight, 18);
            ui.advanceCursor();
            if (displaySelectedFont) {
                sf::Text text;
                text.setFont(sfSelectedFont);
                text.setPosition(ui.cursor);
                text.setString(previewString);
                text.setFillColor(fgColor);
                text.setCharacterSize(28);
                ui.draw(text);
            } else
                ui.drawText(ui.cursor, previewString, fgColor, 28);
            ui.advanceCursor(42);

            static int displayPreviewWidth = 400, displayPreviewHeight = 240;

            if (ui.button(ui.cursor, "Edit text", sf::Color::White, buttonOn, 17)) {
                ui.tooltip("Change the preview text, unicode is allowed\nClick to input in terminal (this window will become unresponsive)");
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                    std::cout << "Preview text: ";
                    _setmode(_fileno(stdin), _O_U16TEXT);
                    std::wcin >> previewString;
                    _setmode(_fileno(stdin), _O_U8TEXT);
                }
            }
            if (ui.button(ui.cursor, format("Display size: %d*%d", displayPreviewWidth, displayPreviewHeight), sf::Color::White, buttonOn, 17)) {
                ui.tooltip(
                        "See how the size of the text compares to a physical display (grey shaded area)\nClick to input in terminal (this window will become unresponsive)");
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                    std::cout << "Display size:\n  w: ";
                    std::cin >> displayPreviewWidth;
                    std::cout << "  h: ";
                    std::cin >> displayPreviewHeight;

                }
            }
            ui.advanceCursor(40);
            ui.cursor.x = 16;

            if (ui.checkMouse(vec(0, ui.cursor.y), sf::Vector2f(fontSectionWidth, fontSectionHeaderHeight - 80 - ui.cursor.y)))
                ui.previewScale *= std::pow(1.25, mouseWheelDelta);

            ui.drawRect(ui.cursor, vec(ui.previewScale * displayPreviewWidth, ui.previewScale * displayPreviewHeight), sf::Color(0, 0, 0, 15));
            int penX = 0, penY = 0;

            ui.drawRect(ui.getPreviewCoord(penX, penY),
                        vec(displayPreviewWidth * ui.previewScale, ui.previewScale * outputSize),
                        sf::Color(195, 55, 230, 20));
            ui.previewOrigin = ui.cursor;

            penY += outputSize;

            for (auto ch: previewString) {
                auto glyph_index = FT_Get_Char_Index(face, ch);

                error = FT_Load_Glyph(
                        face,          /* handle to face object */
                        glyph_index,   /* glyph index           */
                        FT_LOAD_DEFAULT);
                if (error) {
                    ui.drawText(ui.cursor, format("glyph error: %s", getErrorString(error).c_str()), errorColor, 18);
                    continue;
                }

                if (!face->glyph) {
                    ui.drawText(ui.cursor, "no glyph", errorColor, 18);
                    continue;
                }

                error = FT_Render_Glyph(face->glyph,   /* glyph slot  */
                                        FT_RENDER_MODE_MONO);
                if (error) {
                    ui.drawText(ui.cursor, format("render error: %s", getErrorString(error).c_str()), errorColor, 18);
                    continue;
                }

                ui.drawRect(ui.getPreviewCoord(penX + face->glyph->bitmap_left, penY - face->glyph->bitmap_top),
                            vec(face->glyph->bitmap.width * ui.previewScale, face->glyph->bitmap.rows * ui.previewScale),
                            sf::Color(55, 230, 119, 100));
                if (ui.checkMouse(ui.getPreviewCoord(penX + face->glyph->bitmap_left, penY - face->glyph->bitmap_top),
                                  vec(face->glyph->bitmap.width * ui.previewScale, face->glyph->bitmap.rows * ui.previewScale))) {
                    ui.tooltip(format("size: %d*%d\nest: %dKB", face->glyph->bitmap.width, face->glyph->bitmap.rows,
                                      face->num_glyphs * face->glyph->bitmap.width * face->glyph->bitmap.rows / 8 / 1024));
                }


                auto b = face->glyph->bitmap.buffer;
                for (int y = 0; y < face->glyph->bitmap.rows; y++) {
                    for (int x = 0; x < face->glyph->bitmap.width; x++) {
                        sf::Color c = ((b[x >> 3] >> (7 - (x & 0b111))) & 0b1) ? fgColor : sf::Color::Transparent;
                        ui.drawPreviewPixel(penX + face->glyph->bitmap_left + x, penY - face->glyph->bitmap_top + y, c);
                    }
                    b += face->glyph->bitmap.pitch;
                }
                /* increment pen position */
                penX += face->glyph->advance.x >> 6;
                penY += face->glyph->advance.y >> 6; /* not useful for now */
            }
            penY += 5;
        }

        /// FONT SELECTION SECTION
        static float scroll = 0;
        static int selectedFontIdx = -1;

        ui.drawRect(vec(0, fontSectionHeaderHeight - 80), vec(fontSectionWidth, ui.getSize().y - fontSectionHeaderHeight + 80), bgColor);
        for (int i = -scroll / 34; i < fonts.size(); i++) {
            auto& f = fonts[i];
            bool hover = ui.buttonFrame(vec(12, fontSectionHeaderHeight + 34 * i + scroll),
                                        vec(fontSectionWidth - 24, 28),
                                        selectedFontIdx == i ? buttonOn : buttonOff);
            ui.drawText(vec(12 + 4, fontSectionHeaderHeight + 34 * i + scroll + 2),
                        f.second,
                        selectedFontIdx == i ? bgColor : fgColor,
                        18);
            if (hover) {
                ui.tooltip(format("file: %s", f.first.c_str()));
            }
            if (hover && leftMouseDown && selectedFontIdx != i) {
                selectedFontIdx = i;
                reloadFace = true;
            }
            if (fontSectionHeaderHeight + 34 * i + scroll > ui.getSize().y)
                break;
        }

        if (ui.checkMouse(vec(0, fontSectionHeaderHeight), vec(fontSectionWidth, ui.getSize().y - fontSectionHeaderHeight)))
            scroll += mouseWheelDelta * 32;
        scroll = std::max(scroll, ui.getSize().y - fontSectionHeaderHeight - 34 * fonts.size());
        scroll = std::min(scroll, 0.f);

        ui.drawRect(vec(0, fontSectionHeaderHeight - 80), vec(fontSectionWidth, 80), bgColor);
        ui.cursor.x = 12;
        ui.cursor.y = fontSectionHeaderHeight + 12 - 80;
        ui.drawText(ui.cursor, "Fonts", fgColor, 26);
        ui.cursor.x += ui.lastBounds.width + 16;
        if (ui.button(vec(ui.cursor.x, ui.cursor.y + 6), "Refresh", bgColor, buttonOn, 17)) {
            if (leftMouseDown)
                scanFontFiles(fontsPath, fonts);
        }
        if (ui.button(vec(ui.cursor.x, ui.cursor.y + 6), "Set path", bgColor, buttonOn, 17)) {
            ui.tooltip("Set search directory for font files\nClick to input in terminal (this window will become unresponsive)");
            if (leftMouseDown) {
                std::cout << "Set search directory for font files: ";
                std::cin >> fontsPath;
                scanFontFiles(fontsPath, fonts);
            }
        }
        ui.cursor.x = 12;
        ui.advanceCursor(34);
        ui.drawText(ui.cursor, format("searching in %s", fontsPath.c_str()), fgLight, 18);
        ui.advanceCursor();

        /// FONT INFO SECTION

        static std::vector<int> availableRanges;
        static std::vector<std::vector<FT_GlyphSlotRec_>> previewGlyphs(unicodeRangesDef.size());
        if (reloadFace) {
            error = FT_New_Face(library,
                                fonts[selectedFontIdx].first.c_str(),
                                0,
                                &face);
            if (error)
                errorMessage += std::string("FT_New_Face error: " + getErrorString(error) + "\n");

            error = FT_Set_Pixel_Sizes(face, 0, outputSize);
            if (error)
                errorMessage += std::string("FT_New_Face error: " + getErrorString(error) + "\n");

            if (sfSelectedFont.loadFromFile(fonts[selectedFontIdx].first))
                displaySelectedFont = true;
            reloadFace = false;
            validOutputSetting = true;

            /// SCAN RANGES
            std::cerr << "Scanning unicode ranges\n";
            availableRanges.clear();
            for (uint32_t ch = 0; ch <= 0xFFFF; ch++) {
                auto glyph_index = FT_Get_Char_Index(face, ch);

                error = FT_Load_Glyph(
                        face,          /* handle to face object */
                        glyph_index,   /* glyph index           */
                        FT_LOAD_DEFAULT);
                if (error) {
                    std::cerr << "glyph error: " << getErrorString(error) << "\n";
                    continue;
                }

                if (glyph_index) {
                    auto range = getRangeFromUnicode(ch);
                    std::cerr << "Found " + unicodeRangesDef[range].desc << "\n";
                    availableRanges.push_back(range);
                    ch = unicodeRangesDef[range].end;
                }
            }
            std::cerr << "Done scanning\n";

            std::cerr << face->num_glyphs << " glyphs, assume " << outputSize << "*" << outputSize << " bitmaps, it will be "
                      << face->num_glyphs * outputSize * outputSize / 8 / 1024. / 1024. << "MB\n";
        }

        if (selectedFontIdx < 0 || selectedFontIdx >= fonts.size()) {
            errorMessage += "Select a font from the below first\n";
        }
        ui.cursor.x = 16;
        ui.cursor.y = 12;
        if (!errorMessage.empty()) {
            ui.drawText(ui.cursor, errorMessage, errorColor, 20);
        }

        if (validOutputSetting) {

            /// UNICODE RANGES & OUTPUT SECTION
            ui.drawRect(vec(fontSectionWidth, 0), vec(ui.getSize().x - fontSectionWidth, ui.getSize().y), bgColor);
            ui.drawLine(vec(fontSectionWidth, 8), vec(fontSectionWidth, ui.getSize().y - 8), light, 4);

            float rangeHeaderHeight = 12 + 50;
            static float rangeScroll = 0;
            static int hoverRangeIdx = -1;
            float itemHeight = 54;
            float listLeft = fontSectionWidth + 12;
            float itemWidth = ui.getSize().x - listLeft - 24;
            int items = availableRanges.size();

            float charSquareSize = 40;
            float pixelSize = 40. / outputSize;

            for (int i = -rangeScroll / (itemHeight + 6); i < items; i++) {
                auto topLeft = vec(listLeft, rangeHeaderHeight + (itemHeight + 6) * i + rangeScroll);
                auto& r = availableRanges[i];
                bool hover = ui.buttonFrame(vec(listLeft, rangeHeaderHeight + (itemHeight + 6) * i + rangeScroll),
                                            vec(itemWidth, itemHeight),
                                            light);
                ui.cursor = vec(listLeft + 4, rangeHeaderHeight + (itemHeight + 6) * i + rangeScroll + 2);
                ui.drawText(ui.cursor,
                            unicodeRangesDef[r].desc,
                            fgLight,
                            16);

                std::wstring first64;


                uint32_t ch = unicodeRangesDef[r].start;
                previewGlyphs[r].clear();
                for (int i = 0; i < 64; i++) {
                    unsigned int glyph_index = 0;
                    while (!glyph_index && ch <= unicodeRangesDef[r].end)
                        glyph_index = FT_Get_Char_Index(face, ch++);
                    if (ch > unicodeRangesDef[r].end)
                        break;
                    first64.push_back(wchar_t(ch));
                }

                ui.advanceCursor();
                if (displaySelectedFont) {
                    sf::Text text;
                    text.setFont(sfSelectedFont);
                    text.setPosition(ui.cursor);
                    text.setString(first64);
                    text.setFillColor(fgColor);
                    text.setCharacterSize(24);
                    ui.draw(text);
                } else
                    ui.drawText(ui.cursor, first64, fgColor, 24);

                if (hover) {
                    hoverRangeIdx = i;
//                    ui.tooltip(format("file: %s", f.first.c_str()));
                }
                if (rangeHeaderHeight + (itemHeight + 6) * i + rangeScroll > ui.getSize().y)
                    break;
            }

            if (ui.checkMouse(vec(fontSectionWidth, rangeHeaderHeight),
                              vec(ui.getSize().x - fontSectionWidth, ui.getSize().y - rangeHeaderHeight)))
                rangeScroll += mouseWheelDelta * 32;
            rangeScroll = std::max(rangeScroll, ui.getSize().y - rangeHeaderHeight - (itemHeight + 6) * items);
            rangeScroll = std::min(rangeScroll, 0.f);


            ui.cursor = vec(listLeft + 4, 12);
            ui.drawRect(vec(listLeft, 0), vec(ui.getSize().x - listLeft, rangeHeaderHeight), bgColor);
            if (ui.button(ui.cursor, "Save", bgColor, buttonOn, 20)) {
                if (leftMouseDown) {
                    writeFile(fonts[selectedFontIdx].first, outputSize, outputNameGen(face->family_name, face->style_name, outputSize));
                }
            }
            ui.drawText(ui.cursor, format("output file: %s.h", outputNameGen(face->family_name, face->style_name, outputSize).c_str()), fgColor, 20);

        }

        ui.update();
    }
    return 0;
}
