// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/Streaming/SystemSubsystemManager.h"
#include "UObject/UObjectBaseUtility.h"
#include "UObject/NameTypes.h"

DEFINE_LOG_CATEGORY(LogStreamingSubsystem)

void UStreamingSubsystemManager::Initialize(ULevelStreaming* InStreamingLevel, const FString& InLevelName)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingLevelManager::Initialize(): StreamingLevel: %s, LevelName: %s"), *GetNameSafe(InStreamingLevel), *InLevelName);
	if (!IsValid(InStreamingLevel))
	{
		return;
	}

	LevelName = InLevelName;

	StreamingLevel = InStreamingLevel;
	StreamingLevelState = StreamingLevel->GetCurrentState();

	StreamingLevel->OnLevelShown.AddUniqueDynamic(this, &UStreamingSubsystemManager::OnLevelLoaded);
	StreamingLevel->OnLevelHidden.AddUniqueDynamic(this, &UStreamingSubsystemManager::OnLevelUnloaded);
}

void UStreamingSubsystemManager::Deinitialize()
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingLevelManager::Deinitialize(): StreamingLevel: %s, LevelName: %s"), *GetNameSafe(StreamingLevel.Get()), *LevelName);

	if (StreamingLevel.IsValid())
	{
		StreamingLevel->OnLevelShown.RemoveDynamic(this, &UStreamingSubsystemManager::OnLevelLoaded);
		StreamingLevel->OnLevelHidden.RemoveDynamic(this, &UStreamingSubsystemManager::OnLevelUnloaded);
	}

	StreamingLevel.Reset();
}
//–егистрирует запрос на загрузку уровн€ от конкретного объЄма (SubsystemVolume). 
// ≈сли это первый активный запрос на загрузку и нет запросов на выгрузку Ч инициирует загрузку уровн€.
void UStreamingSubsystemManager::AddLoadRequest(AStreamingSubsytemVolume* SubsystemVolume)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingLevelManager::AddLoadRequest(): LevelName: %s, SubsystemVolume: %s"), *LevelName, *GetNameSafe(Cast<UObject>(SubsystemVolume)));
	if (LoadRequests.Contains(SubsystemVolume))
	{
		return;
	}

	LoadRequests.AddUnique(SubsystemVolume);

	if (LoadRequests.Num() == 1 && UnloadRequests.Num() == 0)
	{
		LoadLevel();
	}
}
//”бирает ранее зарегистрированный запрос на загрузку от объЄма. 
// ≈сли после удалени€ запросов на загрузку и выгрузку не осталось Ч инициирует выгрузку уровн€.
void UStreamingSubsystemManager::RemoveLoadRequest(AStreamingSubsytemVolume* SubsystemVolume)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingLevelManager::RemoveLoadRequest(): LevelName: %s, SubsystemVolume: %s"), *LevelName, *GetNameSafe(Cast<UObject>(SubsystemVolume)));
	if (!LoadRequests.Contains(SubsystemVolume))
	{
		return;
	}

	LoadRequests.Remove(SubsystemVolume);

	if (LoadRequests.Num() == 0 && UnloadRequests.Num() == 0)
	{
		UnloadLevel();
	}
}
//–егистрирует запрос на выгрузку уровн€. 
//≈сли это первый запрос на выгрузку (но есть запросы на загрузку) Ч инициирует выгрузку уровн€.
void UStreamingSubsystemManager::AddUnloadRequest(AStreamingSubsytemVolume* SubsystemVolume)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingLevelManager::AddUnloadRequest(): LevelName: %s, SubsystemVolume: %s"), *LevelName, *GetNameSafe(Cast<UObject>(SubsystemVolume)));
	if (UnloadRequests.Contains(SubsystemVolume))
	{
		return;
	}

	UnloadRequests.AddUnique(SubsystemVolume);

	if (UnloadRequests.Num() == 1 && LoadRequests.Num() != 0)
	{
		UnloadLevel();
	}
}

//”бирает запрос на выгрузку. ≈сли после удалени€ не осталось запросов на выгрузку, 
// но есть запросы на загрузку Ч повторно инициирует загрузку уровн€.
void UStreamingSubsystemManager::RemoveUnloadRequest(AStreamingSubsytemVolume* SubsystemVolume)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingLevelManager::RemoveUnloadRequest(): LevelName: %s, SubsystemVolume: %s"), *LevelName, *GetNameSafe(Cast<UObject>(SubsystemVolume)));
	if (!UnloadRequests.Contains(SubsystemVolume))
	{
		return;
	}

	UnloadRequests.Remove(SubsystemVolume);

	if (UnloadRequests.Num() == 0 && LoadRequests.Num() != 0)
	{
		LoadLevel();
	}
}

ULevelStreaming::ECurrentState UStreamingSubsystemManager::GetStreamingLevelState() const
{
	return StreamingLevelState;
}

void UStreamingSubsystemManager::OnLevelLoaded()
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingLevelManager::OnLevelLoaded(): LevelName: %s"), *LevelName);
	if (!StreamingLevel.IsValid())
	{
		return;
	}

	StreamingLevelState = StreamingLevel->GetCurrentState();
}

void UStreamingSubsystemManager::OnLevelUnloaded()
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingLevelManager::OnLevelUnloaded(): LevelName: %s"), *LevelName);
	if (!StreamingLevel.IsValid())
	{
		return;
	}

	StreamingLevelState = StreamingLevel->GetCurrentState();
}

void UStreamingSubsystemManager::LoadLevel()
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingLevelManager::LoadLevel(): LevelName: %s"), *LevelName);
	if (!StreamingLevel.IsValid())
	{
		return;
	}
	//Ёта функци€ сообщает движку, что соответствующий уровень 
	// (Streaming Level) должен быть загружен в пам€ть
	StreamingLevel->SetShouldBeLoaded(true);
	//Ёта функци€ сообщает движку, что уровень должен стать видимым дл€ игрока 
	// (то есть все его объекты, актиоры, ландшафт и т.п. должны по€витьс€ на сцене).
	StreamingLevel->SetShouldBeVisible(true);
	//Ётот флаг говорит UE5: УЅлокировать основной поток до полной загрузки уровн€Ф.
	StreamingLevel->bShouldBlockOnLoad = true;
}

void UStreamingSubsystemManager::UnloadLevel()
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingLevelManager::UnloadLevel(): LevelName: %s"), *LevelName);
	if (!StreamingLevel.IsValid())
	{
		return;
	}

	StreamingLevel->SetShouldBeLoaded(false);
	StreamingLevel->SetShouldBeVisible(false);
	StreamingLevel->bShouldBlockOnUnload = true;
}