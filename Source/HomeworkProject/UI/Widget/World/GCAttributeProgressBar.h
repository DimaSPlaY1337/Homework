// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCAttributeProgressBar.generated.h"

class UProgressBar;
UCLASS()
class HOMEWORKPROJECT_API UGCAttributeProgressBar : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetProgressPercantage(float Percentage);//будет работать только при нанесении урона

protected:
	//meta = (BindWidget) - спецификатор позволяет нам из c++ привязаться к виджетам
	//которые мы создадим внутри блупринтого виджета, без того чтобы искать перебором.
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthProgressBar;
};
