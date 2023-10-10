// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditorWindowCommands.h"

#define LOCTEXT_NAMESPACE "FEditorWindowModule"

void FEditorWindowCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "EditorWindow", "Bring up EditorWindow window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
