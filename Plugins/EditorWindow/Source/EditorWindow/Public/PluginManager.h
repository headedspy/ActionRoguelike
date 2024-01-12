// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


class EDITORWINDOW_API PluginManager
{
protected:
	// the levels datatable given in the UI
	UPROPERTY(EditDefaultsOnly)
	static inline UDataTable* LevelsDataTable;

	// counters for renaming the folder/level in the editor
	static inline unsigned short PIELevelNameCounter;
	static inline unsigned short PIEFolderNameCounter;

	// original levels and their replacement levels
	static inline TMap<ULevelStreaming*, TSet<ULevelStreaming*>> ReplacementLevels;
	//static inline TSet<TPair<ULevelStreaming*, ULevelStreaming*>> ReplacementLevels;

	// actors and their replacement blueprint actors
	static inline TMap<AActor*, AActor*> ReplacementActors;

public:
	static UDataTable* GetLevelsDataTable();

	// get all sublevels contained in a world
	static void GetAllLevels(UWorld* world, TSet<ULevelStreaming*>& OutLevels);

	//returns level streams
	static TSet<ULevelStreaming*> LoadFullLevel(UWorld* World, FTransform Transform, FString FolderName = "", FLinearColor Color = FLinearColor::White);

	// remove sublevel from editor world
	static void RemoveSubLevelFromWorld(ULevelStreaming* LevelStream);

	// fully remove level from the editor world
	static void UnloadFullLevel(ULevelStreaming* LevelStream);

};
