// Fill out your copyright notice in the Description page of Project Settings.


#include "STeleportProjectile.h"
#include "Kismet/GameplayStatics.h"


void ASTeleportProjectile::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(TimerHandle, this, &ASTeleportProjectile::Teleport, 2.0f);
}

void ASTeleportProjectile::Teleport()
{
	FTransform SpawnTM;
	SpawnTM.SetLocation(GetActorLocation());

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleSystem, SpawnTM);

	GetInstigator()->SetActorLocation(GetActorLocation());
	Destroy();
}
