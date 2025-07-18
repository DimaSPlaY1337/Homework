// Fill out your copyright notice in the Description page of Project Settings.


#include "BasPlatform.h"
#include "Components/StaticMeshComponent.h"
#include "Actors/Environment/PlatformTrigger.h"

ABasPlatform::ABasPlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);//включаем репликация перемещений

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	SetRootComponent(PlatformMesh);
}

void ABasPlatform::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(MovementCurve))
	{
		FOnTimelineFloatStatic OnTimelineUpdate;
		OnTimelineUpdate.BindUFunction(this, FName("TickPlatformTimeline"));
		PlatformTimeline.AddInterpFloat(MovementCurve, OnTimelineUpdate);

		FOnTimelineEventStatic OnTimelineFinished;
		OnTimelineFinished.BindUFunction(this, FName("OnPlatfromEndReached"));
		PlatformTimeline.SetTimelineFinishedFunc(OnTimelineFinished);
	}

	if (PlatformBehavior == EPlatfBehavior::Loop)
	{
		MovePlatform();
	}

	StartLocation = GetActorLocation();
	EndLocation = GetActorTransform().TransformPosition(EndPlatformLocation);

	if (IsValid(PlatformTrigger))
	{
		PlatformTrigger->OnTriggerActivated.AddDynamic(this, &ABasPlatform::OnPlatformTriggerActivated);
	}
}

void ABasPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	PlatformTimeline.TickTimeline(DeltaTime);
}

void ABasPlatform::MovePlatform()
{
	if (bIsMovingForward)
	{
		PlatformTimeline.Reverse();
		bIsMovingForward = false;
	}
	else
	{
		PlatformTimeline.Play();
		bIsMovingForward = true;
	}
}

void ABasPlatform::OnPlatformTriggerActivated(bool bIsActivated)
{
	MovePlatform();
}

void ABasPlatform::TickPlatformTimeline(float Value)
{
	FVector NewLocation = FMath::Lerp(StartLocation, EndLocation, Value);
	SetActorLocation(NewLocation);
}

void ABasPlatform::OnPlatfromEndReached()
{
	if (PlatformBehavior == EPlatfBehavior::Loop)
	{
		MovePlatform();
	}
	else if (ReturnTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(ReturnTimer, this, &ABasPlatform::MovePlatform, ReturnTime, false);
		return;
	}
}

