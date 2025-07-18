// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/PlayerHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "GameCodeTypes.h"
#include "CharacterAttributesComponent.h"
#include "ReticleWidget.h"
#include "AmmoWidget.h"
#include "GCBaseCharacter.h"

UReticleWidget* UPlayerHUDWidget::GetReticleWidget()
{
	//позволяет дизайнерам подтянуть этот виджет
	return WidgetTree->FindWidget<UReticleWidget>(ReticleWidgetName);
}

UAmmoWidget* UPlayerHUDWidget::GetAmmoWidget()
{
	return WidgetTree->FindWidget<UAmmoWidget>(AmmoWidgetName);
}

UWidgetCharacterAttributes* UPlayerHUDWidget::GetWidgetCharacterAttributes(EAttributesType Type)
{
	switch (Type)
	{
	case EAttributesType::Health:
	{
		return WidgetTree->FindWidget<UWidgetCharacterAttributes>(HealthWidgetName);
	}
	case EAttributesType::Oxygen:
	{
		return WidgetTree->FindWidget<UWidgetCharacterAttributes>(OxygenWidgetName);
	}
	case EAttributesType::Stamina:
	{
		return WidgetTree->FindWidget<UWidgetCharacterAttributes>(StaminaWidgetName);
	}
	default:
		return nullptr;
	}
}

void UPlayerHUDWidget::SetHighlightInteractableVisibility(bool bIsVisible)
{
	if (!IsValid(InteractableKey))
		return;

	if (bIsVisible)
		InteractableKey->SetVisibility(ESlateVisibility::Visible);
	else
		InteractableKey->SetVisibility(ESlateVisibility::Hidden);
}

void UPlayerHUDWidget::SetHighlightInteractableActionText(FName KeyName)
{
	if (IsValid(InteractableKey))
	{
		//Метод SetActionText() обычно используется в игровом или интерактивном приложении для обновления текста, 
		// отображающегося на экране, который описывает действие, доступное для пользователя. 
		InteractableKey->SetActionText(KeyName);
	}
}
