#include "Menu.h"
#include "Adafruit_GFX.h"
#include <algorithm>

constexpr size_t MENU_WIDTH = 200;

///////////////////////////////////////////////////////////////////////////////////////////////////

Menu::Menu()
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::pushSubMenu(std::vector<std::string> const& entries, size_t selected, int16_t y)
{
    if (entries.empty())
    {
        return;
    }

    //max 1 active transition
//    while (m_subMenus.size() > 10)
//    {
//        m_subMenus.erase(m_subMenus.begin());
//    }

    SubMenu subMenu;
    subMenu.entries = entries;
    subMenu.crtEntry = std::min(selected, entries.size() - 1);
    subMenu.topEntry = 0;
    subMenu.x = MENU_WIDTH * m_subMenus.size();
    subMenu.y = y;

    ESP_LOGI("Menu", "Pushing submenu %d", m_subMenus.size());

    m_subMenus.push_back(subMenu);
    m_crtSubMenuIdx = m_subMenus.size() - 1;

    m_targetX = static_cast<float>(m_crtSubMenuIdx * MENU_WIDTH);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::popSubMenu()
{
    if (m_subMenus.empty())
    {
        return;
    }
    ESP_LOGI("Menu", "Popping submenu %d", m_subMenus.size() - 1);

    SubMenu& oldSubMenu = m_subMenus.back();
    oldSubMenu.popped = true;

    m_crtSubMenuIdx = m_subMenus.size() - 2;

    if (m_crtSubMenuIdx < m_subMenus.size())
    {
        m_targetX = static_cast<float>(m_crtSubMenuIdx * MENU_WIDTH);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::setSubMenuEntry(size_t idx, std::string const& entry)
{
    if (m_subMenus.empty() || m_crtSubMenuIdx >= m_subMenus.size())
    {
        return;
    }

    SubMenu& crtSubMenu = m_subMenus[m_crtSubMenuIdx];

    if (idx >= crtSubMenu.entries.size())
    {
        return;
    }

    crtSubMenu.entries[idx] = entry;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

size_t Menu::process(AiEsp32RotaryEncoder& knob)
{
    if (m_subMenus.empty() || m_crtSubMenuIdx >= m_subMenus.size())
    {
        return size_t(-1);
    }

    SubMenu& crtSubMenu = m_subMenus[m_crtSubMenuIdx];

    int32_t delta = knob.encoderChanged();

    int32_t entry = static_cast<int>(crtSubMenu.crtEntry) + delta;
    if (entry < 0)
    {
        entry = crtSubMenu.entries.size() - 1;
    }
    crtSubMenu.crtEntry = static_cast<size_t>(entry);

    if (crtSubMenu.crtEntry >= crtSubMenu.entries.size())
    {
        crtSubMenu.crtEntry = 0;
    }

    if (knob.currentButtonState() == BUT_RELEASED)
    {
        return crtSubMenu.crtEntry;
    }

    return size_t(-1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::render(Adafruit_GFX& display, size_t maxEntries)
{
    if (m_subMenus.empty())
    {
        return;
    }

    m_crtX = (m_crtX * 9.f + m_targetX * 1.f) / 10.f;

    for (SubMenu& subMenu: m_subMenus)
    {
        int16_t x = subMenu.x - static_cast<int16_t>(m_crtX + 0.5f);
        int16_t y = subMenu.y;

        if (x < display.width() && x > -display.width() &&
            y < display.height() && x > -display.height())
        {
            render(display, subMenu, maxEntries);
        }
        else if (subMenu.popped)
        {
            subMenu.finished = true;
        }
    }

    auto it = std::remove_if(m_subMenus.begin(), m_subMenus.end(), [](SubMenu const& subMenu) { return subMenu.finished; });
    m_subMenus.erase(it, m_subMenus.end());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::render(Adafruit_GFX& display, SubMenu& subMenu, size_t maxEntries)
{
    int16_t parentX = static_cast<int16_t>(m_crtX + 0.5f);
    int16_t x = subMenu.x - parentX;
    int16_t y = subMenu.y;

    const int16_t borderH = 2;
    const int16_t entryH = 10;

    if (maxEntries == 0)
    {
        maxEntries = std::min<size_t>((display.height() - y - borderH * 2) / entryH, subMenu.entries.size());
    }
    else
    {
        maxEntries = std::min<size_t>(maxEntries, subMenu.entries.size());        
    }

    //const int16_t total_entry_h = entry_h * max_entries;
    const int16_t totalH = entryH * maxEntries + borderH * 2; //+borders

    if (subMenu.crtEntry < subMenu.topEntry)
    {
        subMenu.topEntry = subMenu.crtEntry;
    }

    if (subMenu.crtEntry - subMenu.topEntry >= maxEntries)
    {
        subMenu.topEntry = subMenu.crtEntry - maxEntries + 1;
    }

    display.setTextWrap(false);

    display.fillRect(x, y, display.width(), maxEntries*entryH + borderH*2, 0);
    for (int16_t xx = 0; xx < display.width(); xx += 3)
    {
        display.drawPixel(x + xx, y, 0xFFFF);
        display.drawPixel(x + xx, y + maxEntries*entryH + borderH*2, 0xFFFF);
    }

    //show scroll bar
    if (maxEntries < subMenu.entries.size())
    {
        int16_t scrollX = x + display.width() - 1;
        int16_t scrollY = y + subMenu.topEntry * totalH / subMenu.entries.size();
        int16_t scrollH = maxEntries * totalH / subMenu.entries.size();
        display.drawLine(scrollX, scrollY, scrollX, scrollY + scrollH, 0xFFFF);
    }

    y += borderH;

    for (size_t i = 0; i < maxEntries; i++)
    {
        size_t entryIdx = subMenu.topEntry + i;

        if (subMenu.crtEntry == entryIdx)
        {
            int16_t rx = x + 2;
            int16_t rw = display.width() - rx - 2;
            //display.fillRect(rx, y, rw, entryH, 1);
            display.setTextColor(0, 0xFFFF);
        }
        else
        {
            display.setTextColor(0xFFFF, 0);
        }
        display.setCursor(x + 4, y + (entryH - 8) / 2);
        display.print(subMenu.entries[entryIdx].c_str());
        y += entryH;
    }

    //display.setTextColor(1, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
