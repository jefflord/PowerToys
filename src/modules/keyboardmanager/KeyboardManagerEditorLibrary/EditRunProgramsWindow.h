#pragma once

namespace KBMEditor
{
    class KeyboardManagerState;
}

class MappingConfiguration;

// Function to create the Edit Keyboard Window
void CreateEditRunProgramsWindow(HINSTANCE hInst, KBMEditor::KeyboardManagerState& keyboardManagerState, MappingConfiguration& mappingConfiguration);

// Function to check if there is already a window active if yes bring to foreground
bool CheckEditRunProgramsWindowActive();

// Function to close any active Edit Keyboard window
void CloseActiveEditRunProgramsWindow();