// Fill out your copyright notice in the Description page of Project Settings.

#include "ProceduralRoom.h"
#include "Components/SplineComponent.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Kismet2/KismetEditorUtilities.h"

// Sets default values
AProceduralRoom::AProceduralRoom()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SplineComponent = CreateDefaultSubobject<USplineComponent>("Spline");
	RootComponent = SplineComponent;

	SplineComponent->SetClosedLoop(true);
	SplineComponent->ClearSplinePoints();
	SplineComponent->AddSplineLocalPoint(FVector(0.0f, 0.0f, 0.0f));
	SplineComponent->SetSplinePointType(0, ESplinePointType::Constant);
	SplineComponent->AddSplineLocalPoint(FVector(300.0f, 0.0f, 0.0f));
	SplineComponent->SetSplinePointType(1, ESplinePointType::Constant);
	SplineComponent->AddSplineLocalPoint(FVector(300.0f, 300.0f, 0.0f));
	SplineComponent->SetSplinePointType(2, ESplinePointType::Constant);
	SplineComponent->AddSplineLocalPoint(FVector(0.0f, 300.0f, 0.0f));
	SplineComponent->SetSplinePointType(3, ESplinePointType::Constant);

	PCGComponent = CreateDefaultSubobject<UPCGComponent>("PCG");
	Graph = NewObject<UPCGGraph>();
}

// Called when the game starts or when spawned
void AProceduralRoom::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AProceduralRoom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

