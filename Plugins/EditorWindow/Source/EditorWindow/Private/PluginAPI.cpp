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
	PluginManager::ClearAll();
}

TArray<AGateway*> UPluginAPI::GetLevelGateways(ULevelStreaming* Level)
{
	//load unloaded levels
	GWorld->UpdateLevelStreaming();

	bool bGetOnlyEntryGateways = false;

	TArray<AGateway*> Gateways;

	TArray<AActor*> GatewayActors = Level->GetLoadedLevel()->Actors;

	for (AActor* GatewayActor : GatewayActors)
	{
		if (GatewayActor == nullptr) continue;
		if(GatewayActor->IsA(AGateway::StaticClass()))
		{
			AGateway* Gateway = Cast<AGateway>(GatewayActor);
			if (Gateway->EntryGateway) bGetOnlyEntryGateways = true;
			Gateways.Add(Gateway);
		}
	}

	if (bGetOnlyEntryGateways)
	{
		Gateways.RemoveAll([](AGateway* Gateway) {return !(Gateway->EntryGateway); });
	}

	return Gateways;
}

void UPluginAPI::AttachLevelToGateway(AGateway* OutGateway, ULevelStreaming* Level, AGateway* InGateway, bool DeleteGateways)
{
	if (OutGateway == nullptr)
	{
		ShowDialog("OutGateway is NULL");
		return;
	}
	if (InGateway == nullptr)
	{
		ShowDialog("InGateway is NULL");
		return;
	}

	//load unloaded levels
	GWorld->UpdateLevelStreaming();

	FTransform NewTransform;

	float OutYaw = OutGateway->GetActorTransform().Rotator().Yaw;
	float InYaw = InGateway->GetActorTransform().Rotator().Yaw;
	float RotationAngle = 180.0f + OutYaw - InYaw;

	//calculate the rotation
	FQuat Rotation = FRotator(0.f, RotationAngle, 0.f).Quaternion();
	NewTransform.SetRotation(Rotation);

	//calculate the position
	FVector OutPosition = OutGateway->GetTransform().GetLocation();
	FVector InPosition = InGateway->GetTransform().GetLocation();

	//rotate the in position by the calculated angle
	InPosition = InPosition.RotateAngleAxis(RotationAngle, InPosition.ZAxisVector);

	NewTransform.SetLocation(OutPosition - InPosition);

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