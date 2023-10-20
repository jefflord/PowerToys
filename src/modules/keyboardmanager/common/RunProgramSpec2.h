#pragma once
#include "pch.h"
#include <common/interop/keyboard_layout.h>
#include <common/interop/shared_constants.h>
#include "Helpers.h"
#include "InputInterface.h"
#include <string>
#include <sstream>

class RunProgramSpec2
{
public:
    ModifierKey winKey = ModifierKey::Disabled;
    ModifierKey ctrlKey = ModifierKey::Disabled;
    ModifierKey altKey = ModifierKey::Disabled;
    ModifierKey shiftKey = ModifierKey::Disabled;
    DWORD actionKey = {};

    std::wstring path = L"";
    std::vector<DWORD> keys;

    RunProgramSpec2() = default;

    // Constructor to initialize Shortcut from it's virtual key code string representation.
    RunProgramSpec2(const std::wstring& shortcutVK);

private:
    std::vector<std::wstring> splitwstring(const std::wstring& input, wchar_t delimiter);

    bool RunProgramSpec2::SetKey(const DWORD input);
};