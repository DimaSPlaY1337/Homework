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
	void SpawnAI();//спавним AI

protected:	
	//вызывается после каждого изменения св-ва объекта
	//позволяет обрабатывать невалидные выборы пользователей
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangeEvent) override;

	virtual void BeginPlay() override;
	//принимает причину по которой все закончилось
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReson) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Spawner")
	TSubclassOf<AGCAICharacter> CharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Spawner")
	bool bIsSpawnOnStart;//можем спавнит в бегин плее

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Spawner")
	bool bDoOnce = false;//флаг для связывания спавнера с тригеррами. true - спавнер активируется 1 раз

	// Актор, реализующий интерфейс IInteractable
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Spawner")
	AActor* SpawnTriggerActor;

private:
	bool bCanSpawn = true;

	void UnSubscribeFromTrigger();

	UPROPERTY()
	TScriptInterface<IInteractable> SpawnTrigger;//хранит SpawnTriggerActor в виде интерфейса

	FDelegateHandle TriggerHandle;
};
