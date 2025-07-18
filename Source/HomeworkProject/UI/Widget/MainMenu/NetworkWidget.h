// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NetworkWidget.generated.h"

//динамический потому что хотим подписаться их blueprint
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNetworkWidgetClosed);

UCLASS()
class HOMEWORKPROJECT_API UNetworkWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnNetworkWidgetClosed OnNetworkWidgetClosed;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network session")
	bool bIsLAN;

	UFUNCTION(BlueprintPure)
	FText GetNetworkType() const;//получает текущий тип сети

	UFUNCTION(BlueprintCallable)
	void ToggleNetworkType();//отвечает за переключение network мода

	UFUNCTION(BlueprintCallable)
	virtual void CloseWidget();//обрабатывает закрытие виджета(снимаем видимость виджета)

};
