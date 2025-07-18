// Fill out your copyright notice in the Description page of Project Settings.


#include "PlatformTrigger.h"
#include "GameCodeTypes.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include <GCBaseCharacter.h>

APlatformTrigger::APlatformTrigger()
{
	bReplicates = true;//выставляем флаг что эктор рпелецируется
	NetUpdateFrequency = 2.0f;//то с какой частотой пытается обновить свое состояние actor
	MinNetUpdateFrequency = 2.0f;//минимальная частота обновления св-в
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetCollisionProfileName(CollisionProfilePawnInteractionVolume);
	SetRootComponent(TriggerBox);
}

void APlatformTrigger::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APlatformTrigger, bIsActivated);
}

void APlatformTrigger::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APlatformTrigger::OnTriggerOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APlatformTrigger::OnTriggerOverlapEnd);
}

void APlatformTrigger::SetIsActivated(bool bIsActivated_In)
{
	if (OnTriggerActivated.IsBound())
	{
		OnTriggerActivated.Broadcast(bIsActivated_In);
	}
}

void APlatformTrigger::OnRep_IsActivated(bool bIsActivated_Old)
{
	//вызывается с св-вом которое уже отреплецировалось
	SetIsActivated(bIsActivated);
}

void APlatformTrigger::OnTriggerOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGCBaseCharacter* OtherPawn = Cast<AGCBaseCharacter>(OtherActor);
	if (!IsValid(OtherPawn))
	{
		return;
	}

	//ограничение, что логика будет работать только на сервере
	if (GetLocalRole() == ROLE_Authority)
	{
		//добавляем уникальные значения пешек
		OverlappedPawns.AddUnique(OtherPawn);

		if (!bIsActivated && OverlappedPawns.Num() > 0)
		{
			bIsActivated = true;
			SetIsActivated(true);
		}
	}
}

void APlatformTrigger::OnTriggerOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AGCBaseCharacter* OtherPawn = Cast<AGCBaseCharacter>(OtherActor);
	if (!IsValid(OtherPawn))
	{
		return;
	}

	if (GetLocalRole() == ROLE_Authority)
	{
		OverlappedPawns.RemoveSingleSwap(OtherPawn);

		if (bIsActivated && OverlappedPawns.Num() == 0)
		{
			bIsActivated = false;
			SetIsActivated(false);
		}
	}
}
