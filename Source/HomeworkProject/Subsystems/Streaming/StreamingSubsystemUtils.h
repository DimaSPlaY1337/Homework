// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StreamingSubsystemUtils.generated.h"

/**
 * 
 */
//2 функции
//1 : пересекает ли персонаж какие то stream объемы
//2 : пересекает ли объем каких то персонажей
class ACharacter;
class AStreamingSubsytemVolume;
UCLASS()
class HOMEWORKPROJECT_API UStreamingSubsystemUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Streaming Subsystem Utils")
	static void CheckCharacterOverlapStreamingSubsystemVolume(ACharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Streaming Subsystem Utils")
	static void CheckStreamingSubsystemVolumeOverlapCharacter(AStreamingSubsytemVolume* SubsystemVolume);
};
