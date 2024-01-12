// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeneratorScript.h"
#include "GeneratorScriptImplementation.generated.h"

UCLASS()
class ACTIONROGUELIKE_API UGeneratorScriptImplementation : public UClass, public IGeneratorScript
{
	GENERATED_BODY()
public:
	UFUNCTION()
	virtual void OnGenerateButtonPressed() override;
};
