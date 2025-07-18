// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlotWidget.generated.h"

class UImage;
struct FInventorySlot;

UCLASS()
class HOMEWORKPROJECT_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeItemSlot(FInventorySlot& InventorySlot);//инициализирует слот данного виджета
	//связует InventorySlot с текущем виджетом
	void UpdateView();//позволяет обновить наше представление

	void SetItemIcon(UTexture2D* Icon);

protected:
	UPROPERTY(meta = (BindWidget))
	UImage* ImageItemIcon;//будет хранить иконку, которую создадим в blueprint

	//обработка событий
	//1. Обработка нажатия кнопки мышки. Будем потреблять айтем(аптечка и тд).
	//2. Начало операции переноса из одной ячейки в другую
	//3. На конец переноса, когда дропаем айтем
	//4. Отмена данного драгандропа
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	FInventorySlot* LinkedSlot;
};
