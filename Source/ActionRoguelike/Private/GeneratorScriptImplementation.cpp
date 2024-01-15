// Fill out your copyright notice in the Description page of Project Settings.

#include "GeneratorScriptImplementation.h"
#include <Core.h>
#include "PluginAPI.h"
#include "Kismet/DataTableFunctionLibrary.h"

void UGeneratorScriptImplementation::OnGenerateButtonPressed()
{
	UDataTable* LevelsDataTable = UPluginAPI::GetLevelsDataTable();
	if (LevelsDataTable == nullptr) {
		UPluginAPI::ShowDialog("DataTable Empty!");
		return;
	}
	UPluginAPI::ClearAllLevels();
	TArray<FString> Levels = UDataTableFunctionLibrary::GetDataTableColumnAsString(LevelsDataTable, "World");
	FVector SpawnPoint = FVector(0.0f ,0.0f, 0.0f);


	int RandomIndex = FMath::RandRange(0, Levels.Num() - 1);
	ULevelStreaming* Level = UPluginAPI::SpawnLevel(Levels[RandomIndex], SpawnPoint, FRotator(0.0f, 0.0f, 0.0f));
	TArray<AGateway*> NewGateways = UPluginAPI::GetLevelGateways(Level);

	for (int i = 0; i < 4; i++)
	{
		RandomIndex = FMath::RandRange(0, Levels.Num()-1);
		Level = UPluginAPI::SpawnLevel(Levels[RandomIndex], SpawnPoint, FRotator(0.0f, 0.0f, 0.0f));
		TArray<AGateway*> Gateways = UPluginAPI::GetLevelGateways(Level);
		UPluginAPI::AttachLevelToGateway(NewGateways[FMath::RandRange(0, NewGateways.Num()-1)], Level, Gateways[FMath::RandRange(0, Gateways.Num()-1)]);
		NewGateways = UPluginAPI::GetLevelGateways(Level);
	}
}
