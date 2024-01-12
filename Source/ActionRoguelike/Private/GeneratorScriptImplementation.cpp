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

	for (int i = 0; i < 5; i++)
	{
		int RandomIndex = FMath::RandRange(0, Levels.Num()-1);
		UPluginAPI::SpawnLevel(Levels[RandomIndex], SpawnPoint, FRotator(0.0f, 0.0f, 0.0f));
		SpawnPoint += FVector(1000.0f, 0.0f, 0.0f);
	}
}
