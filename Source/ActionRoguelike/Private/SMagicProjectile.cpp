// Fill out your copyright notice in the Description page of Project Settings.

#include "SMagicProjectile.h"

#include "Components/SphereComponent.h"
#include "SAttributeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"


ASMagicProjectile::ASMagicProjectile() {
	SphereComp->OnComponentBeginOverlap.AddDynamic(this, &ASMagicProjectile::OnActorOverlap);
	//FVector HandLocation = GetMesh()->GetSocketLocation("Muzzle_01");
}

void ASMagicProjectile::BeginPlay()
{
	UGameplayStatics::SpawnEmitterAttached(MuzzleParticleClass, SphereComp);
	Super::BeginPlay();
}

void ASMagicProjectile::OnActorOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSoundBase, OtherActor->GetActorLocation());
	if (OtherActor && OtherActor != GetInstigator())
	{
		USAttributeComponent* AttributeComp = Cast<USAttributeComponent>(OtherActor->GetComponentByClass(USAttributeComponent::StaticClass()));

		if (AttributeComp) {
			UGameplayStatics::PlayWorldCameraShake(GetWorld(), CameraShakeDamage, OtherActor->GetActorLocation(), 0.0f, 1000.0f);
			AttributeComp->ApplyHealthChange(-20.0f);
			Destroy();
		}
	}
}
