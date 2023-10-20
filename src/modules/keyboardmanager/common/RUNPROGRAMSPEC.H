#pragma once
#include "ModifierKey.h"

#include <compare>
#include <tuple>
#include <variant>


class RunProgramSpec
{
public:
    ModifierKey winKey = ModifierKey::Disabled;
    ModifierKey ctrlKey = ModifierKey::Disabled;
    ModifierKey altKey = ModifierKey::Disabled;
    ModifierKey shiftKey = ModifierKey::Disabled;
    DWORD actionKey = {};

    std::wstring path = L"";
    std::vector<DWORD> keys;

    RunProgramSpec() = default;

    // Constructor to initialize Shortcut from it's virtual key code string representation.
    RunProgramSpec(const std::wstring& shortcutVK);

private:
    std::vector<std::wstring> splitwstring(const std::wstring& input, wchar_t delimiter);

    bool RunProgramSpec::SetKey(const DWORD input);
};

RunProgramSpec::RunProgramSpec(const std::wstring& shortcutVK) :
    winKey(ModifierKey::Disabled), ctrlKey(ModifierKey::Disabled), altKey(ModifierKey::Disabled), shiftKey(ModifierKey::Disabled), actionKey(NULL)
{
    auto _keys = splitwstring(shortcutVK, ';');
    for (auto it : _keys)
    {
        auto vkKeyCode = std::stoul(it);
        SetKey(vkKeyCode);
    }
}

std::vector<std::wstring> RunProgramSpec::splitwstring(const std::wstring& input, wchar_t delimiter)
{
    std::wstringstream ss(input);
    std::wstring item;
    std::vector<std::wstring> splittedStrings;
    while (std::getline(ss, item, delimiter))
    {
        splittedStrings.push_back(item);
    }

    return splittedStrings;
}

bool RunProgramSpec::SetKey(const DWORD input)
{
    // Since there isn't a key for a common Win key we use the key code defined by us
    if (input == CommonSharedConstants::VK_WIN_BOTH)
    {
        if (winKey == ModifierKey::Both)
        {
            return false;
        }
        winKey = ModifierKey::Both;
    }
    else if (input == VK_LWIN)
    {
        if (winKey == ModifierKey::Left)
        {
            return false;
        }
        winKey = ModifierKey::Left;
    }
    else if (input == VK_RWIN)
    {
        if (winKey == ModifierKey::Right)
        {
            return false;
        }
        winKey = ModifierKey::Right;
    }
    else if (input == VK_LCONTROL)
    {
        if (ctrlKey == ModifierKey::Left)
        {
            return false;
        }
        ctrlKey = ModifierKey::Left;
    }
    else if (input == VK_RCONTROL)
    {
        if (ctrlKey == ModifierKey::Right)
        {
            return false;
        }
        ctrlKey = ModifierKey::Right;
    }
    else if (input == VK_CONTROL)
    {
        if (ctrlKey == ModifierKey::Both)
        {
            return false;
        }
        ctrlKey = ModifierKey::Both;
    }
    else if (input == VK_LMENU)
    {
        if (altKey == ModifierKey::Left)
        {
            return false;
        }
        altKey = ModifierKey::Left;
    }
    else if (input == VK_RMENU)
    {
        if (altKey == ModifierKey::Right)
        {
            return false;
        }
        altKey = ModifierKey::Right;
    }
    else if (input == VK_MENU)
    {
        if (altKey == ModifierKey::Both)
        {
            return false;
        }
        altKey = ModifierKey::Both;
    }
    else if (input == VK_LSHIFT)
    {
        if (shiftKey == ModifierKey::Left)
        {
            return false;
        }
        shiftKey = ModifierKey::Left;
    }
    else if (input == VK_RSHIFT)
    {
        if (shiftKey == ModifierKey::Right)
        {
            return false;
        }
        shiftKey = ModifierKey::Right;
    }
    else if (input == VK_SHIFT)
    {
        if (shiftKey == ModifierKey::Both)
        {
            return false;
        }
        shiftKey = ModifierKey::Both;
    }
    else
    {
        if (actionKey == input)
        {
            return false;
        }
        actionKey = input;
    }

    return true;
}
