// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include <UObject/ObjectMacros.h>
#include "PluginManager.h"
#include "GeneratorActor.h"
#include "EditorWindow.generated.h"


// Helper structs (TPair cant work on UPROPERTY)
USTRUCT(BlueprintType)
struct FWeightedWorld
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UWorld> World;

	UPROPERTY(EditAnywhere)
	uint16 Weight;
};

USTRUCT(BlueprintType)
struct FWeightedActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> Actor;

	UPROPERTY(EditAnywhere)
	uint16 Weight;
};

// hash function for the helper stucts, used when they get put in a set
inline uint32 GetTypeHash(const FWeightedWorld& World)
{
	return FCrc::MemCrc32(&World, sizeof(FWeightedWorld));
}

inline uint32 GetTypeHash(const FWeightedActor& Actor)
{
	return FCrc::MemCrc32(&Actor, sizeof(FWeightedActor));
}


// Structs to use for the levels datatables
USTRUCT(BlueprintType)
struct FLevelsStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UWorld> World;

	UPROPERTY(EditAnywhere)
	TSet<FWeightedWorld> ReplaceWorlds;
};


USTRUCT(BlueprintType)
struct FTagsStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName ActorTag;

	UPROPERTY(EditAnywhere)
	uint8 NumberOfElements;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ReplaceActor;
};

USTRUCT(BlueprintType)
struct FActorsStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> Actor;

	UPROPERTY(EditAnywhere)
	TSet<FWeightedActor> ReplaceActors;
};

// enum for generation algorithm execution method
#undef CPP
enum ExecutionMethod : uint8 { Python, Blueprint, CPP };

// helper function to convert the enum to string for the UI
inline FString ExecutionMethodToString(ExecutionMethod Method)
{
	switch (Method)
	{
	case ExecutionMethod::Python:
		return "Python";
	case ExecutionMethod::Blueprint:
		return "Blueprint";
	case ExecutionMethod::CPP:
		return "C++";
	}
	return "INVALID";
}

class FReply;
class FEditorWindowModule : public IModuleInterface, public PluginManager
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
private:

	void RegisterMenus();

	FReply GenerateLevelsButtonClicked();
	FReply GenerateTagsButtonClicked();
	FReply GenerateActorsButtonClicked();
	FReply MergeButtonClicked();
	FReply OpenFileButtonClicked();
	FReply ExecuteScriptButtonClicked();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	UDataTable* ActorsDataTable;
	UDataTable* TagsDataTable;

	// paths for the selected assets used in the dropdown menus
	FString GeneratorClassPath;
	FString LevelsDataTablePath;
	FString TagsDataTablePath;
	FString ActorsDataTablePath;
	FString GeneratorBPPath;

	// selected generator assets
	AGeneratorActor* GeneratorBP;
	UClass* GeneratorClass;

	// generation randomization seeds
	bool RandomizeLevelsSeed;
	bool RandomizeTagsSeed;
	bool RandomizeActorsSeed;
	uint16 LevelsSeedNum;
	uint16 TagsSeedNum;
	uint16 ActorsSeedNum;

	// Used by the execution method combobox selector
	TArray<TSharedPtr<ExecutionMethod>> ComboItems;
	TSharedPtr<STextBlock> ComboBoxTitleBlock;

	// selected execution method
	ExecutionMethod Selection;

	// path to the selected python script
	FString FilePath;

	// check the Value statement and log a message if it's false; returns the Value
	bool CheckAndLog(bool Value, FString MessageToLog);

	// clear all engine formatting of a level path
	FString ClearPathFormatting(FString InputString);


	// binded to FEditorDelegates::OnAddLevelToWorld event, invoked after a level is added through the levels menu
	UFUNCTION()
	void ManualAddLevel(ULevel* Level);

	// binded to FEditorDelegates::MapChange event, invoked when the map is changed in the editor
	UFUNCTION()
	void EditorMapChange(uint32 flags);

	// move all actors from the given level into the persistent level
	void MoveAllActorsFromLevel(ULevelStreaming* LevelStream);

	// return random actor from a list of weighted actors using a given random stream
	UClass* WeightedRandomActor(TSet<FWeightedActor> WeightedActors, FRandomStream& RandomStream, uint16 SumOfWeights);
};
