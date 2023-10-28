#include "pch.h"
#include "RunProgramSpec2.h"
#include <common/interop/keyboard_layout.h>
#include <common/interop/shared_constants.h>
#include "Helpers.h"
#include "InputInterface.h"
#include <string>
#include <sstream>

RunProgramSpec2::RunProgramSpec2(const std::wstring& shortcutVK, const std::wstring& targetAppSpec) :
    winKey(ModifierKey::Disabled), ctrlKey(ModifierKey::Disabled), altKey(ModifierKey::Disabled), shiftKey(ModifierKey::Disabled), actionKey(NULL)
{
    auto _keys = splitwstringOnChar(shortcutVK, ';');
    for (auto it : _keys)
    {
        auto vkKeyCode = std::stoul(it);
        SetKey(vkKeyCode);
    }

    auto targetParts = splitwStringOnString(targetAppSpec, CommonSharedConstants::KBM_RUN_PROGRAM_DELIMITER, false);

    path = targetParts[0];
    args = targetParts[1];
    dir = targetParts[2];
}

std::vector<std::wstring> RunProgramSpec2::splitwstringOnChar(const std::wstring& input, wchar_t delimiter)
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

bool RunProgramSpec2::SetKey(const DWORD input)
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
