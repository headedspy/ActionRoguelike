// Fill out your copyright notice in the Description page of Project Settings.

#include "ProceduralRoom.h"
#include "Components/SplineComponent.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Kismet2/KismetEditorUtilities.h"
#include <Elements/PCGSplineSampler.h>
#include <Elements/PCGStaticMeshSpawner.h>
#include "UObject/ConstructorHelpers.h"
#include <MeshSelectors/PCGMeshSelectorWeighted.h>

// Sets default values
AProceduralRoom::AProceduralRoom()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SplineComponent = CreateDefaultSubobject<USplineComponent>("Spline");
	RootComponent = SplineComponent;
	PCGComponent = CreateDefaultSubobject<UPCGComponent>("PCG");

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

void AProceduralRoom::InitializeGraph()
{
	Graph = NewObject<UPCGGraph>();
	PCGComponent->SetGraphLocal(Graph);

	UPCGSplineSamplerSettings* SplineSamplerNodeSettings = NewObject<UPCGSplineSamplerSettings>();
	SplineSamplerNodeSettings->Params.Dimension = EPCGSplineSamplingDimension::OnInterior;

	UPCGNode* SplineSamplerNode = Graph->AddNode(SplineSamplerNodeSettings);

	//TODO: set mesh instance
	UPCGStaticMeshSpawnerSettings* StaticMeshSpawnerSettings = NewObject<UPCGStaticMeshSpawnerSettings>();
	UPCGMeshSelectorBase* MeshSelector = NewObject<UPCGMeshSelectorWeighted>();

	FPCGContext Context;
	UPCGSpatialData* SpatialData = nullptr;

	TArray<FPCGMeshInstanceList> MeshInstances;
	FCollisionProfileName CollisionProfileName;
	TArray<UMaterialInterface*> MaterialOverrides;
	MeshInstances.Add(FPCGMeshInstanceList(FloorMesh, false, CollisionProfileName, false, MaterialOverrides, 10.0f, 1000.0f));

	UPCGPointData* OutPointData = nullptr;
	MeshSelector->SelectInstances(Context, StaticMeshSpawnerSettings, SpatialData, MeshInstances, OutPointData);

	StaticMeshSpawnerSettings->MeshSelectorInstance = MeshSelector;

	UPCGNode* StaticMeshSpawnerNode = Graph->AddNode(StaticMeshSpawnerSettings);


	Graph->AddLabeledEdge(Graph->GetInputNode(), FName("In"), SplineSamplerNode, FName("In"));
	Graph->AddLabeledEdge(SplineSamplerNode, FName("Out"), StaticMeshSpawnerNode, FName("In"));
	Graph->AddLabeledEdge(StaticMeshSpawnerNode, FName("Out"), Graph->GetOutputNode(), FName("Out"));
}

