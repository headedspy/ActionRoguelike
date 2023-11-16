// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LoadAllStreamingLevels.generated.h"

/**
 * 
 */
UCLASS()
class EDITORWINDOW_API ULoadAllStreamingLevels : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (DisplayName="Load All Streaming Levels", CompactNodeTitle="Load Levels"), Category = "My Nodes")
	static bool LoadAllStreamingLevels();
	
};
