// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CharacterComponents/AIPatrollingComponent.h"
#include "Actors/Navigation/PatrollingPath.h"

bool UAIPatrollingComponent::CanPatrol() const
{
	return IsValid(PatrollingPath) && PatrollingPath->GetWayPoints().Num() > 0;
}

FVector UAIPatrollingComponent::SelectClosestWayPoint()
{
	FVector OwnerLocation = GetOwner()->GetActorLocation();
	const TArray<FVector> WayPoints = PatrollingPath->GetWayPoints();
	FTransform PathTransform = PatrollingPath->GetActorTransform();

	FVector ClosestWayPoint;
	float MinSqDistance = FLT_MAX;
	for (int32 i = 0; i < WayPoints.Num(); ++i)
	{
		FVector WayPointWorld = PathTransform.TransformPosition(WayPoints[i]);
		float CurrentSqDistance = (OwnerLocation - WayPointWorld).SizeSquared();
		if (CurrentSqDistance < MinSqDistance)
		{
			MinSqDistance = CurrentSqDistance;
			ClosestWayPoint = WayPointWorld;
			CurrentWayPointIndex = i;
		}
	}
	return ClosestWayPoint;
}

FVector UAIPatrollingComponent::SelectNextWayPoint()
{
	if (PatrollingSettings.patrol == Patrol::Circle)
	{
		++CurrentWayPointIndex;
		const TArray<FVector> WayPoints = PatrollingPath->GetWayPoints();
		if (CurrentWayPointIndex == PatrollingPath->GetWayPoints().Num())
		{
			CurrentWayPointIndex = 0;
		}

		FTransform PathTransform = PatrollingPath->GetActorTransform();
		FVector WayPoint = PathTransform.TransformPosition(WayPoints[CurrentWayPointIndex]);
		return WayPoint;
	}
	else
	{
		CurrentWayPointIndex+=k;
		const TArray<FVector> WayPoints = PatrollingPath->GetWayPoints();
		if (CurrentWayPointIndex == PatrollingPath->GetWayPoints().Num() && k==1)
		{
			CurrentWayPointIndex = PatrollingPath->GetWayPoints().Num()-2;
			k = -1;
		}
		else if (CurrentWayPointIndex == -1 && k == -1)
		{
			CurrentWayPointIndex = 1;
			k = 1;
		}

		FTransform PathTransform = PatrollingPath->GetActorTransform();
		FVector WayPoint = PathTransform.TransformPosition(WayPoints[CurrentWayPointIndex]);
		return WayPoint;
	}
}

