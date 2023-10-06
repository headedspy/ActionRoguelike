// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SPowerUpBase.h"
#include "SHealthPotion.generated.h"

class UStaticMeshComponent;

/**
 * 
 */
UCLASS()
class ACTIONROGUELIKE_API ASHealthPotion : public ASPowerUpBase
{
	GENERATED_BODY()
public:
	ASHealthPotion();

protected:

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* MeshComp;

	bool IsActive;
	float HealAmount;
	FTimerHandle TimerHandle;

	void Reactivate();

	void Interact_Implementation(APawn* InstigatorPawn);
	
};
