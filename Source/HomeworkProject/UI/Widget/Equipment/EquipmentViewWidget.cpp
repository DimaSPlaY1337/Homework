// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/Equipment/EquipmentViewWidget.h"
#include "Actors/Equipment/EquipableItem.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "EquipmentSlotWidget.h"
#include "Components/VerticalBox.h"

void UEquipmentViewWidget::InitializeEquipmentWidget(UCharacterEquipmentComponent* EquipmentComponent)
{
	LinkedEquipmentComponent = EquipmentComponent;
	const TArray<AEquipableItem*>& Items = LinkedEquipmentComponent->GetItems();
	//����������� �� ������. ���������� ������ ����
	for (int32 Index = 1; Index < Items.Num(); ++Index)//++Index - ���������� 1 ����
		AddEquipmentSlotView(Items[Index], Index);
}

void UEquipmentViewWidget::AddEquipmentSlotView(AEquipableItem* LinkToWeapon, int32 SlotIndex)
{
	checkf(IsValid(DefaultSlotViewClass.Get()), TEXT("UEquipmentViewWidget::AddEquipmentSlotView"));

	UEquipmentSlotWidget* SlotWidget = CreateWidget<UEquipmentSlotWidget>(this, DefaultSlotViewClass);

	if (SlotWidget != nullptr)
	{
		SlotWidget->InitializeEquipmentSlot(LinkToWeapon, SlotIndex);

		VBWeaponSlots->AddChildToVerticalBox(SlotWidget);
		//SlotWidget->UpdateView(); ��������!!!!!! ���� �������� �����. ������ ����� ������� � ��������
		SlotWidget->OnEquipmentDropInSlot.BindUObject(this, &UEquipmentViewWidget::EquipEquipmentToSlot);
		SlotWidget->OnEquipmentRemoveFromSlot.BindUObject(this, &UEquipmentViewWidget::RemoveEquipmentFromSlot);
	}
}

void UEquipmentViewWidget::UpdateSlot(int32 SlotIndex)
{
	//������� ���� �����������, ������� ����� ����� �� 1 ������
	UEquipmentSlotWidget* WidgetToUpdate = Cast<UEquipmentSlotWidget>(VBWeaponSlots->GetChildAt(SlotIndex - 1));
	if (IsValid(WidgetToUpdate))
	{
		WidgetToUpdate->InitializeEquipmentSlot(LinkedEquipmentComponent->GetItems()[SlotIndex], SlotIndex);
		WidgetToUpdate->UpdateView();
	}
}

bool UEquipmentViewWidget::EquipEquipmentToSlot(const TSubclassOf<AEquipableItem>& WeaponClass, int32 SenderIndex)
{
	const bool Result = LinkedEquipmentComponent->AddEquipmentItemToSlot(WeaponClass, SenderIndex);
	if (Result)
		UpdateSlot(SenderIndex);
	return Result;
}

void UEquipmentViewWidget::RemoveEquipmentFromSlot(int32 SlotIndex)
{
	LinkedEquipmentComponent->RemoveItemFromSlot(SlotIndex);
}
