// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Interactive/Pickables/PickableItem.h"
#include "GCBaseCharacter.h"
#include "PickableWeapon.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class HOMEWORKPROJECT_API APickableWeapon : public APickableItem
{
	GENERATED_BODY()
	
public:
	APickableWeapon();
	//методы интерфейса
	virtual void Interact(AGCBaseCharacter* Character) override;
	virtual FName GetActionEventName() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* WeaponMesh;
	//используем UStaticMeshComponent вместо USkeletalMesh, потому что если у нас
	//скелетные мешки будут валяться по всему уровню, мы конечно можем выключить у них тик
	//, но это будет оверхэт. Благо ue позволяет из скелетной мешки очень легко создать
	//статическую мешку.
};
