// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HighlightInteractable.generated.h"

/**
 * 
 */
class UTextBlock;
UCLASS()
class HOMEWORKPROJECT_API UHighlightInteractable : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetActionText(FName KeyName);

protected:
	//если он пытается найти виджет с таким же именем и с таким же типом
	//если он находит то он привязывается и мы можем с ним спокойно работать
	//UTextBlock — это один из базовых классов интерфейса пользователя(UI) в Unreal Engine, 
	// который используется для отображения текста на экране.
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ActionText;

};
