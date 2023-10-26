#pragma once

#include <keyboardmanager/common/Shortcut.h>

namespace KBMEditor
{
    class KeyboardManagerState;
}

class KeyDropDownControl;
namespace winrt::Windows::UI::Xaml
{
    struct XamlRoot;
    namespace Controls
    {
        struct StackPanel;
        struct TextBox;
        struct Button;
    }
}

class RunProgramControl
{
private:
    // Wrap grid for the drop downs to display the selected runProgram
    winrt::Windows::Foundation::IInspectable runProgramDropDownVariableSizedWrapGrid;

    // Button to type the runProgram
    winrt::Windows::Foundation::IInspectable typeRunProgram;

    // StackPanel to parent the above controls
    winrt::Windows::Foundation::IInspectable runProgramControlLayout;

    // Function to set the accessible name of the target app text box
    static void SetAccessibleNameForTextBox(TextBox targetAppTextBox, int rowIndex);

    // Function to set the accessible names for all the controls in a row
    static void UpdateAccessibleNames(StackPanel sourceColumn, StackPanel mappedToColumn, TextBox targetAppTextBox, Button deleteButton, int rowIndex);

    // Needed to parse the targetApp string
    static std::vector<std::wstring> RunProgramControl::splitString(const std::wstring& str, const std::wstring& delimiter, bool ignoreEmpty);

    // Needed to clean some strings
    static void removeChars(std::wstring& str, const wchar_t* removeList);

    // Wrapper to clean some strings
    static void RunProgramControl::removeQuotes(std::wstring& str);

public:
    // Handle to the current Edit RunProgram Window
    static HWND editRunProgramsWindowHandle;

    // Pointer to the keyboard manager state
    static KBMEditor::KeyboardManagerState* keyboardManagerState;

    // Stores the current list of remappings
    static RemapBuffer runProgramRemapBuffer;

    // Vector to store dynamically allocated KeyDropDownControl objects to avoid early destruction
    std::vector<std::unique_ptr<KeyDropDownControl>> keyDropDownControlObjects;

    // constructor
    RunProgramControl(StackPanel table, StackPanel row, const int colIndex, TextBox targetApp, TextBox targetAppArgs, TextBox targetAppDir);

    // Function to add a new row to the runProgram table. If the originalKeys and newKeys args are provided, then the displayed runPrograms are set to those values.
    static void AddNewRunProgramControlRow(StackPanel& parent, std::vector<std::vector<std::unique_ptr<RunProgramControl>>>& keyboardRemapControlObjects, const Shortcut& originalKeys = Shortcut(), const std::wstring& targetAppName = L"");

    // Function to return the stack panel element of the RunProgramControl. This is the externally visible UI element which can be used to add it to other layouts
    StackPanel GetRunProgramControl();

    // Function to create the detect runProgram UI window
    static void CreateDetectRunProgramWindow(winrt::Windows::Foundation::IInspectable const& sender, XamlRoot xamlRoot, KBMEditor::KeyboardManagerState& keyboardManagerState, const int colIndex, StackPanel table, std::vector<std::unique_ptr<KeyDropDownControl>>& keyDropDownControlObjects, StackPanel controlLayout, TextBox targetApp, TextBox targetAppArgs, TextBox targetAppDir, bool isHybridControl, bool isSingleKeyWindow, HWND parentWindow, RemapBuffer& remapBuffer);
};
