// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Animations/Notifies/AnimNotify_ChangeThrowableFlag.h"

void UAnimNotify_ChangeThrowableFlag::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	AGCBaseCharacter* CharacterOwner = Cast<AGCBaseCharacter>(MeshComp->GetOwner());
	if (!IsValid(CharacterOwner))
	{
		return;
	}
	flag = CharacterOwner->GetCharacterEquipmentComponent_Mutable()->GetThrowingStatus();
	if (!CharacterOwner->IsPlayerControlled())
	{
		if (flag == false)
			CharacterOwner->GetCharacterEquipmentComponent_Mutable()->ChangeThrowingStatus(true);
		else
			CharacterOwner->GetCharacterEquipmentComponent_Mutable()->ChangeThrowingStatus(false);
	}
}
