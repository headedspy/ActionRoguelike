// Fill out your copyright notice in the Description page of Project Settings.

#include "PluginManager.h"
#include <Engine/LevelStreamingDynamic.h>
#include <WorldBrowser/Public/WorldBrowserModule.h>
#include "EditorLevelUtils.h"

UDataTable* PluginManager::GetLevelsDataTable()
{
	return LevelsDataTable;
}

void PluginManager::GetAllLevels(UWorld* world, TSet<ULevelStreaming*>& OutLevels) {
	if (world == nullptr) return;

	FWorldContext& NewWorldContext = GEngine->CreateNewWorldContext(EWorldType::None);
	NewWorldContext.SetCurrentWorld(world);

	TSet<ULevelStreaming*> TempLevels;
	const TArray<ULevelStreaming*> Levels = world->GetStreamingLevels();

	for (ULevelStreaming* Level : Levels)
	{
		OutLevels.Add(Level);
	}

	GEngine->DestroyWorldContext(world);
}

TSet<ULevelStreaming*> PluginManager::LoadFullLevel(UWorld* World, FTransform Transform, FString FolderName, FLinearColor Color)
{
	TSet<ULevelStreaming*> AllLevels;
	TSet<ULevelStreaming*> LoadedLevels;

	bool success = false;

	GetAllLevels(World, AllLevels);

	ULevelStreaming* RootLevelStream = ULevelStreamingDynamic::LoadLevelInstance(GEditor->GetEditorWorldContext().World(),
																				World->GetPathName(),
																				Transform.GetLocation(),
																				Transform.Rotator(),
																				success);
	ensure(success);
	LoadedLevels.Add(RootLevelStream);

	RootLevelStream->RenameForPIE(PIELevelNameCounter++);

	FEditorDelegates::RefreshLevelBrowser.Broadcast();

	// create a folder if there are sublevels
	if (AllLevels.Num() != 0) {
		RootLevelStream->SetFolderPath(FName("/" + FolderName));
	}

	RootLevelStream->LevelColor = Color;


	for (ULevelStreaming* Level : AllLevels) {
		FTransform SubLevelTransform = Level->LevelTransform + Transform;

		ULevelStreaming* LevelStream = ULevelStreamingDynamic::LoadLevelInstance(GEditor->GetEditorWorldContext().World(),
																				Level->GetWorldAssetPackageFName().ToString(),
																				SubLevelTransform.GetLocation(),
																				SubLevelTransform.Rotator(),
																				success);

		ensure(success);

		LevelStream->RenameForPIE(PIELevelNameCounter++);
		LevelStream->SetFolderPath(FName("/" + FolderName));
		LevelStream->LevelColor = Color;

		LoadedLevels.Add(LevelStream);
	}

	if (AllLevels.Num() != 0)
		PIEFolderNameCounter++;

	FEditorDelegates::RefreshLevelBrowser.Broadcast();
	return LoadedLevels;
}

void PluginManager::RemoveSubLevelFromWorld(ULevelStreaming* LevelStream)
{
	TArray<ULevelStreaming*> Levels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();

	for (ULevelStreaming* LoadedLevel : Levels)
	{
		//compare levels by their BuildDataId
		if (LoadedLevel->GetLoadedLevel()->LevelBuildDataId == LevelStream->GetLoadedLevel()->LevelBuildDataId)
		{
			ensure(UEditorLevelUtils::RemoveLevelFromWorld(LoadedLevel->GetLoadedLevel()));
			FEditorDelegates::RefreshLevelBrowser.Broadcast();
			break;
		}
	}
}

void PluginManager::UnloadFullLevel(ULevelStreaming* LevelStream)
{
	FWorldBrowserModule& WBModule = FModuleManager::LoadModuleChecked<FWorldBrowserModule>("WorldBrowser");
	TSharedPtr<FLevelCollectionModel> WorldModel = WBModule.SharedWorldModel((UWorld*)LevelStream->GetLoadedLevel()->GetOuter());

	//remove root level stream
	ensure(UEditorLevelUtils::RemoveLevelFromWorld(LevelStream->GetLoadedLevel()));

	//cleanup empty folder by rebooting worldbrowser module
	WorldModel.Reset();
	WBModule.ShutdownModule();
	WBModule.StartupModule();

	WBModule.OnBrowseWorld.Broadcast(GEditor->GetEditorWorldContext().World());

	FEditorDelegates::RefreshLevelBrowser.Broadcast();
}

