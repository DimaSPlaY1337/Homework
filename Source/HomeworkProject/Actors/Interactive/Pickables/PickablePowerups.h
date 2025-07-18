// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Interactive/Pickables/PickableItem.h"
#include "GCBaseCharacter.h"
#include "PickablePowerups.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class HOMEWORKPROJECT_API APickablePowerups : public APickableItem
{
	GENERATED_BODY()
	
public:
	APickablePowerups();
	
	virtual void Interact(AGCBaseCharacter* Character) override;
	virtual FName GetActionEventName() const override;

protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* PowerupMesh;//мешка поверапа
};
