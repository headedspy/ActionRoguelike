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

ULevelStreaming* UPluginAPI::SpawnLevel(FString LevelPath, FVector Position, FRotator Rotation)
{
	UWorld* World = LoadObject<UWorld>(GetTransientPackage(), *LevelPath);
	ensure(World);
	FTransform Transform(Rotation, Position, FVector(1,1,1));

	TSet<ULevelStreaming*> LevelsStreamed = PluginManager::LoadFullLevel(World, Transform, "", FColor::MakeRandomColor());

	return LevelsStreamed.Array()[0];
}

void UPluginAPI::ClearAllLevels()
{
	TArray<ULevelStreaming*> Levels = GEditor->GetEditorWorldContext().World()->GetStreamingLevels();

	for (ULevelStreaming* Level : Levels)
	{
		PluginManager::UnloadFullLevel(Level);
	}
}

TArray<AGateway*> UPluginAPI::GetLevelGateways(ULevelStreaming* Level)
{
	//load unloaded levels
	GWorld->UpdateLevelStreaming();
	TArray<AGateway*> Gateways;

	TArray<AActor*> GatewayActors = Level->GetLoadedLevel()->Actors;

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

void UPluginAPI::AttachLevelToGateway(AGateway* OutGateway, ULevelStreaming* Level, AGateway* InGateway, bool DeleteGateways)
{
	//load unloaded levels
	GWorld->UpdateLevelStreaming();

	FTransform NewTransform;

	float OutYaw = OutGateway->GetTransform().Rotator().Yaw;
	float InYaw = InGateway->GetTransform().Rotator().Yaw;

	//calculate the rotation
	FQuat Rotation = FRotator(0.f, + (180.0f + OutYaw - InYaw), 0.f).Quaternion();
	NewTransform.SetRotation(Rotation);

	//calculate the position
	FVector OutPosition = OutGateway->GetTransform().GetLocation();
	FVector InPosition = InGateway->GetTransform().GetLocation();
	float Distance = FVector::Distance(OutPosition, InPosition + OutPosition);
	FVector ShiftVector = OutGateway->GetActorForwardVector() * Distance;

	NewTransform.SetLocation(OutPosition + ShiftVector);

	//set the level transform
	FLevelUtils::SetEditorTransform(Level, NewTransform);

	//delete the gateways
	if (DeleteGateways)
	{
		GWorld->EditorDestroyActor(OutGateway, true);
		GWorld->EditorDestroyActor(InGateway, true);

		GEditor->ForceGarbageCollection(true);
	}
}