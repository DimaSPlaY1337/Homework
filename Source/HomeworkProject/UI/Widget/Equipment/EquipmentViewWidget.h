// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EquipmentViewWidget.generated.h"

/**
 *
 */
class UEquipmentSlotWidget;
class UVerticalBox;
UCLASS()
class HOMEWORKPROJECT_API UEquipmentViewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//инициализация вью виджета
	void InitializeEquipmentWidget(UCharacterEquipmentComponent* EquipmentComponent);

protected:
	//добавление вьюхи
	void AddEquipmentSlotView(AEquipableItem* LinkToWeapon, int32 SlotIndex);
	//обновление слота
	void UpdateSlot(int32 SlotIndex);

	//экипировка элемента в слоте
	bool EquipEquipmentToSlot(const TSubclassOf<AEquipableItem>& WeaponClass, int32 SenderIndex);
	void RemoveEquipmentFromSlot(int32 SlotIndex);

	//внутри чего распологаем наш equipment виджет
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* VBWeaponSlots;

	//св-во для визуального представления
	UPROPERTY(EditDefaultsOnly, Category = "ItemContainer View Settings")
	TSubclassOf<UEquipmentSlotWidget> DefaultSlotViewClass;//св-во для визуального представления

	TWeakObjectPtr<UCharacterEquipmentComponent> LinkedEquipmentComponent;
};
