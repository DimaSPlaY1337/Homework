// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreamingSubsytemVolume.generated.h"

//эктор при пересечении которого будет выгружен список нужных уровней 
// или будет загружен список нужных
class UBoxComponent;
class UStreamingSubsystem;
class ACharacter;
UCLASS()
class HOMEWORKPROJECT_API AStreamingSubsytemVolume : public AActor
{
	GENERATED_BODY()
	
public:
	AStreamingSubsytemVolume();

	const TSet<FString>& GetLevelsToLoad() const;
	const TSet<FString>& GetLevelsToUnload() const;

	//обработка пересечения нашего персонажа с объёмом
	void HandleCharacterOverlapBegin(ACharacter* Character);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Collision")
	UBoxComponent* CollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	TSet<FString> LevelsToLoad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	TSet<FString> LevelsToUnload;

private:
	TWeakObjectPtr<UStreamingSubsystem> StreamingSubsystem;
	TWeakObjectPtr<ACharacter> OverlappedCharacter;
};
