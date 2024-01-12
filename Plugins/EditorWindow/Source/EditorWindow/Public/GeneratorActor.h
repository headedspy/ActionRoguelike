// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PluginManager.h"
#include "GeneratorActor.generated.h"

UCLASS()
class EDITORWINDOW_API AGeneratorActor : public AActor, public PluginManager
{
	GENERATED_BODY()

public:
	UFUNCTION(CallInEditor, BlueprintImplementableEvent)
	void OnGenerateButtonPressed();

};
