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
	//���� �� �������� ����� ������ � ����� �� ������ � � ����� �� �����
	//���� �� ������� �� �� ������������� � �� ����� � ��� �������� ��������
	//UTextBlock � ��� ���� �� ������� ������� ���������� ������������(UI) � Unreal Engine, 
	// ������� ������������ ��� ����������� ������ �� ������.
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ActionText;

};
