// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponWheelWidget.generated.h"

struct FWeaponTableRow;
class UImage;
class UTextBlock;
class UCharacterEquipmentComponent;

UCLASS()
class HOMEWORKPROJECT_API UWeaponWheelWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitializeWeaponWheelWidget(UCharacterEquipmentComponent* EquipmentComponent);

	void NextSegment();
	void PreviousSegment();
	void ConfirmSelection();

protected:
	virtual void NativeConstruct() override;
	void SelectSegment();//подсвечивает элементы

	UPROPERTY(meta = (BindWidget))
	UImage* RadialBackground;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponNameText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon wheel settings")
	TArray<EEquipmentSlots> EquipmentSlotsSegments;

private:
	//позволит найти описание EquipableItem для конкретного сегмента
	FWeaponTableRow* GetTableRowForSegments(int32 SegmentIndex) const;

	int32 CurrentSegmentIndex;
	UMaterialInstanceDynamic* BackgroundMaterial;
	TWeakObjectPtr<UCharacterEquipmentComponent> LinkedEquipmentComponent;

};
