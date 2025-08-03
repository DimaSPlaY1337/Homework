// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/Streaming/StreamingSubsystemUtils.h"
#include "GameFramework/Character.h"
#include "Subsystems/Streaming/StreamingSubsytemVolume.h"
#include "Subsystems/Streaming/SystemSubsystemManager.h"
//CheckCharacterOverlapStreamingSubsystemVolume Ч смотрит, 
// в какие объЄмы зашЄл персонаж и сообщает этим объЄмам об этом.
void UStreamingSubsystemUtils::CheckCharacterOverlapStreamingSubsystemVolume(ACharacter* Character)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingSubsystemUtils::CheckCharacterOverlapStreamingSubsystemVolume(): %s"), *GetNameSafe(Character));
	if (!IsValid(Character))
	{
		return;
	}

	TSet<AActor*> OverlappingActors;
	Character->GetOverlappingActors(OverlappingActors, AStreamingSubsytemVolume::StaticClass());
	for (AActor* OverlappedActor : OverlappingActors)
	{
		if (!IsValid(OverlappedActor) || !OverlappedActor->HasActorBegunPlay())
		{
			continue;
		}

		StaticCast<AStreamingSubsytemVolume*>(OverlappedActor)->HandleCharacterOverlapBegin(Character);
	}
}
//CheckStreamingSubsystemVolumeOverlapCharacter Ч смотрит, 
// какие персонажи вход€т в заданный объЄм, и сообщает им об этом.
void UStreamingSubsystemUtils::CheckStreamingSubsystemVolumeOverlapCharacter(AStreamingSubsytemVolume* SubsystemVolume)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingSubsystemUtils::CheckStreamingSubsystemVolumeOverlapCharacter(): %s"), *GetNameSafe(SubsystemVolume));
	if (!IsValid(SubsystemVolume))
	{
		return;
	}

	TSet<AActor*> OverlappingActors;
	SubsystemVolume->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());
	for (AActor* OverlappedActor : OverlappingActors)
	{
		if (!IsValid(OverlappedActor) || !OverlappedActor->HasActorBegunPlay())
		{
			continue;
		}

		SubsystemVolume->HandleCharacterOverlapBegin(StaticCast<ACharacter*>(OverlappedActor));
	}
}