// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "EditorWindow.generated.h"

// Struct to use in creating the datatable
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
class UUserDefinedStruct;


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

	AActor* AddActor(TSubclassOf<AActor> ActorClass, FTransform Transform);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);



private:
	TSharedPtr<class FUICommandList> PluginCommands;

	UPROPERTY()
	FString DataTablePath;

	UPROPERTY(EditDefaultsOnly)
	UDataTable* DataTable;

	unsigned short PIERenameCounter;

	int32 BuildNum;
	int32 SeedNum;

	bool DeleteOnReplace;

	bool ErrorCheck();

	void CheckboxButtonStateChanged(ECheckBoxState IState);

	void GetAllLevels(UWorld* world, TSet<ULevelStreaming*>& OutLevels);
	void GetAllSubLevels(ULevelStreaming* level, TSet<ULevelStreaming*>& OutLevels, bool AlterTransform);
	FString ClearPathFormatting(FString InputString, FString RemoveFrom);

	bool LoadFullLevel(UWorld* World, FTransform Transform, bool AddToGameLevels=true);

	TMap<ULevelStreaming*, TSet<ULevelStreaming*>> GameLevels;
	TSet<UWorld*> LevelsWithAlteredTransform;
};
