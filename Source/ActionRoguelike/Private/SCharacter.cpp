// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"

#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "SInteractionComponent.h"

#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

bool ASCharacter::bActiveTeleport = false;

// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->SetupAttachment(RootComponent);

	CameraComp = CreateDefaultSubobject<UCameraComponent>("CameraComp");
	CameraComp->SetupAttachment(SpringArmComp);

	InteractionComp = CreateDefaultSubobject<USInteractionComponent>("InteractionComp");

	SpringArmComp->bUsePawnControlRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ASCharacter::MoveForward(float value)
{
	FRotator ControlRot = GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	AddMovementInput(ControlRot.Vector(), value);
}

void ASCharacter::MoveRight(float value)
{
	FRotator ControlRot = GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	FVector RightVector = UKismetMathLibrary::GetRightVector(ControlRot);

	AddMovementInput(RightVector, value);
}

void ASCharacter::PrimaryAttack() 
{
	PlayAnimMontage(AttackAnim);

	GetWorldTimerManager().SetTimer(TimerHandle, this, &ASCharacter::PrimaryAttack_TimeElapsed, 0.2f);
}

FTransform ASCharacter::ProjectileTransform() {
	FHitResult LineTraceHit;
	FVector StartPos = CameraComp->GetComponentTransform().GetLocation();
	FVector EndPos = StartPos + (CameraComp->GetComponentTransform().GetRotation().Vector() * 2000.0f);
	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	QueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	bool hit = GetWorld()->LineTraceSingleByObjectType(LineTraceHit, StartPos, EndPos, QueryParams);

	FVector HandLocation = GetMesh()->GetSocketLocation("Muzzle_01");
	FRotator SpawnRotation = GetActorRotation();
	if (hit) {
		SpawnRotation = UKismetMathLibrary::FindLookAtRotation(HandLocation, LineTraceHit.ImpactPoint);
	}
	else {
		SpawnRotation = UKismetMathLibrary::FindLookAtRotation(HandLocation, EndPos);
	}

	return FTransform(SpawnRotation, HandLocation);
}

void ASCharacter::PrimaryAttack_TimeElapsed()
{
	FTransform SpawnTM = ProjectileTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Instigator = this;

	GetWorld()->SpawnActor<AActor>(MagicProjectileClass, SpawnTM, SpawnParams);
}

void ASCharacter::SecondaryAttack() 
{
	PlayAnimMontage(AttackAnim);

	GetWorldTimerManager().SetTimer(TimerHandle, this, &ASCharacter::SecondaryAttack_TimeElapsed, 0.2f);
}

void ASCharacter::SecondaryAttack_TimeElapsed() 
{
	FTransform SpawnTM = ProjectileTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Instigator = this;

	GetWorld()->SpawnActor<AActor>(BlackHoleProjectileClass, SpawnTM, SpawnParams);
}

void ASCharacter::Teleport()
{
	if (bActiveTeleport) return;

	bActiveTeleport = true;
	PlayAnimMontage(AttackAnim);

	GetWorldTimerManager().SetTimer(TimerHandle, this, &ASCharacter::Teleport_TimeElapsed, 0.2f);
}

void ASCharacter::Teleport_TimeElapsed()
{
	FTransform SpawnTM = ProjectileTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Instigator = this;

	GetWorld()->SpawnActor<AActor>(TeleportProjectileClass, SpawnTM, SpawnParams);
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ASCharacter::ResetActiveTeleport, 2.0f);
}

void ASCharacter::ResetActiveTeleport() {
	bActiveTeleport = false;
}

void ASCharacter::PrimaryInteract()
{
	if (InteractionComp) {
		InteractionComp->PrimaryInteract();
	}
}

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// -- Rotation Visualization -- //
	const float DrawScale = 100.0f;
	const float Thickness = 5.0f;

	FVector LineStart = GetActorLocation();
	// Offset to the right of pawn
	LineStart += GetActorRightVector() * 100.0f;
	// Set line end in direction of the actor's forward
	FVector ActorDirection_LineEnd = LineStart + (GetActorForwardVector() * 100.0f);
	// Draw Actor's Direction
	DrawDebugDirectionalArrow(GetWorld(), LineStart, ActorDirection_LineEnd, DrawScale, FColor::Yellow, false, 0.0f, 0, Thickness);

	FVector ControllerDirection_LineEnd = LineStart + (GetControlRotation().Vector() * 100.0f);
	// Draw 'Controller' Rotation ('PlayerController' that 'possessed' this character)
	DrawDebugDirectionalArrow(GetWorld(), LineStart, ControllerDirection_LineEnd, DrawScale, FColor::Green, false, 0.0f, 0, Thickness);
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("PrimaryAttack", IE_Pressed, this, &ASCharacter::PrimaryAttack);
	PlayerInputComponent->BindAction("SecondaryAttack", IE_Pressed, this, &ASCharacter::SecondaryAttack);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASCharacter::Jump);

	PlayerInputComponent->BindAction("PrimaryInteract", IE_Pressed, this, &ASCharacter::PrimaryInteract);

	PlayerInputComponent->BindAction("Teleport", IE_Pressed, this, &ASCharacter::Teleport);

}

