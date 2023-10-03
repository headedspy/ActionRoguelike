// Fill out your copyright notice in the Description page of Project Settings.


#include "STeleportProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"


void ASTeleportProjectile::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(TimerHandle, this, &ASTeleportProjectile::Teleport, 2.0f);
	//BoxComp->OnComponentHit.AddDynamic(this, &ASExplosiveBarrel::Explode);
	SphereComp->OnComponentHit.AddDynamic(this, &ASTeleportProjectile::CollisionHit);
}

void ASTeleportProjectile::CollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) {
	GetWorldTimerManager().ClearTimer(TimerHandle);
	Teleport();
}

void ASTeleportProjectile::Teleport()
{
	FTransform SpawnTM;
	SpawnTM.SetLocation(GetActorLocation());

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleSystem, SpawnTM);

	GetInstigator()->SetActorLocation(GetActorLocation());
	Destroy();
}
