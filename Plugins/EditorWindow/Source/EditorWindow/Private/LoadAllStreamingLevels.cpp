// Fill out your copyright notice in the Description page of Project Settings.


#include "LoadAllStreamingLevels.h"
#include <EditorLevelUtils.h>
#include "Engine/LevelStreamingAlwaysLoaded.h"

bool ULoadAllStreamingLevels::LoadAllStreamingLevels()
{
	TArray<ULevelStreaming*> Levels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();

	for (ULevelStreaming* LevelStream : Levels)
	{
		UEditorLevelUtils::SetStreamingClassForLevel(LevelStream, ULevelStreamingAlwaysLoaded::StaticClass());
	}

	return true;
}
