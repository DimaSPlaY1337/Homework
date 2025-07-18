// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include <GCBaseCharacter.h>
#include "InventoryItem.generated.h"

class UInventoryItem;
class AEquipableItem;
class APickableItem;
class AGCBaseCharacter;

USTRUCT(BlueprintType)
struct FInventoryItemDescription : public FTableRowBase//благодаря этому сможем создавать дата таблицы(таблица поможет избежать проблему с цикличными ссылками)
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	UTexture2D* Icon;
};

//через эту таблицу мы будем находить соответствие между поднимаемым эктором и эктором экипировки.
USTRUCT(BlueprintType)
struct FWeaponTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon view")
	TSubclassOf<APickableItem> PickableActor;//поднимаемый

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon view")
	TSubclassOf<AEquipableItem> EquipableActor;//эктор экипировки

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon view")
	FInventoryItemDescription WeaponItemDescription;//нужен для работы с item-ом в инвенторе
};

USTRUCT(BlueprintType)
struct FItemTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	TSubclassOf<APickableItem> PickableActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	TSubclassOf<UInventoryItem> InventoryItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	FInventoryItemDescription InventoryItemDescription;

};

UCLASS(Blueprintable)
class HOMEWORKPROJECT_API UInventoryItem : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(FName DataTableID_In, const FInventoryItemDescription& Description_In);

	FName GetDataTableID() const;
	const FInventoryItemDescription& GetDescription() const;

	virtual bool IsEquipable() const;
	virtual bool IsConsumable() const;

	//метод позволяющий употребить айтем
	virtual bool Consume(AGCBaseCharacter* ConsumeTarget) PURE_VIRTUAL(UInventoryItem::Consume, return false;);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory item")
	FName DataTableID = NAME_None;//ID для поиска таблицей

	UPROPERTY(EditDefaultsOnly, Category = "Inventory item")
	FInventoryItemDescription Description;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory item")
	bool bIsEquipable = false;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory item")
	bool bIsConsumable = false;//потребляемым(аптечки)

private:
	bool bIsInitialized = false;
};
