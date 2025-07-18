// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Items/InventoryItem.h"
#include "UI/Widget/Equipment/EquipmentSlotWidget.h"
#include "Utils/GCDataTableUtils.h"
#include "Actors/Equipment/EquipableItem.h"
#include "Inventory/Items/Equipables/WeaponInventoryItem.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/Widget/Inventory/InventorySlotWidget.h"

void UEquipmentSlotWidget::InitializeEquipmentSlot(TWeakObjectPtr<AEquipableItem> Equipment, int32 Index)
{
	if (!Equipment.IsValid())
		return;

	//говорим чем мы инициализируем
	LinkedEquipableItem = Equipment;
	SlotIndexInComponent = Index;

	//получаем информацию об оружии
	FWeaponTableRow* EquipmentData = GCDataTableUtils::FindWeaponData(Equipment->GetDataTableID());
	if (EquipmentData != nullptr)
	{
		//Адаптер - паттерн проектирования
		//AdapterLinkedInventoryItem - объект который позволяет работать с некоторым классом, с которым мы вообще не должны работать
		AdapterLinkedInventoryItem = NewObject<UWeaponInventoryItem>(Equipment->GetOwner());
		AdapterLinkedInventoryItem->Initialize(Equipment->GetDataTableID(), EquipmentData->WeaponItemDescription);
		AdapterLinkedInventoryItem->SetEquipmentClass(EquipmentData->EquipableActor);
	}
}

void UEquipmentSlotWidget::UpdateView()
{
	if (LinkedEquipableItem.IsValid())
	{
		//выставляем эконку с помощью SetBrushFromTexture
		ImageWeaponIcon->SetBrushFromTexture(AdapterLinkedInventoryItem->GetDescription().Icon);
		TBWeaponName->SetText(AdapterLinkedInventoryItem->GetDescription().Name);
	}
	else
	{
		//вычищаем эконку
		ImageWeaponIcon->SetBrushFromTexture(nullptr);
		TBWeaponName->SetText(FText::FromName(FName(NAME_None)));
	}
}

FReply UEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!LinkedEquipableItem.IsValid())
		return FReply::Handled();

	//UWidgetBlueprintLibrary::DetectDragIfPressed(): Это статический метод библиотеки UWidgetBlueprintLibrary,
	// который предназначен для проверки, было ли нажатие мыши использовано для начала перетаскивания элемента.
	//NativeReply необходимо для предоставления информации о результатах обработки события.
	FEventReply Reply = UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton);
	return Reply.NativeReply;
}

void UEquipmentSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	checkf(DragAndDropWidgetClass.Get() != nullptr, TEXT("UEquipmentSlotWidget::NativeOnDragDetected"));

	if (!AdapterLinkedInventoryItem.IsValid())
		return;

	//создаём драгендроп операцию
	UDragDropOperation* DragOperation = Cast<UDragDropOperation>(UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass()));

	//создаём новый UInventorySlotWidget виджет
	UInventorySlotWidget* DragWidget = CreateWidget<UInventorySlotWidget>(GetOwningPlayer(), DragAndDropWidgetClass);
	DragWidget->SetItemIcon(AdapterLinkedInventoryItem->GetDescription().Icon);//выставляем цвет иконки

	DragOperation->DefaultDragVisual = DragWidget;//визуал виджета
	DragOperation->Pivot = EDragPivot::CenterCenter;//
	DragOperation->Payload = AdapterLinkedInventoryItem.Get();
	OutOperation = DragOperation;
	//Payload представляет собой данные, которые передаются вместе с операцией перетаскивания
	// Здесь `DragOperation` представляет собой объект, который отвечает за операцию перетаскивания. 
	// `LinkedSlot->Item.Get()` используется для получения ссылки на объект, который вы хотите передать 
	// вместе с операцией перетаскивания, и назначается свойству `Payload` этой операции. Таким образом, 
	// когда пользователи начинают перетаскивание, `Payload` содержит информацию о том, что именно перетаскивается.

	LinkedEquipableItem.Reset();//очищаем equipable item
	OnEquipmentRemoveFromSlot.ExecuteIfBound(SlotIndexInComponent);

	UpdateView();//обновляем, чтобы иконка была пустой
}

bool UEquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UWeaponInventoryItem* OperationObject = Cast<UWeaponInventoryItem>(InOperation->Payload);
	if (IsValid(OperationObject))
		return OnEquipmentDropInSlot.Execute(OperationObject->GetEquipWeaponClass(), SlotIndexInComponent);
	return false;
}

void UEquipmentSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	//еще раз пробуем добавить элемент из экипировки в наш виджет
	AdapterLinkedInventoryItem = Cast<UWeaponInventoryItem>(InOperation->Payload);
	OnEquipmentDropInSlot.Execute(AdapterLinkedInventoryItem->GetEquipWeaponClass(), SlotIndexInComponent);
}
