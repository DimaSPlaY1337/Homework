// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Actors/Interactive/Interface/Interactive.h"
#include "Components/TimelineComponent.h"
#include "Subsystems/SaveSubsystem/SaveSubsystemInterface.h"
#include "Door.generated.h"

UCLASS()
class HOMEWORKPROJECT_API ADoor : public AActor, public IInteractable, public ISaveSubsystemInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADoor();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void Interact(AGCBaseCharacter* Character) override;

	virtual FName GetActionEventName() const override;

	virtual bool HasOnInteractionCallback() const;

	virtual FDelegateHandle AddOnInteractionUFunction(UObject* Object, const FName& FunctionName) override;
	virtual void RemoveOnInteractionDelegate(FDelegateHandle DelegateHandle) override;

	virtual void OnLevelDeserialized_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interactive | Door")
	UStaticMeshComponent* DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interactive | Door")
	USceneComponent* DoorPivot;//узел к которому будет привязываться наша мешка
	//и которая будет открываться и закрываться

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactive | Door")
	float AngleClosed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactive | Door")
	float AngleOpened = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactive | Door")
	UCurveFloat* DoorAnimationCurve;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	IInteractable::FOnInteraction OnInteractionEvent;

private:
	void InteractWithDoor();

	UFUNCTION()//функция которая будет вызываться во время тика таймлайна
	void UpdateDoorAnimation(float Alpha);

	UFUNCTION()
	void OnDoorAnimationFinished();

	FTimeline DoorOpenAnimTimeline;
	
	UPROPERTY(SaveGame)//атрибут отвечающей за то будет ли св-во сериализ или десериализ
	bool bIsOpened = false;

};
