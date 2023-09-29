// Fill out your copyright notice in the Description page of Project Settings.


#include "SInteractionComponent.h"
#include "SGameplayInterface.h"

#include "Camera/CameraComponent.h"

// Sets default values for this component's properties
USInteractionComponent::USInteractionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	// ...
	
}


// Called every frame
void USInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void USInteractionComponent::PrimaryInteract()
{
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	AActor* MyOwner = GetOwner();

	FVector EyeLocation;
	FRotator EyeRotation;

	MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	TArray<UCameraComponent*> Comps;

	MyOwner->GetComponents(Comps);
	if (Comps.Num() > 0) {
		UCameraComponent* CamComp = Comps[0];

		EyeLocation = CamComp->GetComponentLocation();
		EyeRotation = CamComp->GetComponentRotation();
	}

	FVector EndLocation = EyeLocation + (EyeRotation.Vector() * 1000.0);

	//FHitResult Hit;
	//bool bBlockingHit = GetWorld()->LineTraceSingleByObjectType(Hit, EyeLocation, EndLocation, ObjectQueryParams);

	float Radius = 30.0f;

	TArray<FHitResult> Hits;
	FCollisionShape Shape;
	Shape.SetSphere(Radius);

	bool bBlockingHit = GetWorld()->SweepMultiByObjectType(Hits, EyeLocation, EndLocation, FQuat::Identity,ObjectQueryParams, Shape);
	FColor DebugColor = bBlockingHit ? FColor::Green : FColor::Red;

	for (FHitResult Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();

		if (HitActor)
		{
			if (HitActor->Implements<USGameplayInterface>())
			{
				APawn* MyPawn = Cast<APawn>(GetOwner());

				ISGameplayInterface::Execute_Interact(HitActor, MyPawn);
				break;
			}
		}

		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, Radius, 16, DebugColor, false, 2.0f, 0, 1.0f);
	}

	DrawDebugLine(GetWorld(), EyeLocation, EndLocation, DebugColor, false, 2.0f, 0, 2.0f);
}
