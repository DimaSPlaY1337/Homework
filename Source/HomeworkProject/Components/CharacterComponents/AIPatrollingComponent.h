// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Actors/Navigation/PatrollingPath.h"
#include "AIPatrollingComponent.generated.h"

UENUM(BlueprintType)
enum class Patrol : uint8
{
	Circle,
	Loop
};

USTRUCT(BlueprintType)
struct FPatrollingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	Patrol patrol = Patrol::Circle;

};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOMEWORKPROJECT_API UAIPatrollingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	bool CanPatrol() const;
	FVector SelectClosestWayPoint();
	FVector SelectNextWayPoint();

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Path")
	APatrollingPath* PatrollingPath;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Patrol")
	FPatrollingSettings PatrollingSettings;

private:
	int32 CurrentWayPointIndex = -1;
	int8 k = 1;
};
