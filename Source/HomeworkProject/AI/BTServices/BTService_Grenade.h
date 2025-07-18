// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_Grenade.generated.h"

/**
 * 
 */
UCLASS()
class HOMEWORKPROJECT_API UBTService_Grenade : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_Grenade();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
		FBlackboardKeySelector TargetKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
		float MaxThrowDistance = 800.0f;

private:
	bool Status = false;
};
