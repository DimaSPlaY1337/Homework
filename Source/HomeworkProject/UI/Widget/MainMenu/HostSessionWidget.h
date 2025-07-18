// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/MainMenu/NetworkWidget.h"
#include "HostSessionWidget.generated.h"

/**
 * 
 */
UCLASS()
class HOMEWORKPROJECT_API UHostSessionWidget : public UNetworkWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network session")
	FName ServerName;

	UFUNCTION(BlueprintCallable)
	void CreateSession();//создаёт сессию
};
