// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/Widget/Inventory/InventoryViewWidget.h"
#include "CharacterInventoryComponent.generated.h"

class UInventoryItem;
class UInventoryViewWidget;
//контейнер в котором будем хранить описание нашего айтема
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE(FInventorySlotUpdate);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<UInventoryItem> Item;//указатель на айтем, который содержит данный контейнер

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 0;//сколько элементов есть в данном контейнере 

	void BindOnInventorySlotUpdate(const FInventorySlotUpdate& Callback) const;
	void UnbindOnInventorySlotUpdate();
	void UpdateSlotState();
	void ClearSlot();

private:
	//mutable позволяет изменять значение член-данных класса, даже если сам объект класса является константным.
	mutable FInventorySlotUpdate OnInventorySlotUpdate;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOMEWORKPROJECT_API UCharacterInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	void OpenViewInventory(APlayerController* PlayerController);//открывает view нашего инвентрая
	void CloseViewInventory();//закрывает view нашего инвентрая
	bool IsViewVisible() const;//видим или не видим

	int32 GetCapacity() const;//ёмкость нашего инвентаря
	bool HasFreeSlot() const;//есть ли свободный слот

	bool AddItem(TWeakObjectPtr<UInventoryItem> ItemToAdd, int32 Count);
	bool RemoveItem(FName ItemID);

	TArray<FInventorySlot> GetAllItemsCopy() const;//копирует все элементы текущего инвентаря
	TArray<FText> GetAllItemsNames() const;

protected:
	UPROPERTY(EditAnywhere, Category = "Items")
	TArray<FInventorySlot> InventorySlots;//массив слотов инвентаря

	UPROPERTY(EditAnywhere, Category = "View settings")
	TSubclassOf<UInventoryViewWidget> InventoryViewWidgetClass;//виджет который будет открываться

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory settings", meta = (ClampMin = 1, UIMin = 1))
	int32 Capacity = 16;//ёмкость нашего контейнера(кол-во слотов)

	virtual void BeginPlay() override;
	
	void CreateViewWidget(APlayerController* PlayerController);

	FInventorySlot* FindItemSlot(FName ItemID);//находит слот с подходящим элементом инвенторя
	FInventorySlot* FindFreeSlot();//находит подходящйи свободный слот

private:
	UPROPERTY()
	UInventoryViewWidget* InventoryViewWidget;

	int32 ItemsInInventory;//текущее кол-во элементов у нас в инвенторе
};
