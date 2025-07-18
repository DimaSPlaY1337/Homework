// Fill out your copyright notice in the Description page of Project Settings.

#include "AITurretController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "AI/Characters/Turret.h"
#include "Perception/AISense_Damage.h"
#include <iostream>

void AAITurretController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	if (IsValid(InPawn))
	{
		checkf(InPawn->IsA<ATurret>(), TEXT("AAITurretController::SetPawn AAITurretController can possess only Turrets"));
		CachedTurret = StaticCast<ATurret*>(InPawn);
	}
	else
	{
		CachedTurret = nullptr;
	}
}

void AAITurretController::ActorsPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	if (!CachedTurret->TurretAttributesComponent->IsAlive())
	{
		return;
	}
	Super::ActorsPerceptionUpdated(UpdatedActors);
	if (!CachedTurret.IsValid())
	{
		return;
	}

	AActor* ClosestSightActor = GetClosestSensedActor(UAISense_Sight::StaticClass());
	AActor* ClosestDamageActor = GetClosestSensedActor(UAISense_Damage::StaticClass());
	if (Shots != CachedTurret->GetShots())
	{
		Shots = CachedTurret->GetShots();
		CachedTurret->TurnTurretToTarget(ClosestDamageActor);
	}
	else
	{
		CachedTurret->CurrentTarget = ClosestSightActor;
		CachedTurret->OnCurrentTargetSet();
	}
}
