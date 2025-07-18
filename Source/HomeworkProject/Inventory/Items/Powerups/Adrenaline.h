// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Items/InventoryItem.h"
#include "Adrenaline.generated.h"

/**
 * 
 */
UCLASS()
class HOMEWORKPROJECT_API UAdrenaline : public UInventoryItem
{
	GENERATED_BODY()
	
public:
	virtual bool Consume(AGCBaseCharacter* ConsumeTarget) override;
};
