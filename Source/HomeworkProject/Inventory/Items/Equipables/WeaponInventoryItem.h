// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Items/InventoryItem.h"
#include "WeaponInventoryItem.generated.h"

/**
 * 
 */
UCLASS()
class HOMEWORKPROJECT_API UWeaponInventoryItem : public UInventoryItem
{
	GENERATED_BODY()
	
public:
	UWeaponInventoryItem();

	void SetEquipmentClass(TSubclassOf<AEquipableItem>& WeaponClass);
	TSubclassOf<AEquipableItem> GetEquipWeaponClass() const;

protected:
	//класс шаблон позволяющий выбирать нужный класс
	TSubclassOf<AEquipableItem> EquipWeaponClass;

};
