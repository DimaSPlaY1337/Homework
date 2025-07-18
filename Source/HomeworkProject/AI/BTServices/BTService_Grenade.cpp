// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTServices/BTService_Grenade.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GCBaseCharacter.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "AIController.h"
#include "GameCodeTypes.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"

UBTService_Grenade::UBTService_Grenade()
{
	NodeName = "Grenade";
}

void UBTService_Grenade::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!IsValid(AIController) || !IsValid(Blackboard))
	{
		return;
	}

	AGCBaseCharacter* Character = Cast<AGCBaseCharacter>(AIController->GetPawn());
	if (!IsValid(Character))
	{
		return;
	}

	AActor* CurrentTraget = Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!IsValid(CurrentTraget))
	{
		return;
	}

	float DistSq = FVector::DistSquared(CurrentTraget->GetActorLocation(), Character->GetActorLocation());
	if (DistSq > FMath::Square(MaxThrowDistance))
	{
		return;
	}

	Character->EquipPrimaryItem();
}
