#include "pch.h"
#include "centralized_kb_hook.h"
#include <common/debug_control.h>
#include <common/utils/winapi_error.h>
#include <common/utils/json.h>
#include <common/logger/logger.h>
#include <common/interop/shared_constants.h>
#include <common/SettingsAPI/settings_objects.h>
#include <common/SettingsAPI/settings_helpers.h>
#include <modules/keyboardmanager/common/Shortcut.h>
#include <modules/keyboardmanager/common/RemapShortcut.h>
#include <modules/keyboardmanager/common/KeyboardManagerConstants.h>

namespace CentralizedKeyboardHook
{

    struct HotkeyDescriptor
    {
        Hotkey hotkey;
        std::wstring moduleName;
        std::function<bool()> action;

        bool operator<(const HotkeyDescriptor& other) const
        {
            return hotkey < other.hotkey;
        };
    };

    std::multiset<HotkeyDescriptor> hotkeyDescriptors;
    std::mutex mutex;
    HHOOK hHook{};

    // To store information about handling pressed keys.
    struct PressedKeyDescriptor
    {
        DWORD virtualKey; // Virtual Key code of the key we're keeping track of.
        std::wstring moduleName;
        std::function<bool()> action;
        UINT_PTR idTimer; // Timer ID for calling SET_TIMER with.
        UINT millisecondsToPress; // How much time the key must be pressed.
        bool operator<(const PressedKeyDescriptor& other) const
        {
            // We'll use the virtual key as the real key, since looking for a hit with the key is done in the more time sensitive path (low level keyboard hook).
            return virtualKey < other.virtualKey;
        };
    };
    std::multiset<PressedKeyDescriptor> pressedKeyDescriptors;
    std::mutex pressedKeyMutex;
    long lastKeyInChord = 0;

    // keep track of last pressed key, to detect repeated keys and if there are more keys pressed.
    const DWORD VK_DISABLED = CommonSharedConstants::VK_DISABLED;
    DWORD vkCodePressed = VK_DISABLED;

    // Save the runner window handle for registering timers.
    HWND runnerWindow;

    struct DestroyOnExit
    {
        ~DestroyOnExit()
        {
            Stop();
        }
    } destroyOnExitObj;

    // Handle the pressed key proc
    void PressedKeyTimerProc(
        HWND hwnd,
        UINT /*message*/,
        UINT_PTR idTimer,
        DWORD /*dwTime*/)
    {
        std::multiset<PressedKeyDescriptor> copy;
        {
            // Make a copy, to look for the action to call.
            std::unique_lock lock{ pressedKeyMutex };
            copy = pressedKeyDescriptors;
        }
        for (const auto& it : copy)
        {
            if (it.idTimer == idTimer)
            {
                it.action();
            }
        }

        KillTimer(hwnd, idTimer);
    }

    LRESULT CALLBACK KeyboardHookProc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        if (nCode < 0)
        {
            return CallNextHookEx(hHook, nCode, wParam, lParam);
        }

        const auto& keyPressInfo = *reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (keyPressInfo.dwExtraInfo == PowertoyModuleIface::CENTRALIZED_KEYBOARD_HOOK_DONT_TRIGGER_FLAG)
        {
            // The new keystroke was generated from one of our actions. We should pass it along.
            return CallNextHookEx(hHook, nCode, wParam, lParam);
        }

        /*
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            Logger::trace(L"KEY UP");
        }
        */

        // Check if the keys are pressed.
        if (!pressedKeyDescriptors.empty())
        {
            bool wasKeyPressed = vkCodePressed != VK_DISABLED;
            // Hold the lock for the shortest possible duration
            if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
            {
                if (!wasKeyPressed)
                {
                    // If no key was pressed before, let's start a timer to take into account this new key.
                    std::unique_lock lock{ pressedKeyMutex };
                    PressedKeyDescriptor dummy{ .virtualKey = keyPressInfo.vkCode };
                    auto [it, last] = pressedKeyDescriptors.equal_range(dummy);
                    for (; it != last; ++it)
                    {
                        SetTimer(runnerWindow, it->idTimer, it->millisecondsToPress, PressedKeyTimerProc);
                    }
                }
                else if (vkCodePressed != keyPressInfo.vkCode)
                {
                    // If a different key was pressed, let's clear the timers we have started for the previous key.
                    std::unique_lock lock{ pressedKeyMutex };
                    PressedKeyDescriptor dummy{ .virtualKey = vkCodePressed };
                    auto [it, last] = pressedKeyDescriptors.equal_range(dummy);
                    for (; it != last; ++it)
                    {
                        KillTimer(runnerWindow, it->idTimer);
                    }
                }
                vkCodePressed = keyPressInfo.vkCode;
            }
            if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            {
                std::unique_lock lock{ pressedKeyMutex };
                PressedKeyDescriptor dummy{ .virtualKey = keyPressInfo.vkCode };
                auto [it, last] = pressedKeyDescriptors.equal_range(dummy);
                for (; it != last; ++it)
                {
                    KillTimer(runnerWindow, it->idTimer);
                }
                vkCodePressed = 0x100;
            }
        }

        if ((wParam != WM_KEYDOWN) && (wParam != WM_SYSKEYDOWN))
        {
            return CallNextHookEx(hHook, nCode, wParam, lParam);
        }

        LocalKey lHotkey;

        lHotkey.win = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
        lHotkey.control = static_cast<bool>(GetAsyncKeyState(VK_CONTROL) & 0x8000);
        lHotkey.shift = static_cast<bool>(GetAsyncKeyState(VK_SHIFT) & 0x8000);
        lHotkey.alt = static_cast<bool>(GetAsyncKeyState(VK_MENU) & 0x8000);

        lHotkey.l_win = (GetAsyncKeyState(VK_LWIN) & 0x8000);
        lHotkey.l_control = static_cast<bool>(GetAsyncKeyState(VK_LCONTROL) & 0x8000);
        lHotkey.l_shift = static_cast<bool>(GetAsyncKeyState(VK_LSHIFT) & 0x8000);
        lHotkey.l_alt = static_cast<bool>(GetAsyncKeyState(VK_LMENU) & 0x8000);

        lHotkey.r_win = (GetAsyncKeyState(VK_RWIN) & 0x8000);
        lHotkey.r_control = static_cast<bool>(GetAsyncKeyState(VK_RCONTROL) & 0x8000);
        lHotkey.r_shift = static_cast<bool>(GetAsyncKeyState(VK_RSHIFT) & 0x8000);
        lHotkey.r_alt = static_cast<bool>(GetAsyncKeyState(VK_RMENU) & 0x8000);

        lHotkey.key = static_cast<unsigned char>(keyPressInfo.vkCode);

        Hotkey hotkey{
            .win = lHotkey.win,
            .ctrl = lHotkey.control,
            .shift = lHotkey.shift,
            .alt = lHotkey.alt,
            .key = static_cast<UCHAR>(lHotkey.key)
        };

        handleCreateProcessHotKeysAndChords(lHotkey);

        std::function<bool()> action;
        {
            // Hold the lock for the shortest possible duration
            std::unique_lock lock{ mutex };
            HotkeyDescriptor dummy{ .hotkey = hotkey };
            auto it = hotkeyDescriptors.find(dummy);
            if (it != hotkeyDescriptors.end())
            {
                action = it->action;
            }
        }

        if (action)
        {
            if (action())
            {
                // After invoking the hotkey send a dummy key to prevent Start Menu from activating
                INPUT dummyEvent[1] = {};
                dummyEvent[0].type = INPUT_KEYBOARD;
                dummyEvent[0].ki.wVk = 0xFF;
                dummyEvent[0].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, dummyEvent, sizeof(INPUT));

                // Swallow the key press
                return 1;
            }
        }

        return CallNextHookEx(hHook, nCode, wParam, lParam);
    }

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

        RunProgramSpec(const std::wstring& shortcutVK) :
            winKey(ModifierKey::Disabled), ctrlKey(ModifierKey::Disabled), altKey(ModifierKey::Disabled), shiftKey(ModifierKey::Disabled), actionKey(NULL)
        {
            auto _keys = splitwstring(shortcutVK, ';');
            for (auto it : _keys)
            {
                auto vkKeyCode = std::stoul(it);
                SetKey(vkKeyCode);
            }
        }

        std::vector<std::wstring> splitwstring(const std::wstring& input, wchar_t delimiter)
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

        bool SetKey(const DWORD input)
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
    };

    bool getConfigInit = false;
    std::vector<RunProgramSpec> runProgramSpecs;

    void RefreshConfig()
    {
        getConfigInit = false;
        runProgramSpecs.clear();
    }

    void setupConfig()
    {
        if (!getConfigInit)
        {
            PowerToysSettings::PowerToyValues settings = PowerToysSettings::PowerToyValues::load_from_settings_file(KeyboardManagerConstants::ModuleName);
            auto current_config = settings.get_string_value(KeyboardManagerConstants::ActiveConfigurationSettingName);

            if (!current_config)
            {
                return;
            }

            std::wstring currentConfig = KeyboardManagerConstants::DefaultConfiguration;

            currentConfig = *current_config;

            // Read the config file and load the remaps.
            auto configFile = json::from_file(PTSettingsHelper::get_module_save_folder_location(KeyboardManagerConstants::ModuleName) + L"\\" + *current_config + L".json");
            if (!configFile)
            {
                return;
            }

            auto jsonData = json::from_file(PTSettingsHelper::get_module_save_folder_location(KeyboardManagerConstants::ModuleName) + L"\\" + *current_config + L".json");

            //auto jsonData = json::from_file(L"c:\\Temp\\keyboardManagerConfig.json");

            if (!jsonData)
            {
                return;
            }

            auto keyboardManagerConfig = jsonData->GetNamedObject(L"remapShortcuts");

            if (keyboardManagerConfig)
            {
                auto global = keyboardManagerConfig.GetNamedArray(L"runProgram");
                for (const auto& it : global)
                {
                    try
                    {
                        auto originalKeys = it.GetObjectW().GetNamedString(KeyboardManagerConstants::OriginalKeysSettingName);

                        auto path = it.GetObjectW().GetNamedString(L"targetApp");

                        auto runProgramSpec = RunProgramSpec(originalKeys.c_str());

                        runProgramSpec.path = path;

                        /*
                        // auto isChord = it.GetObjectW().GetNamedBoolean(L"isChord");
                        
                        
                        runProgramSpec.win = it.GetObjectW().GetNamedBoolean(L"win");
                        runProgramSpec.shift = it.GetObjectW().GetNamedBoolean(L"shift");
                        runProgramSpec.alt = it.GetObjectW().GetNamedBoolean(L"alt");
                        runProgramSpec.ctrl = it.GetObjectW().GetNamedBoolean(L"control");

                        auto keys = it.GetObjectW().GetNamedArray(L"keys");
                        auto program = it.GetObjectW().GetNamedObject(L"program");
                        auto path = program.GetObjectW().GetNamedString(L"path");

                        runProgramSpec.path = path;
                        
                        for (const auto& key : keys)
                        {
                            runProgramSpec.keys.push_back(static_cast<UCHAR>(key.GetNumber()));
                        }

                        

                        */
                        runProgramSpecs.push_back(runProgramSpec);
                    }
                    catch (...)
                    {
                        Logger::error(L"Improper Key Data JSON. Try the next remap.");
                    }
                }
            }

            getConfigInit = true;
        }
    }

    bool isPartOfAnyRunProgramSpec(DWORD key)
    {
        for (RunProgramSpec runProgramSpec : runProgramSpecs)
        {
            if (runProgramSpec.actionKey == key)
            {
                return true;
            }

            /*for (unsigned char c : runProgramSpec.keys)
            {
                if (c == key)
                {
                    return true;
                }
            }*/
        }
        return false;
    }

    bool isPartOfThisRunProgramSpec(RunProgramSpec runProgramSpec, DWORD key)
    {
        for (DWORD c : runProgramSpec.keys)
        {
            if (c == key)
            {
                return true;
            }
        }
        return false;
    }

    void handleCreateProcessHotKeysAndChords(LocalKey hotkey)
    {
        if (hotkey.win || hotkey.shift || hotkey.control || hotkey.alt)
        {
            setupConfig();

            if (!isPartOfAnyRunProgramSpec(hotkey.key))
            {
                lastKeyInChord = 0;
                return;
            }
        }
        else
        {
            return;
        }

        for (RunProgramSpec runProgramSpec : runProgramSpecs)
        {
            if (
                (runProgramSpec.winKey == ModifierKey::Disabled || (runProgramSpec.winKey == ModifierKey::Left && hotkey.l_win)) 
                && (runProgramSpec.shiftKey == ModifierKey::Disabled || (runProgramSpec.shiftKey == ModifierKey::Left && hotkey.l_shift)) 
                && (runProgramSpec.altKey == ModifierKey::Disabled || (runProgramSpec.altKey == ModifierKey::Left && hotkey.l_alt)) 
                && (runProgramSpec.ctrlKey == ModifierKey::Disabled || (runProgramSpec.ctrlKey == ModifierKey::Left && hotkey.l_control)))
            {
                auto runProgram = false;

                if (runProgramSpec.actionKey == hotkey.key)
                {
                    runProgram = true;
                }

                /*if (runProgramSpec.keys.size() == 1 && runProgramSpec.keys[0] == hotkey.key)
                {
                    runProgram = true;
                }
                else if (runProgramSpec.keys.size() == 2 && runProgramSpec.keys[0] == lastKeyInChord && runProgramSpec.keys[1] == hotkey.key)
                {
                    runProgram = true;
                }
                else
                {
                    lastKeyInChord = hotkey.key;
                }*/

                if (runProgram)
                {
                    Logger::trace(L"runProgram {}", runProgram);
                    lastKeyInChord = 0;

                    if (runProgramSpec.path.compare(L"RefreshConfig") == 0)
                    {
                        RefreshConfig();
                    }
                    else
                    {
                        std::wstring executable_path = runProgramSpec.path;
                        std::wstring executable_args = fmt::format(L"\"{}\"", executable_path);
                        STARTUPINFO startup_info = { sizeof(startup_info) };
                        PROCESS_INFORMATION process_info = { 0 };
                        CreateProcessW(executable_path.c_str(), executable_args.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup_info, &process_info);
                    }
                }
            }
        }
    }

    void SetHotkeyAction(const std::wstring& moduleName, const Hotkey& hotkey, std::function<bool()>&& action) noexcept
    {
        Logger::trace(L"Register hotkey action for {}", moduleName);
        std::unique_lock lock{ mutex };
        hotkeyDescriptors.insert({ .hotkey = hotkey, .moduleName = moduleName, .action = std::move(action) });
    }

    void AddPressedKeyAction(const std::wstring& moduleName, const DWORD vk, const UINT milliseconds, std::function<bool()>&& action) noexcept
    {
        // Calculate a unique TimerID.
        auto hash = std::hash<std::wstring>{}(moduleName); // Hash the module as the upper part of the timer ID.
        const UINT upperId = hash & 0xFFFF;
        const UINT lowerId = vk & 0xFFFF; // The key to press can be the lower ID.
        const UINT timerId = upperId << 16 | lowerId;
        std::unique_lock lock{ pressedKeyMutex };
        pressedKeyDescriptors.insert({ .virtualKey = vk, .moduleName = moduleName, .action = std::move(action), .idTimer = timerId, .millisecondsToPress = milliseconds });
    }

    void ClearModuleHotkeys(const std::wstring& moduleName) noexcept
    {
        Logger::trace(L"UnRegister hotkey action for {}", moduleName);
        {
            std::unique_lock lock{ mutex };
            auto it = hotkeyDescriptors.begin();
            while (it != hotkeyDescriptors.end())
            {
                if (it->moduleName == moduleName)
                {
                    it = hotkeyDescriptors.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
        {
            std::unique_lock lock{ pressedKeyMutex };
            auto it = pressedKeyDescriptors.begin();
            while (it != pressedKeyDescriptors.end())
            {
                if (it->moduleName == moduleName)
                {
                    it = pressedKeyDescriptors.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    void Start() noexcept
    {
#if defined(DISABLE_LOWLEVEL_HOOKS_WHEN_DEBUGGED)
        const bool hook_disabled = IsDebuggerPresent();
#else
        const bool hook_disabled = false;
#endif
        if (!hook_disabled)
        {
            if (!hHook)
            {
                hHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, NULL, NULL);
                if (!hHook)
                {
                    DWORD errorCode = GetLastError();
                    show_last_error_message(L"SetWindowsHookEx", errorCode, L"centralized_kb_hook");
                }
            }
        }
    }

    void Stop() noexcept
    {
        if (hHook && UnhookWindowsHookEx(hHook))
        {
            hHook = NULL;
        }
    }

    void RegisterWindow(HWND hwnd) noexcept
    {
        runnerWindow = hwnd;
    }
}
