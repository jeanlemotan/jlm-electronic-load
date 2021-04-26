#include "Menu.h"
#include "JLMBackBuffer.h"
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////////////////////////

Menu::Menu()
{
    m_crtX = -1000.f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::pushSubMenu(std::vector<Entry> entries, size_t selected, int16_t y)
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
    subMenu.crtEntry = std::min(selected, entries.size() - 1);
    subMenu.entries = std::move(entries);
    subMenu.topEntry = 0;
    subMenu.page = m_subMenus.size();
    subMenu.y = y;

    ESP_LOGI("Menu", "Pushing submenu %d", m_subMenus.size());

    m_subMenus.push_back(subMenu);
    m_crtSubMenuIdx = m_subMenus.size() - 1;

    m_targetScreenX = static_cast<float>(m_crtSubMenuIdx);
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
        m_targetScreenX = static_cast<float>(m_crtSubMenuIdx);
    }
    else
    {
        m_targetScreenX = -1.f;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Menu::Entry& Menu::getSubMenuEntry(size_t idx)
{
    m_emptyEntry = Entry();
    if (m_subMenus.empty() || m_crtSubMenuIdx >= m_subMenus.size())
    {
        return m_emptyEntry;
    }

    SubMenu& crtSubMenu = m_subMenus[m_crtSubMenuIdx];
    if (idx >= crtSubMenu.entries.size())
    {
        return m_emptyEntry;
    }

    return crtSubMenu.entries[idx];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

size_t Menu::process(RotaryEncoder& knob)
{
    if (m_subMenus.empty() || m_crtSubMenuIdx >= m_subMenus.size())
    {
        return size_t(-1);
    }

    SubMenu& crtSubMenu = m_subMenus[m_crtSubMenuIdx];

    int32_t delta = knob.encoderDelta();

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

	if (knob.buttonState() == RotaryEncoder::ButtonState::Released)
    {
        return crtSubMenu.crtEntry;
    }

    return size_t(-1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::render(JLMBackBuffer& display, size_t maxEntries)
{
    if (m_subMenus.empty())
    {
        return;
    }

    m_crtX = (m_crtX * 7.f + m_targetScreenX * (display.width() + 10) * 3.f) / 10.f;

    for (SubMenu& subMenu: m_subMenus)
    {
        int16_t x = subMenu.page * display.width() - static_cast<int16_t>(m_crtX + 0.5f);
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

void Menu::render(JLMBackBuffer& display, SubMenu& subMenu, size_t maxEntries)
{
    int16_t parentX = static_cast<int16_t>(m_crtX + 0.5f);
    int16_t x = subMenu.page * display.width() - parentX;
    int16_t y = subMenu.y;

    const int16_t borderH = 2;

    if (subMenu.maxLineH == 0)
    {
        for (const Entry& entry: subMenu.entries)
        {
            subMenu.maxLineH = std::max(subMenu.maxLineH, display.getTextHeight(entry.text.c_str()));
        }
    }

    GFXfont* font = display.getFont();
    const int16_t entryH = font->yAdvance;

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

    display.setOpacity(128);
    display.fillRect(x, y, display.width(), maxEntries*entryH + borderH*2, 0); 
    display.setOpacity(255);

    for (int16_t xx = 0; xx < display.width(); xx += 3)
    {
        display.drawPixel(x + xx, y, k_topBorderColor);
        display.drawPixel(x + xx, y + maxEntries*entryH + borderH*2, k_bottomBorderColor);
    }

    //show scroll bar
    if (maxEntries < subMenu.entries.size())
    {
        int16_t scrollX = x + display.width() - 1;
        int16_t scrollY = y + subMenu.topEntry * totalH / subMenu.entries.size();
        int16_t scrollH = maxEntries * totalH / subMenu.entries.size();
        display.drawLine(scrollX, scrollY, scrollX, scrollY + scrollH, k_scrollBarColor);
    }

    y += borderH;

    y += entryH - (entryH - subMenu.maxLineH);
    for (size_t i = 0; i < maxEntries; i++)
    {
        size_t entryIdx = subMenu.topEntry + i;

        if (subMenu.crtEntry == entryIdx)
        {
            int16_t rx = x + 2;
            int16_t r = 3;
            display.fillCircle(rx, y - subMenu.maxLineH / 2 + 1, r, k_selectedColor);
            display.setTextColor(k_selectedColor);
        }
        else
        {
            display.setTextColor(k_unselectedColor);
        }
        display.setCursor(x + 8, y);
        display.print(subMenu.entries[entryIdx].text.c_str());
        y += entryH;
    }

    //display.setTextColor(1, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
