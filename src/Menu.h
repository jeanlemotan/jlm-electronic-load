#pragma once

#include <vector>
#include <string>
#include "RotaryEncoder.h"

class JLMBackBuffer;

class Menu
{
public:
    Menu();

    static constexpr uint16_t k_topBorderColor = 0xFFFF;
    static constexpr uint16_t k_bottomBorderColor = 0xFFFF;
    static constexpr uint16_t k_scrollBarColor = 0xFFFF;
    static constexpr uint16_t k_selectedColor = 0xAAAA;
    static constexpr uint16_t k_unselectedColor = 0xFFFF;

    enum Flags : uint8_t
    {
        FlagDisabled = 1 << 0,
    };

    struct Entry
    {
        Entry() = default;
        Entry(const char* text) : text(text) {}
        Entry(std::string text) : text(std::move(text)) {}
        Entry(const char* text, uint16_t color) : text(text), color(color) {}
        Entry(std::string text, uint16_t color) : text(std::move(text)), color(color) {}
        std::string text;
        uint16_t color = 0xFFFF;
        uint8_t flags = 0;
    };

    void pushSubMenu(std::vector<Entry> entries, size_t selected, int16_t y);
    void popSubMenu();

    Entry& getSubMenuEntry(size_t idx);

    size_t process(RotaryEncoder& knob);

    void render(JLMBackBuffer& display, size_t maxEntries);

private:
    Entry m_emptyEntry;
    struct SubMenu
    {
        std::vector<Entry> entries;
        size_t crtEntry = 0;
        size_t topEntry = 0;
        int16_t page = 0;
        int16_t y = 0;
        bool popped = false;
        bool finished = false;
        uint16_t maxLineH = 0;
    };

    std::vector<SubMenu> m_subMenus;
    size_t m_crtSubMenuIdx = 0;

    float m_targetScreenX = 0;
    float m_crtX = 0;

    void render(JLMBackBuffer& display, SubMenu& data, size_t maxEntries);
};
