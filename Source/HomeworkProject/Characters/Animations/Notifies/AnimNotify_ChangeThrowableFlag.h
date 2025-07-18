// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ChangeThrowableFlag.generated.h"

/**
 * 
 */
UCLASS()
class HOMEWORKPROJECT_API UAnimNotify_ChangeThrowableFlag : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

private:
	bool flag;
};
