// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralRoom.generated.h"

class USplineComponent;
class UPCGComponent;
class UPCGGraph;

UCLASS()
class EDITORWINDOW_API AProceduralRoom : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProceduralRoom();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	USplineComponent* SplineComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPCGComponent* PCGComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPCGGraph* Graph;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void InitializeGraph();

	UPROPERTY(EditAnywhere)
	UStaticMesh* FloorMesh;

};
