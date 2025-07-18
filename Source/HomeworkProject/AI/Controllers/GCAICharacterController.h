// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/Controllers/GCAIController.h"
#include "GCAICharacterController.generated.h"

/**
 * 
 */
class AGCAICharacter;
UCLASS()
class HOMEWORKPROJECT_API AGCAICharacterController : public AGCAIController
{
	GENERATED_BODY()
	
public:
	virtual void SetPawn(APawn* InPawn) override;

	virtual void ActorsPerceptionUpdated(const TArray<AActor*>& UpdatedActors) override;

	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
		float TargetReachRadius = 100.0f;

	virtual void BeginPlay() override;

	void SetupPatrolling();

private:
	void TryMoveToNextTarget();//перемещаемся к точке или главному персонажу

	bool IsTargetReached(FVector TargetLocation) const;

	TWeakObjectPtr<AGCAICharacter> CachedAICharacter;

	bool bIsPatrolling = false;
};
