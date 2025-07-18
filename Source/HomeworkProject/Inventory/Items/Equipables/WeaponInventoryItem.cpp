// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Items/Equipables/WeaponInventoryItem.h"

UWeaponInventoryItem::UWeaponInventoryItem()
{
	bIsConsumable = true;
}

void UWeaponInventoryItem::SetEquipmentClass(TSubclassOf<AEquipableItem>& WeaponClass)
{
	EquipWeaponClass = WeaponClass;
}

TSubclassOf<AEquipableItem> UWeaponInventoryItem::GetEquipWeaponClass() const
{
	return EquipWeaponClass;
}
