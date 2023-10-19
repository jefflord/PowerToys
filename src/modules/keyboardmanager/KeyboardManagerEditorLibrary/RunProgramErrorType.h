#pragma once

// Type to store codes for different errors
enum class RunProgramErrorType
{
    NoError,
    SameKeyPreviouslyMapped,
    MapToSameKey,
    ConflictingModifierKey,
    SameRunProgramPreviouslyMapped,
    MapToSameRunProgram,
    ConflictingModifierRunProgram,
    WinL,
    CtrlAltDel,
    RemapUnsuccessful,
    SaveFailed,
    RunProgramStartWithModifier,
    RunProgramCannotHaveRepeatedModifier,
    RunProgramAtleast2Keys,
    RunProgramOneActionKey,
    RunProgramNotMoreThanOneActionKey,
    RunProgramMaxRunProgramSizeOneActionKey,
    RunProgramDisableAsActionKey
};