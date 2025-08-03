// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StreamingSubsystem.generated.h"

/**
 * 
 */
//–егестрирует сообщени€ от volume и передайт нужному streaming level-у.
class UStreamingSubsystemManager;
class AStreamingSubsytemVolume;
UCLASS()
class HOMEWORKPROJECT_API UStreamingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual UWorld* GetWorld() const override;

	void OnVolumeOverlapBegin(AStreamingSubsytemVolume* SubsystemVolume);
	void OnVolumeOverlapEnd(AStreamingSubsytemVolume* SubsystemVolume);

	bool CanUseSubsystem() const;

private:
	void CreateStreamingLevelManagers(UWorld* World);
	void RemoveStreamingLevelManagers();
	void OnPreLoadMap(const FString& MapName);
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);
	bool FindStreamingLevelManager(const FString& LevelName, UStreamingSubsystemManager*& LevelManager);

	UPROPERTY(Transient)
	TMap<FString, UStreamingSubsystemManager*> StreamingLevelManagers;
};
