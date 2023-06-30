#ifndef FONTCONVERT_UI_H
#define FONTCONVERT_UI_H

#include <SFML/Graphics.hpp>
#include <wchar.h>
#include <cmath>

#define cursorLeft 12
#define bgColor sf::Color(237, 237, 237)
#define fgColor sf::Color(48, 48, 48)
#define fgLight sf::Color(100, 100, 100)
#define light sf::Color(200, 200, 200)
#define accent sf::Color(94, 169, 255)
#define errorColor sf::Color(186, 56, 56)
#define buttonOn sf::Color(209, 137, 65)
#define buttonOff sf::Color(214, 208, 199)
#define buttonHover sf::Color(0, 0, 0, 50)

template<typename... Args>
std::string format(const char* format, Args... args) {
    char formatBuffer[1024];
    sprintf(formatBuffer, format, args...);
    return formatBuffer;
}

template<typename... Args>
std::wstring format(const wchar_t* format, Args... args) {
    wchar_t formatBuffer[1024];
    swprintf(formatBuffer, format, args...);
    return formatBuffer;
}

std::string toHex(uint16_t n) {
    const char* table = "0123456789ABCDEF";
    char out[5] = {table[(n >> 12) & 0xF], table[(n >> 8) & 0xF], table[(n >> 4) & 0xF], table[(n >> 0) & 0xF], '\0'};
    return out;
}

sf::Vector2f vec(float x, float y) {
    return {x, y};
}

class UI : public sf::RenderWindow {
    std::string tooltipStr;

public:
    sf::Font font;
    sf::Vector2f previewOrigin;
    float previewScale = 3;
    sf::Vector2f cursor;
    sf::FloatRect lastBounds;
    int lastTextSize = 18;

    UI() : sf::RenderWindow(sf::VideoMode(1500, 900), "font convert by jrymk") {

    }

    bool loadFont(std::string fontPath) {
        return font.loadFromFile(fontPath);
    }

    void drawLine(sf::Vector2f pos1, sf::Vector2f pos2, const sf::Color& strokeColor, float strokeWidth) {
        sf::RectangleShape rect;
        float len = std::sqrt(std::pow(pos2.x - pos1.x, 2.f) + std::pow(pos2.y - pos1.y, 2.f));
        float ang = std::atan((pos2.y - pos1.y) / (pos2.x - pos1.x));
        rect.setSize(sf::Vector2f(std::sqrt(std::pow(pos2.x - pos1.x, 2.f) + std::pow(pos2.y - pos1.y, 2.f)), strokeWidth));
        rect.setOrigin(len / 2., strokeWidth / 2.);
        rect.setPosition(sf::Vector2f((pos1.x + pos2.x) / 2.f, (pos1.y + pos2.y) / 2.f));
        rect.setRotation(ang / M_PI * 180);
        rect.setFillColor(strokeColor);
        draw(rect);
    }

    void drawText(sf::Vector2f pos, const std::string& str, sf::Color color, unsigned int size) {
        sf::Text text;
        text.setFont(font);
        text.setPosition(pos);
        text.setString(str);
        text.setFillColor(color);
        text.setCharacterSize(size);
        draw(text);
        lastBounds = text.getLocalBounds();
        lastTextSize = size;
    }

    void drawText(sf::Vector2f pos, const std::wstring& str, sf::Color color, unsigned int size) {
        sf::Text text;
        text.setFont(font);
        text.setPosition(pos);
        text.setString(str);
        text.setFillColor(color);
        text.setCharacterSize(size);
        draw(text);
        lastBounds = text.getLocalBounds();
        lastTextSize = size;
    }

    void advanceCursor(float amount = 0) {
        if (amount == 0)
            cursor.y += lastTextSize * 1.3f;
        else
            cursor.y += amount;
    }

    void drawRect(sf::Vector2f pos, sf::Vector2f size, sf::Color fill, sf::Color border = sf::Color::Transparent, float stroke = -2.) {
        sf::RectangleShape rect;
        rect.setPosition(pos);
        rect.setSize(size);
        rect.setFillColor(fill);
        rect.setOutlineColor(border);
        rect.setOutlineThickness(stroke);
        draw(rect);
    }

    void drawCircle(sf::Vector2f pos, float radius, sf::Color fill, sf::Color border = sf::Color::Transparent, float stroke = -2.) {
        sf::CircleShape rect;
        rect.setPosition(pos);
        rect.setRadius(radius);
        rect.setPointCount(20);
        rect.setFillColor(fill);
        rect.setOutlineColor(border);
        rect.setOutlineThickness(stroke);
        draw(rect);
    }

    sf::Vector2f getPreviewCoord(int x, int y) {
        return {previewOrigin.x + x * previewScale, previewOrigin.y + y * previewScale};
    }

    void drawPreviewPixel(int x, int y, sf::Color color) {
        drawRect(getPreviewCoord(x, y),
                 sf::Vector2f(previewScale, previewScale),
                 color);
    }

    bool checkMouse(sf::Vector2f pos, sf::Vector2f size) {
        auto mouse = sf::Mouse::getPosition(*this);
        return (mouse.x >= pos.x && mouse.y >= pos.y && mouse.x < pos.x + size.x && mouse.y < pos.y + size.y);
    }

    bool button(sf::Vector2f pos, const std::string& str, sf::Color fg, sf::Color bg, unsigned int size) {
        bool hover = false;
        sf::Text text;
        text.setFont(font);
        text.setString(str);
        text.setCharacterSize(size);

        drawRect(pos, sf::Vector2f(text.getLocalBounds().width + 12, size * 1.3), bg);
        if (checkMouse(pos, sf::Vector2f(text.getLocalBounds().width + 12, size * 1.3))) {
            drawRect(pos, sf::Vector2f(text.getLocalBounds().width + 12, size * 1.3), buttonHover);
            hover = true;
        }

        drawText(vec(pos.x + 4, pos.y), str, fg, size);

        cursor.x += text.getLocalBounds().width + 12 + 8;

        return hover;
    }

    bool buttonFrame(sf::Vector2f pos, sf::Vector2f size, sf::Color bg) {
        bool hover = false;

        drawRect(pos, size, bg);
        if (checkMouse(pos, size)) {
            drawRect(pos, size, buttonHover);
            hover = true;
        }
        return hover;
    }

    void tooltip(const std::string& str) {
        tooltipStr = str;
    }

    void update() {
        if (!tooltipStr.empty()) {
            auto mouse = sf::Mouse::getPosition(*this);
            sf::Text text;
            text.setFont(font);
            text.setString(tooltipStr);
            text.setCharacterSize(16);

            if (mouse.x > sf::RenderWindow::getSize().x - text.getLocalBounds().width) {
                drawRect(sf::Vector2f(mouse.x - 16 - text.getLocalBounds().width, mouse.y + 16),
                         sf::Vector2f(text.getLocalBounds().width + 12, text.getLocalBounds().height + 4),
                         sf::Color(255, 255, 255, 230));
                text.setFillColor(fgColor);
                text.setPosition(sf::Vector2f(mouse.x - 16 + 3 - text.getLocalBounds().width, mouse.y + 16));
            } else {
                drawRect(sf::Vector2f(mouse.x + 16, mouse.y + 16), sf::Vector2f(text.getLocalBounds().width + 12, text.getLocalBounds().height + 4),
                         sf::Color(255, 255, 255, 230));
                text.setFillColor(fgColor);
                text.setPosition(sf::Vector2f(mouse.x + 16 + 3, mouse.y + 16));
            }

            draw(text);
        }

        tooltipStr = "";
        display();
    }

};

#endif //FONTCONVERT_UI_H
