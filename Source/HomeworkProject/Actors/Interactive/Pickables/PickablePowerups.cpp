// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Interactive/Pickables/PickablePowerups.h"
#include "GameCodeTypes.h"
#include "Inventory/Items/InventoryItem.h"
#include "Utils/GCDataTableUtils.h"

APickablePowerups::APickablePowerups()
{
	PowerupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PowerupMesh"));
	SetRootComponent(PowerupMesh);
}

void APickablePowerups::Interact(AGCBaseCharacter* Character)
{
	FItemTableRow* ItemData = GCDataTableUtils::FindInventoryItemData(GetDataTableID());

	if (ItemData == nullptr)
		return;

	//создаём InventoryItem c помощью NewObject и храним указатель на объект с помощью TWeakObjectPtr
	//чтобы можно было удалить
	TWeakObjectPtr<UInventoryItem> Item = TWeakObjectPtr<UInventoryItem>(NewObject<UInventoryItem>(Character, ItemData->InventoryItemClass));
	Item->Initialize(DataTableID, ItemData->InventoryItemDescription);

	const bool bPickedUp = Character->PickupItem(Item);
	if (bPickedUp)
		Destroy();
}

FName APickablePowerups::GetActionEventName() const
{
	return ActionInteract;
}
