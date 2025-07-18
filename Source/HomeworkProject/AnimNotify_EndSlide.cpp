// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_EndSlide.h"
#include "GCBaseCharacter.h"
#include "GCBaseCharacterMovementComponent.h"

void UAnimNotify_EndSlide::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (MeshComp->GetOwner()->GetLocalRole() == ROLE_Authority) {

		AGCBaseCharacter* CharacterOwner = Cast<AGCBaseCharacter>(MeshComp->GetOwner());

		if (IsValid(CharacterOwner))
		{
			CharacterOwner->GetBaseCharacterMovementComponent()->StopSlide();
		}
	}
}
