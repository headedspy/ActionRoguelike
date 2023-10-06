// Fill out your copyright notice in the Description page of Project Settings.


#include "SHealthPotion.h"
#include "SAttributeComponent.h"
#include "Components/StaticMeshComponent.h"

ASHealthPotion::ASHealthPotion()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>("MeshComp");
	RootComponent = MeshComp;

	IsActive = true;
	HealAmount = 40.0f;
}

void ASHealthPotion::Reactivate()
{
	IsActive = true;
	MeshComp->SetVisibility(true);
}

void ASHealthPotion::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!IsActive) return;
	USAttributeComponent* AttributeComponent = Cast<USAttributeComponent>(InstigatorPawn->GetComponentByClass(USAttributeComponent::StaticClass()));

	if (AttributeComponent->IsMaxHealth()) return;

	AttributeComponent->ApplyHealthChange(HealAmount);
	
	IsActive = false;
	MeshComp->SetVisibility(false);
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ASHealthPotion::Reactivate, 10.0f);
}
