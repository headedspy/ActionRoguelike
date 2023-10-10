// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorWindowStyle.h"

class FEditorWindowCommands : public TCommands<FEditorWindowCommands>
{
public:

	FEditorWindowCommands()
		: TCommands<FEditorWindowCommands>(TEXT("EditorWindow"), NSLOCTEXT("Contexts", "EditorWindow", "EditorWindow Plugin"), NAME_None, FEditorWindowStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};