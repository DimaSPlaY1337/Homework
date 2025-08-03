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
//������������ ������ �� �������� ������ �� ����������� ������ (SubsystemVolume). 
// ���� ��� ������ �������� ������ �� �������� � ��� �������� �� �������� � ���������� �������� ������.
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
//������� ����� ������������������ ������ �� �������� �� ������. 
// ���� ����� �������� �������� �� �������� � �������� �� �������� � ���������� �������� ������.
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
//������������ ������ �� �������� ������. 
//���� ��� ������ ������ �� �������� (�� ���� ������� �� ��������) � ���������� �������� ������.
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

//������� ������ �� ��������. ���� ����� �������� �� �������� �������� �� ��������, 
// �� ���� ������� �� �������� � �������� ���������� �������� ������.
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
	//��� ������� �������� ������, ��� ��������������� ������� 
	// (Streaming Level) ������ ���� �������� � ������
	StreamingLevel->SetShouldBeLoaded(true);
	//��� ������� �������� ������, ��� ������� ������ ����� ������� ��� ������ 
	// (�� ���� ��� ��� �������, �������, �������� � �.�. ������ ��������� �� �����).
	StreamingLevel->SetShouldBeVisible(true);
	//���� ���� ������� UE5: ������������ �������� ����� �� ������ �������� �������.
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