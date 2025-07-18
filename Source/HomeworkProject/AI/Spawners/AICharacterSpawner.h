// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/Characters/GCAICharacter.h"
#include "AICharacterSpawner.generated.h"

class IInteractable;
UCLASS()
class HOMEWORKPROJECT_API AAICharacterSpawner : public AActor
{
	GENERATED_BODY()
	
public:
	AAICharacterSpawner();

	UFUNCTION()
	void SpawnAI();//������� AI

protected:	
	//���������� ����� ������� ��������� ��-�� �������
	//��������� ������������ ���������� ������ �������������
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangeEvent) override;

	virtual void BeginPlay() override;
	//��������� ������� �� ������� ��� �����������
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReson) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Spawner")
	TSubclassOf<AGCAICharacter> CharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Spawner")
	bool bIsSpawnOnStart;//����� ������� � ����� ����

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Spawner")
	bool bDoOnce = false;//���� ��� ���������� �������� � ����������. true - ������� ������������ 1 ���

	// �����, ����������� ��������� IInteractable
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Spawner")
	AActor* SpawnTriggerActor;

private:
	bool bCanSpawn = true;

	void UnSubscribeFromTrigger();

	UPROPERTY()
	TScriptInterface<IInteractable> SpawnTrigger;//������ SpawnTriggerActor � ���� ����������

	FDelegateHandle TriggerHandle;
};
