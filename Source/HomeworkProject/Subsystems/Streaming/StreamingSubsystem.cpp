// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/Streaming/StreamingSubsystem.h"
#include "Subsystems/Streaming/SystemSubsystemManager.h"
#include "Subsystems/Streaming/StreamingSubsytemVolume.h"

void UStreamingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingSubsystem::Initialize(): %s"), *GetNameSafe(this));

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UStreamingSubsystem::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UStreamingSubsystem::OnPostLoadMapWithWorld);

	CreateStreamingLevelManagers(GetWorld());
}

void UStreamingSubsystem::Deinitialize()
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingSubsystem::Deinitialize(): %s"), *GetNameSafe(this));

	//.RemoveAll(this); — отписывает все функции вашего объекта, ранее подписанные на этот делегат.
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	RemoveStreamingLevelManagers();

	Super::Deinitialize();
}

//Здесь, если GameInstance не валиден, объект пытается найти свой Outer и получить через него UWorld.
// То есть, если объект был создан внутри другого объекта(например, Subsystem создан внутри 
// GameMode или Actor), то у него сохранится ссылка на внешний контейнер, и через этот контейнер 
// можно добраться до нужных глобальных данных(например, мира игры).
UWorld* UStreamingSubsystem::GetWorld() const
{
	UGameInstance* GameInstance = GetGameInstance();
	if (IsValid(GameInstance))
	{
		return GameInstance->GetWorld();
	}

	//В Unreal Engine Outer — это “внешний” объект, контейнер или 
	// "владелец" (owner) данного объекта.
	UObject* Outer = GetOuter();
	if (IsValid(Outer))
	{
		return Outer->GetWorld();
	}

	return nullptr;
}

void UStreamingSubsystem::OnVolumeOverlapBegin(AStreamingSubsytemVolume* SubsystemVolume)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingSubsystem::OnVolumeOverlapBegin(): %s, SubsystemVolume: %s"), *GetNameSafe(this), *GetNameSafe(Cast<UObject>(SubsystemVolume)));

	for (const FString& LevelToLoad : SubsystemVolume->GetLevelsToLoad())
	{
		UStreamingSubsystemManager* LevelManager = nullptr;;
		if (!FindStreamingLevelManager(LevelToLoad, LevelManager))
		{
			continue;
		}

		LevelManager->AddLoadRequest(SubsystemVolume);
	}

	for (const FString& LevelToUnload : SubsystemVolume->GetLevelsToUnload())
	{
		UStreamingSubsystemManager* LevelManager = nullptr;;
		if (!FindStreamingLevelManager(LevelToUnload, LevelManager))
		{
			continue;
		}

		LevelManager->AddUnloadRequest(SubsystemVolume);
	}
}

void UStreamingSubsystem::OnVolumeOverlapEnd(AStreamingSubsytemVolume* SubsystemVolume)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingSubsystem::OnVolumeOverlapBegin(): %s, SubsystemVolume: %s"), *GetNameSafe(this), *GetNameSafe(SubsystemVolume));

	for (const FString& LevelToLoad : SubsystemVolume->GetLevelsToLoad())
	{
		UStreamingSubsystemManager* LevelManager = nullptr;;
		if (!FindStreamingLevelManager(LevelToLoad, LevelManager))
		{
			continue;
		}

		LevelManager->RemoveLoadRequest(SubsystemVolume);
	}

	for (const FString& LevelToUnload : SubsystemVolume->GetLevelsToUnload())
	{
		UStreamingSubsystemManager* LevelManager = nullptr;;
		if (!FindStreamingLevelManager(LevelToUnload, LevelManager))
		{
			continue;
		}

		LevelManager->RemoveUnloadRequest(SubsystemVolume);
	}
}

bool UStreamingSubsystem::CanUseSubsystem() const
{
	return StreamingLevelManagers.Num() > 0;
}

void UStreamingSubsystem::CreateStreamingLevelManagers(UWorld* World)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingSubsystem::CreateStreamingLevelManagers(): %s, World: %s"), *GetNameSafe(this), *GetNameSafe(World));

	RemoveStreamingLevelManagers();

	if (!IsValid(World))
	{
		return;
	}

	const TArray<ULevelStreaming*>& StreamingLevels = World->GetStreamingLevels();
	StreamingLevelManagers.Reserve(StreamingLevels.Num());
	for (ULevelStreaming* Level : StreamingLevels)
	{
		FString ShortLevelName = FPackageName::GetShortName(Level->GetWorldAssetPackageName());
		if (ShortLevelName.StartsWith(World->StreamingLevelsPrefix))
		{
			ShortLevelName = ShortLevelName.RightChop(World->StreamingLevelsPrefix.Len());
		}

		UStreamingSubsystemManager* Manager = NewObject<UStreamingSubsystemManager>(this);
		Manager->Initialize(Level, ShortLevelName);

		StreamingLevelManagers.Add(ShortLevelName, Manager);
	}
}

void UStreamingSubsystem::RemoveStreamingLevelManagers()
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingSubsystem::RemoveStreamingLevelManagers(): %s"), *GetNameSafe(this));

	for (TPair<FString, UStreamingSubsystemManager*>& Manager : StreamingLevelManagers)
	{
		if (!IsValid(Manager.Value))
		{
			continue;
		}
		Manager.Value->Deinitialize();
	}

	StreamingLevelManagers.Empty();
}

void UStreamingSubsystem::OnPreLoadMap(const FString& MapName)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingSubsystem::OnPreLoadMap(): %s, MapName: %s"), *GetNameSafe(this), *MapName);

	RemoveStreamingLevelManagers();
}

void UStreamingSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	UE_LOG(LogStreamingSubsystem, Display, TEXT("UStreamingSubsystem::OnPostLoadMapWithWorld(): %s, LoadedWorld: %s"), *GetNameSafe(this), *GetNameSafe(LoadedWorld));

	CreateStreamingLevelManagers(LoadedWorld);
}

bool UStreamingSubsystem::FindStreamingLevelManager(const FString& LevelName, UStreamingSubsystemManager*& LevelManager)
{
	UStreamingSubsystemManager** ManagerPtr = StreamingLevelManagers.Find(LevelName);
	if (ManagerPtr == nullptr || !IsValid(*ManagerPtr))
	{
		LevelManager = nullptr;

		return false;
	}

	LevelManager = *ManagerPtr;

	return true;
}