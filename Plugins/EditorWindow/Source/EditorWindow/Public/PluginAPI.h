// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/LevelStreaming.h"
#include "PluginAPI.generated.h"

UCLASS(Blueprintable)
class EDITORWINDOW_API AGateway : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	bool EntryGateway;
};

UCLASS()
class EDITORWINDOW_API UPluginAPI : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// display a dialog window with a custom message
	UFUNCTION(BlueprintCallable, Category="PluginAPI")
	static void ShowDialog(FString InputString);

	// get the levels datatable
	UFUNCTION(BlueprintPure, Category="PluginAPI")
	static UDataTable* GetLevelsDataTable();

	// spawn level from world asset path, returns all spawned levels
	UFUNCTION(BlueprintCallable, Category="PluginAPI")
	static ULevelStreaming* SpawnLevel(FString LevelPath, FVector Position, FRotator Rotation);

	// delete all streamed levels from the editor world
	UFUNCTION(BlueprintCallable, Category="PluginAPI")
	static void ClearAllLevels();

	// get all the gateways for the laoded level
	UFUNCTION(BlueprintCallable, Category="PluginAPI")
	static TArray<AGateway*> GetLevelGateways(ULevelStreaming* Level);

	// connect the given level to a gateway
	UFUNCTION(BlueprintCallable, Category = "PluginAPI")
	static void AttachLevelToGateway(AGateway* OutGateway, ULevelStreaming* Level, AGateway* InGateway, bool DeleteGateways = true);
};
