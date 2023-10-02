// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystemComponent;

UCLASS()
class ACTIONROGUELIKE_API ASProjectileBase : public AActor
{
	GENERATED_BODY()

	
public:	
	// Sets default values for this actor's properties
	ASProjectileBase();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		USphereComponent* SphereComp;

	UPROPERTY(VisibleAnywhere)
		UProjectileMovementComponent* ProjectileMovementComp;

	UPROPERTY(VisibleAnywhere)
		UParticleSystemComponent* ParticleSystemComp;

	float ProjectileSpeed = 100.0f;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
