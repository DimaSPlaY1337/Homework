// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Items/InventoryItem.h"
#include "Medkit.generated.h"

/**
 * 
 */
UCLASS()
class HOMEWORKPROJECT_API UMedkit : public UInventoryItem
{
	GENERATED_BODY()
	
public:
	virtual bool Consume(AGCBaseCharacter* ConsumeTarget) override;//добавляем метод UInventoryItem, чтобы он работал 

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Medkit")
	float Health = 25.0f;
};
