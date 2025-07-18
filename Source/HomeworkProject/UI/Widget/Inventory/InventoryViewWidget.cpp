// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/Inventory/InventoryViewWidget.h"
#include "Components/CharacterComponents/CharacterInventoryComponent.h"
#include "InventorySlotWidget.h"
#include "Components/GridPanel.h"

void UInventoryViewWidget::InitializeViewWidget(TArray<FInventorySlot>& InventorySlots)
{
	for (FInventorySlot& Item : InventorySlots)
	{
		AddItemSlotView(Item);
	}
}

void UInventoryViewWidget::AddItemSlotView(FInventorySlot& SlotToAdd)
{
	checkf(InventorySlotWidgetClass.Get() != nullptr, TEXT("UItemContainerWidget::AddItemSlotView widget class"));

	UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(this, InventorySlotWidgetClass);

	if (SlotWidget != nullptr)
	{
		SlotWidget->InitializeItemSlot(SlotToAdd);

		const int32 CurrentSlotCount = GridPanelItemSlots->GetChildrenCount();//берем кол-во элементов, которое есть в Grid панели
		const int32 CurrentSlotRow = CurrentSlotCount / ColumnCount;//получем текущий ряд слота
		const int32 CurrentSlotColumn = CurrentSlotCount % ColumnCount;//получаем текущую колонку нашего слота
		//вызов GridPanelItemSlots->AddChildToGrid(SlotWidget, CurrentSlotRow, CurrentSlotColumn); 
		// добавляет виджет SlotWidget в указанное место (строка и колонка) в сетевой панели GridPanelItemSlots. 
		// После этого данный виджет будет отображаться в графическом интерфейсе в соответствующем месте сетки.
		GridPanelItemSlots->AddChildToGrid(SlotWidget, CurrentSlotRow, CurrentSlotColumn);

		SlotWidget->UpdateView();
	}
}
