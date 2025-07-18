// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/Equipment/WeaponWheelWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Inventory/Items/InventoryItem.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Actors/Equipment/EquipableItem.h"
#include "Utils/GCDataTableUtils.h"
#include "GameCodeTypes.h"
#include "Blueprint/WidgetTree.h"

void UWeaponWheelWidget::InitializeWeaponWheelWidget(UCharacterEquipmentComponent* EquipmentComponent)
{
	LinkedEquipmentComponent = EquipmentComponent;
}

void UWeaponWheelWidget::NextSegment()
{
	CurrentSegmentIndex++;
	if (CurrentSegmentIndex == EquipmentSlotsSegments.Num())
	{
		CurrentSegmentIndex = 0;
	}
	SelectSegment();
}

void UWeaponWheelWidget::PreviousSegment()
{
	CurrentSegmentIndex--;
	if (CurrentSegmentIndex < 0)
	{
		CurrentSegmentIndex = EquipmentSlotsSegments.Num() - 1;
	}
	SelectSegment();
}

void UWeaponWheelWidget::ConfirmSelection()
{
	EEquipmentSlots SeletedSlot = EquipmentSlotsSegments[CurrentSegmentIndex];
	LinkedEquipmentComponent->EquipItemInSlot(SeletedSlot);
	RemoveFromParent();
}

void UWeaponWheelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsValid(RadialBackground) && !IsValid(BackgroundMaterial))
	{
		BackgroundMaterial = RadialBackground->GetDynamicMaterial();
		BackgroundMaterial->SetScalarParameterValue(FName("Segments"), EquipmentSlotsSegments.Num());
	}

	for (int i = 0; i < EquipmentSlotsSegments.Num(); ++i)
	{
		FName WidgetName = FName(FString::Printf(TEXT("ImageSegment%i"), i));
		UImage* WeaponImage = WidgetTree->FindWidget<UImage>(WidgetName);
		if (!IsValid(WeaponImage))
			continue;//пропускает выполнение оставшегося кода в текущей итерации и переходит сразу к следующей итерации цикла.

		FWeaponTableRow* WeaponData = GetTableRowForSegments(i);
		if (WeaponData == nullptr)
		{
			WeaponImage->SetOpacity(0.0f);//прозрачность
		}
		else
		{
			WeaponImage->SetOpacity(1.0f);
			WeaponImage->SetBrushFromTexture(WeaponData->WeaponItemDescription.Icon);
		}
	}
}

void UWeaponWheelWidget::SelectSegment()
{
	BackgroundMaterial->SetScalarParameterValue(FName("Index"), CurrentSegmentIndex);
	FWeaponTableRow* WeaponData = GetTableRowForSegments(CurrentSegmentIndex);
	if (WeaponData == nullptr)
	{
		WeaponNameText->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		WeaponNameText->SetVisibility(ESlateVisibility::Visible);
		WeaponNameText->SetText(WeaponData->WeaponItemDescription.Name);
	}
}

FWeaponTableRow* UWeaponWheelWidget::GetTableRowForSegments(int32 SegmentIndex) const
{
	const EEquipmentSlots& SegmentSlot = EquipmentSlotsSegments[SegmentIndex];
	AEquipableItem* EquipableItem = LinkedEquipmentComponent->GetItems()[(int32)SegmentIndex];
	if (!IsValid(EquipableItem))
	{
		return nullptr;
	}
	return GCDataTableUtils::FindWeaponData(EquipableItem->GetDataTableID());
}
