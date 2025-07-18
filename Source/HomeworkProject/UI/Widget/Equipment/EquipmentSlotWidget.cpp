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

	//������� ��� �� ��������������
	LinkedEquipableItem = Equipment;
	SlotIndexInComponent = Index;

	//�������� ���������� �� ������
	FWeaponTableRow* EquipmentData = GCDataTableUtils::FindWeaponData(Equipment->GetDataTableID());
	if (EquipmentData != nullptr)
	{
		//������� - ������� ��������������
		//AdapterLinkedInventoryItem - ������ ������� ��������� �������� � ��������� �������, � ������� �� ������ �� ������ ��������
		AdapterLinkedInventoryItem = NewObject<UWeaponInventoryItem>(Equipment->GetOwner());
		AdapterLinkedInventoryItem->Initialize(Equipment->GetDataTableID(), EquipmentData->WeaponItemDescription);
		AdapterLinkedInventoryItem->SetEquipmentClass(EquipmentData->EquipableActor);
	}
}

void UEquipmentSlotWidget::UpdateView()
{
	if (LinkedEquipableItem.IsValid())
	{
		//���������� ������ � ������� SetBrushFromTexture
		ImageWeaponIcon->SetBrushFromTexture(AdapterLinkedInventoryItem->GetDescription().Icon);
		TBWeaponName->SetText(AdapterLinkedInventoryItem->GetDescription().Name);
	}
	else
	{
		//�������� ������
		ImageWeaponIcon->SetBrushFromTexture(nullptr);
		TBWeaponName->SetText(FText::FromName(FName(NAME_None)));
	}
}

FReply UEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!LinkedEquipableItem.IsValid())
		return FReply::Handled();

	//UWidgetBlueprintLibrary::DetectDragIfPressed(): ��� ����������� ����� ���������� UWidgetBlueprintLibrary,
	// ������� ������������ ��� ��������, ���� �� ������� ���� ������������ ��� ������ �������������� ��������.
	//NativeReply ���������� ��� �������������� ���������� � ����������� ��������� �������.
	FEventReply Reply = UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton);
	return Reply.NativeReply;
}

void UEquipmentSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	checkf(DragAndDropWidgetClass.Get() != nullptr, TEXT("UEquipmentSlotWidget::NativeOnDragDetected"));

	if (!AdapterLinkedInventoryItem.IsValid())
		return;

	//������ ���������� ��������
	UDragDropOperation* DragOperation = Cast<UDragDropOperation>(UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass()));

	//������ ����� UInventorySlotWidget ������
	UInventorySlotWidget* DragWidget = CreateWidget<UInventorySlotWidget>(GetOwningPlayer(), DragAndDropWidgetClass);
	DragWidget->SetItemIcon(AdapterLinkedInventoryItem->GetDescription().Icon);//���������� ���� ������

	DragOperation->DefaultDragVisual = DragWidget;//������ �������
	DragOperation->Pivot = EDragPivot::CenterCenter;//
	DragOperation->Payload = AdapterLinkedInventoryItem.Get();
	OutOperation = DragOperation;
	//Payload ������������ ����� ������, ������� ���������� ������ � ��������� ��������������
	// ����� `DragOperation` ������������ ����� ������, ������� �������� �� �������� ��������������. 
	// `LinkedSlot->Item.Get()` ������������ ��� ��������� ������ �� ������, ������� �� ������ �������� 
	// ������ � ��������� ��������������, � ����������� �������� `Payload` ���� ��������. ����� �������, 
	// ����� ������������ �������� ��������������, `Payload` �������� ���������� � ���, ��� ������ ���������������.

	LinkedEquipableItem.Reset();//������� equipable item
	OnEquipmentRemoveFromSlot.ExecuteIfBound(SlotIndexInComponent);

	UpdateView();//���������, ����� ������ ���� ������
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
	//��� ��� ������� �������� ������� �� ���������� � ��� ������
	AdapterLinkedInventoryItem = Cast<UWeaponInventoryItem>(InOperation->Payload);
	OnEquipmentDropInSlot.Execute(AdapterLinkedInventoryItem->GetEquipWeaponClass(), SlotIndexInComponent);
}
