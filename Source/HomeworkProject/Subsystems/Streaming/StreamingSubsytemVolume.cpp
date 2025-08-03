// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/Streaming/StreamingSubsytemVolume.h"
#include "Subsystems/Streaming/SystemSubsystemManager.h"
#include "Components/BoxComponent.h"
#include "StreamingSubsystemUtils.h"
#include "StreamingSubsystem.h"
#include "GameFramework/Character.h"


AStreamingSubsytemVolume::AStreamingSubsytemVolume()
{
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
}

const TSet<FString>& AStreamingSubsytemVolume::GetLevelsToLoad() const
{
	return LevelsToLoad;
}

const TSet<FString>& AStreamingSubsytemVolume::GetLevelsToUnload() const
{
	return LevelsToUnload;
}

void AStreamingSubsytemVolume::HandleCharacterOverlapBegin(ACharacter* Character)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("AStreamingSubsytemVolume::HandleCharacterOverlapBegin(): %s, Character: %s"), *GetNameSafe(this), *GetNameSafe(Character));
	if (!StreamingSubsystem.IsValid() || !StreamingSubsystem->CanUseSubsystem())
	{
		return;
	}

	if (!IsValid(Character) || !Character->IsPlayerControlled())
	{
		return;
	}

	if (OverlappedCharacter.IsValid())
	{
		return;
	}

	OverlappedCharacter = Character;
	StreamingSubsystem->OnVolumeOverlapBegin(this);
}

void AStreamingSubsytemVolume::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogStreamingSubsystem, Display, TEXT("AStreamingSubsytemVolume::BeginPlay(): %s"), *GetNameSafe(this));

	StreamingSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UStreamingSubsystem>();

	CollisionComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &AStreamingSubsytemVolume::OnOverlapBegin);
	CollisionComponent->OnComponentEndOverlap.AddUniqueDynamic(this, &AStreamingSubsytemVolume::OnOverlapEnd);
	CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

	//
	UStreamingSubsystemUtils::CheckStreamingSubsystemVolumeOverlapCharacter(this);
}

void AStreamingSubsytemVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("AStreamingSubsytemVolume::BeginPlay(): %s"), *GetNameSafe(this));

	CollisionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &AStreamingSubsytemVolume::OnOverlapBegin);
	CollisionComponent->OnComponentEndOverlap.RemoveDynamic(this, &AStreamingSubsytemVolume::OnOverlapEnd);
	CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	if (OverlappedCharacter.IsValid())//игра завершилась но персонаж все равно пересекается
	{
		//Reset() обычно реализован так, 
		// чтобы объект возвращался в начальное состояние, будто он только что был создан 
		//либо чтобы очистить все временные данные и подготовить объект для 
		// повторного использования
		OverlappedCharacter.Reset();

		StreamingSubsystem->OnVolumeOverlapEnd(this);
	}

	StreamingSubsystem.Reset();

	Super::EndPlay(EndPlayReason);
}

void AStreamingSubsytemVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("AStreamingSubsytemVolume::OnOverlapBegin(): %s, OtherActor: %s"), *GetNameSafe(this), *GetNameSafe(OtherActor));
	if (!IsValid(OtherActor))
	{
		return;
	}

	HandleCharacterOverlapBegin(Cast<ACharacter>(OtherActor));
}

void AStreamingSubsytemVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("AStreamingSubsytemVolume::OnOverlapEnd(): %s, OtherActor: %s"), *GetNameSafe(this), *GetNameSafe(OtherActor));
	if (!StreamingSubsystem.IsValid() || !StreamingSubsystem->CanUseSubsystem())
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!IsValid(Character) || !Character->IsPlayerControlled())
	{
		return;
	}

	if (OverlappedCharacter != OtherActor)
	{
		return;
	}

	OverlappedCharacter.Reset();
	StreamingSubsystem->OnVolumeOverlapEnd(this);
}