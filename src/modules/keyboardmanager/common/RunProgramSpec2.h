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
    std::wstring args = L"";
    std::wstring dir = L"";

    std::vector<DWORD> keys;

    RunProgramSpec2() = default;

    // Constructor to initialize Shortcut from it's virtual key code string representation.
    RunProgramSpec2(const std::wstring& shortcutVK, const std::wstring& targetAppSpec);

private:
    std::vector<std::wstring> splitwstringOnChar(const std::wstring& input, wchar_t delimiter);

    std::vector<std::wstring> splitwStringOnString(const std::wstring& str, const std::wstring& delimiter, bool ignoreEmpty);

    bool RunProgramSpec2::SetKey(const DWORD input);
};