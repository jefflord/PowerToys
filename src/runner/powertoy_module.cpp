#include "pch.h"
#include "powertoy_module.h"
#include "centralized_kb_hook.h"
#include "centralized_hotkeys.h"
#include <common/logger/logger.h>
#include <common/utils/winapi_error.h>

std::map<std::wstring, PowertoyModule>& modules()
{
    static std::map<std::wstring, PowertoyModule> modules;
    return modules;
}

PowertoyModule load_powertoy(const std::wstring_view filename)
{
    auto handle = winrt::check_pointer(LoadLibraryW(filename.data()));
    auto create = reinterpret_cast<powertoy_create_func>(GetProcAddress(handle, "powertoy_create"));
    if (!create)
    {
        FreeLibrary(handle);
        winrt::throw_last_error();
    }
    auto pt_module = create();
    if (!pt_module)
    {
        FreeLibrary(handle);
        winrt::throw_hresult(winrt::hresult(E_POINTER));
    }
    return PowertoyModule(pt_module, handle);
}

json::JsonObject PowertoyModule::json_config() const
{
    int size = 0;
    pt_module->get_config(nullptr, &size);
    std::wstring result;
    result.resize(size - 1);
    pt_module->get_config(result.data(), &size);
    return json::JsonObject::Parse(result);
}

PowertoyModule::PowertoyModule(PowertoyModuleIface* pt_module, HMODULE handle) :
    handle(handle), pt_module(pt_module)
{
    if (!pt_module)
    {
        throw std::runtime_error("Module not initialized");
    }

    update_hotkeys();
    UpdateHotkeyEx();
}

void PowertoyModule::update_hotkeys()
{
    CentralizedKeyboardHook::ClearModuleHotkeys(pt_module->get_key());

    size_t hotkeyCount = pt_module->get_hotkeys(nullptr, 0);
    std::vector<PowertoyModuleIface::Hotkey> hotkeys(hotkeyCount);
    pt_module->get_hotkeys(hotkeys.data(), hotkeyCount);

    auto modulePtr = pt_module.get();

    if (hotkeyCount >= 1)
    {
        hotkeys[0].win = true;
        hotkeys[0].key = 0;
    }

    for (size_t i = 0; i < hotkeyCount; i++)
    {
        auto action = [modulePtr, i] {
            Logger::trace(L"{} hotkey is invoked from Centralized keyboard hook", modulePtr->get_key());
            return modulePtr->on_hotkey(i);
        };

        if (hotkeys[i].win && hotkeys[i].key == 0)
        {
            CentralizedKeyboardHook::AddPressedKeyAction2(pt_module->get_key(), VK_LWIN, action);
        }
        else
        {
            CentralizedKeyboardHook::SetHotkeyAction(pt_module->get_key(), hotkeys[i], action);
        }
    }
}

void PowertoyModule::UpdateHotkeyEx()
{
    CentralizedHotkeys::UnregisterHotkeysForModule(pt_module->get_key());
    auto container = pt_module->GetHotkeyEx();

    if (container.has_value())
    {
        if (pt_module->is_enabled())
        {
            auto hotkey = container.value();
            auto modulePtr = pt_module.get();

            if (hotkey.vkCode > 0)
            {
                auto action = [modulePtr](WORD modifiersMask, WORD vkCode) {
                    Logger::trace(L"{} hotkey Ex is invoked from Centralized keyboard hook", modulePtr->get_key());
                    modulePtr->OnHotkeyEx();
                };

                CentralizedHotkeys::AddHotkeyAction({ hotkey.modifiersMask, hotkey.vkCode }, { pt_module->get_key(), action });
            }
            else
            {
                auto action = [modulePtr] {
                    modulePtr->OnHotkeyEx();
                    return false;
                };

                CentralizedKeyboardHook::AddPressedKeyAction2(pt_module->get_key(), VK_LWIN, action);
            }
        }
    }

    // HACK:
    // Just for enabling the shortcut guide legacy behavior of pressing the Windows Key.
    // This is not the sort of behavior we'd like to have generalized on other modules.
    // But this was a way to bring back the long windows key behavior that the community wanted back while maintaining the separate process.
    if (pt_module->keep_track_of_pressed_win_key())
    {
        auto modulePtr = pt_module.get();
        auto action = [modulePtr] {
            modulePtr->OnHotkeyEx();
            return false;
        };
        CentralizedKeyboardHook::AddPressedKeyAction(pt_module->get_key(), VK_LWIN, pt_module->milliseconds_win_key_must_be_pressed(), action);
        CentralizedKeyboardHook::AddPressedKeyAction(pt_module->get_key(), VK_RWIN, pt_module->milliseconds_win_key_must_be_pressed(), action);
    }
}
