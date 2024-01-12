// Fill out your copyright notice in the Description page of Project Settings.

#include "PluginAPI.h"
#include <Engine/LevelStreamingDynamic.h>
#include <Kismet/GameplayStatics.h>
#include "PluginManager.h"
#include <LevelUtils.h>

void UPluginAPI::ShowDialog(FString inputString)
{
	FText DialogText = FText::FromString(inputString);
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);
}

UDataTable* UPluginAPI::GetLevelsDataTable()
{
	return PluginManager::GetLevelsDataTable();
}

TSet<ULevelStreaming*> UPluginAPI::SpawnLevel(FString LevelPath, FVector Position, FRotator Rotation)
{
	UWorld* World = LoadObject<UWorld>(GetTransientPackage(), *LevelPath);
	ensure(World);
	FTransform Transform(Rotation, Position, FVector(1,1,1));

	TSet<ULevelStreaming*> LevelsStreamed = PluginManager::LoadFullLevel(World, Transform, "", FColor::MakeRandomColor());

	return LevelsStreamed;
}

void UPluginAPI::ClearAllLevels()
{
	TArray<ULevelStreaming*> Levels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();

	for (ULevelStreaming* Level : Levels)
	{
		PluginManager::UnloadFullLevel(Level);
	}
}

TArray<AGateway*> UPluginAPI::GetLevelGateways(TSet<ULevelStreaming*> Level)
{
	//load unloaded levels
	GWorld->UpdateLevelStreaming();
	TArray<AGateway*> Gateways;

	ULevelStreaming* RootLevel = Level.Array()[0];
	TArray<AActor*> GatewayActors = RootLevel->GetLoadedLevel()->Actors;

	for (AActor* GatewayActor : GatewayActors)
	{
		if (GatewayActor == nullptr) continue;
		if(GatewayActor->IsA(AGateway::StaticClass()))
		{
			Gateways.Add(Cast<AGateway>(GatewayActor));
		}
	}

	return Gateways;
}

void UPluginAPI::AttachLevelToGateway(AGateway* OutGateway, TSet<ULevelStreaming*> Level, AGateway* InGateway, bool DeleteGateways)
{
	//load unloaded levels
	GWorld->UpdateLevelStreaming();

	FTransform NewTransform;

	float OutYaw = OutGateway->GetTransform().Rotator().Yaw;
	float InYaw = InGateway->GetTransform().Rotator().Yaw;

	FQuat Rotation = FRotator(0.f, + (180.0f + OutYaw - InYaw), 0.f).Quaternion();
	NewTransform.SetRotation(Rotation);

	FVector OutPosition = OutGateway->GetTransform().GetLocation();
	FVector InPosition = InGateway->GetTransform().GetLocation();
	float Distance = FVector::Distance(OutPosition, InPosition + OutPosition);
	FVector ShiftVector = OutGateway->GetActorForwardVector() * Distance;

	NewTransform.SetLocation(OutPosition + ShiftVector);

	for (ULevelStreaming* LevelStream : Level)
	{
		FLevelUtils::SetEditorTransform(LevelStream, NewTransform);
	}

	if (DeleteGateways)
	{
		GWorld->EditorDestroyActor(OutGateway, true);
		GWorld->EditorDestroyActor(InGateway, true);

		GEditor->ForceGarbageCollection(true);
	}
}