// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SProjectileBase.h"
#include "STeleportProjectile.generated.h"

class UParticleSystem;

/**
 * 
 */
UCLASS()
class ACTIONROGUELIKE_API ASTeleportProjectile : public ASProjectileBase
{
	GENERATED_BODY()
	
	void BeginPlay();

	void Teleport();

	UFUNCTION()
	void CollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:

	UPROPERTY(EditAnywhere)
	UParticleSystem* ParticleSystem;

	FTimerHandle TimerHandle;

};
