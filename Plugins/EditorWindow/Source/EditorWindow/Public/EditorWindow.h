// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "EditorWindow.generated.h"

// Struct to use for creating the datatable
USTRUCT(BlueprintType)
struct FWorldStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UWorld> World;

	UPROPERTY(EditAnywhere)
	TSet<TSoftObjectPtr<UWorld>> ReplaceWorlds;
};

class FToolBarBuilder;
class FMenuBuilder;
class FReply;

class FEditorWindowModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
private:

	void RegisterMenus();

	FReply BuildButtonClicked();
	FReply ReplaceButtonClicked();
	FReply ChangeStreamingLevelsMode();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	UPROPERTY()
	FString DataTablePath;

	UPROPERTY(EditDefaultsOnly)
	UDataTable* DataTable;

	uint16 BuildNum;
	uint16 SeedNum;

	bool RandomizeSeed;

	unsigned short PIEFolderNameCounter;

	bool ErrorCheck();

	// streamed level and his streamed sub-levels
	TMap<ULevelStreaming*, TSet<ULevelStreaming*>> GameLevels;

	// original levels and their replacement level
	TSet<TPair<ULevelStreaming*, ULevelStreaming*>> ReplacementLevels;

	// get all sublevels contained in a world
	void GetAllLevels(UWorld* world, TSet<ULevelStreaming*>& OutLevels);

	FString ClearPathFormatting(FString InputString, FString RemoveFrom);

	//returns root level stream or nullptr if fail
	ULevelStreaming* LoadFullLevel(UWorld* World, FTransform Transform);

	bool UnloadFullLevel(ULevelStreaming* LevelStream);

	//helper function to remove sublevel form edito world
	void RemoveSubLevelFromWorld(ULevelStreaming* LevelStream);

};
