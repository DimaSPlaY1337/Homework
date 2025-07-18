// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Spawners/AICharacterSpawner.h"
#include "AI/Characters/GCAICharacter.h"
#include "Actors/Interactive/Interface/Interactive.h"
#include "UObject/ScriptInterface.h"

// Sets default values
AAICharacterSpawner::AAICharacterSpawner()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnerRoot"));
	SetRootComponent(SceneRoot);
}

void AAICharacterSpawner::SpawnAI()
{
	if (!bCanSpawn || !IsValid(CharacterClass))
		return;

	AGCAICharacter* AICharater = GetWorld()->SpawnActor<AGCAICharacter>(CharacterClass, GetTransform());
	//из-за того что мы спавним эктора динамически контроллер у AI не появиться, нужно вручную сделать его
	//когда мы спавним на карте AI там автоматически появляется контроллер
	if (!IsValid(AICharater->Controller))
		AICharater->SpawnDefaultController();

	if (bDoOnce)
	{
		UnSubscribeFromTrigger();
		bCanSpawn = false;
	}

}


void AAICharacterSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangeEvent)
{
	Super::PostEditChangeProperty(PropertyChangeEvent);
	//проверяем что мы меняем имя подходящего property
	if (PropertyChangeEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(AAICharacterSpawner, SpawnTriggerActor))
	{
		SpawnTrigger = SpawnTriggerActor;
		if (SpawnTrigger.GetInterface())
		{
			if (!SpawnTrigger->HasOnInteractionCallback())
			{
				SpawnTriggerActor = nullptr;
				SpawnTrigger = nullptr;
			}
		}
		else
		{
			SpawnTriggerActor = nullptr;
			SpawnTrigger = nullptr;
		}
	}
}

void AAICharacterSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (SpawnTrigger.GetInterface())
		TriggerHandle = SpawnTrigger->AddOnInteractionUFunction(this, FName("SpawnAI"));

	if (bIsSpawnOnStart)
		SpawnAI();
}

void AAICharacterSpawner::EndPlay(const EEndPlayReason::Type EndPlayReson)
{
	UnSubscribeFromTrigger();
	Super::EndPlay(EndPlayReson);
}

void AAICharacterSpawner::UnSubscribeFromTrigger()
{
	if (SpawnTrigger.GetInterface() && TriggerHandle.IsValid())
		SpawnTrigger->RemoveOnInteractionDelegate(TriggerHandle);
}

