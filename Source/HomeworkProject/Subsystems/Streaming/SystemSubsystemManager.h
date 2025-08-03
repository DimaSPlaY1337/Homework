// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/LevelStreaming.h"
#include "Subsystems/Streaming/StreamingSubsytemVolume.h"
#include "SystemSubsystemManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogStreamingSubsystem, Log, All);
/**
 * 
 */
//если на уровне есть хотя бы 1 ссылка на загрузку, то он загрузиться
//если уровень на выгрзку он никогда не будет загружаться
//class AStreamingSubsytemVolume;
UCLASS()
class HOMEWORKPROJECT_API UStreamingSubsystemManager : public UObject
{
	GENERATED_BODY()
public:
	void Initialize(ULevelStreaming* InStreamingLevel, const FString& InLevelName);
	void Deinitialize();

	void AddLoadRequest(AStreamingSubsytemVolume* SubsystemVolume);
	void RemoveLoadRequest(AStreamingSubsytemVolume* SubsystemVolume);

	void AddUnloadRequest(AStreamingSubsytemVolume* SubsystemVolume);
	void RemoveUnloadRequest(AStreamingSubsytemVolume* SubsystemVolume);

	ULevelStreaming::ECurrentState GetStreamingLevelState() const;

private:
	UFUNCTION()
	void OnLevelLoaded();

	UFUNCTION()
	void OnLevelUnloaded();

	void LoadLevel();
	void UnloadLevel();

	FString LevelName;
	ULevelStreaming::ECurrentState StreamingLevelState = ULevelStreaming::ECurrentState::Unloaded;
	TWeakObjectPtr<ULevelStreaming> StreamingLevel;
	TArray<TWeakObjectPtr<AStreamingSubsytemVolume>> LoadRequests;
	TArray<TWeakObjectPtr<AStreamingSubsytemVolume>> UnloadRequests;
};
