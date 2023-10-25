// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

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

	void ChangedState(ECheckBoxState newState);

	FReply BuildButtonClicked();
	FReply ReplaceButtonClicked();

	AActor* AddActor(TSubclassOf<AActor> ActorClass, FTransform Transform);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	UPROPERTY(EditDefaultsOnly)
	UDataTable* DataTable;

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	UPROPERTY()
	FString DataTablePath;

	unsigned short Counter;
};
