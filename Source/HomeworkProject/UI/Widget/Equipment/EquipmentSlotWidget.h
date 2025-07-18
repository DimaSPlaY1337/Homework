// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EquipmentSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UInventorySlotWidget;
class AEquipableItem;
class UWeaponInventoryItem;
UCLASS()
class HOMEWORKPROJECT_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	//DECLARE_DELEGATE_RetVal_TwoParams - делегат возвращает значение
	//1 - возвращаемое значение, 2 - имя делегата, 3 4 - параметры. int32 - слот куда мы перетащили
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnEquipmentDropInSlot, const TSubclassOf<AEquipableItem>&, int32);
	DECLARE_DELEGATE_OneParam(FOnEquipmentRemoveFromSlot, int32);//делегат удаления из какого то слота

	FOnEquipmentDropInSlot OnEquipmentDropInSlot;
	FOnEquipmentRemoveFromSlot OnEquipmentRemoveFromSlot;

	//инициализация эквипмент слота
	void InitializeEquipmentSlot(TWeakObjectPtr<AEquipableItem> Equipment, int32 Index);
	void UpdateView();
protected:
	UPROPERTY(meta = (BindWidget))//используется для задания эконки
	UImage* ImageWeaponIcon;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TBWeaponName;

	//нужно для дрэн энд дроп операции
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UInventorySlotWidget> DragAndDropWidgetClass;

	//обработка событий
	//1. Обработка нажатия кнопки мышки. Будем потреблять айтем(аптечка и тд).
	//2. Начало операции переноса из одной ячейки в другую
	//3. На конец переноса, когда дропаем айтем
	//4. Отмена данного драгандропа(бросили в неположеном месте или фалз от NativeOnDrop)
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	TWeakObjectPtr<AEquipableItem> LinkedEquipableItem;
	TWeakObjectPtr<UWeaponInventoryItem> AdapterLinkedInventoryItem;

	int32 SlotIndexInComponent = 0;
};
