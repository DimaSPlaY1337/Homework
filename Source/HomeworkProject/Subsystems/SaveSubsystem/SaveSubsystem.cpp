// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveSubsystem.h"
#include "SaveSubsystemTypes.h"
#include "Kismet/GameplayStatics.h"
#include "SaveSubsystemInterface.h"
#include "SaveSubsystemUtils.h"
#include "GameFramework/Character.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"

const FGameSaveData & USaveSubsystem::GetGameSaveData() const
{
	return GameSaveData;
}

void USaveSubsystem::SaveGame()
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::SaveGame(): %s"), *GetNameSafe(this));

	SerializeGame();
	WriteSaveToFile();
}

void USaveSubsystem::LoadLastGame()
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::LoadLastGame(): %s"), *GetNameSafe(this));

	if (SaveIds.Num() == 0)//проверка есть ли сохраненные игры. Если их нет то ничего не делаем
	{
		UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::LoadLastGame(): %s Failed! No saves."), *GetNameSafe(this));
		return;
	}

	LoadGame(SaveIds[SaveIds.Num() - 1]);
}

void USaveSubsystem::LoadGame(int32 SaveId)
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::LoadGame()"));
	if (!SaveIds.Contains(SaveId))
	{
		UE_LOG(LogSaveSubsystem, Warning, TEXT("USaveSubsystem::LoadGame(): Failed!"));
		return;
	}

	LoadSaveFromFile(SaveId);
	UGameplayStatics::OpenLevel(this, GameSaveData.LevelName);
}

void USaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::Initialize(): %s"), *GetNameSafe(this));

	GameSaveData = FGameSaveData();

	SaveDirectoryName = FString::Printf(TEXT("%sSaveGames/"), *FPaths::ProjectSavedDir());

	FSaveDirectoryVisitor DirectoryVisitor(SaveIds);
	/*IterateDirectory — это функция интерфейса IPlatformFile в Unreal Engine, которая позволяет перебрать все файлы и 
	поддиректории в указанной директории и вызвать для каждого из них функцию посетителя(visitor).
	Она вызывает метод Visit объекта - посетителя один раз для каждого файла или каталога в указанной директории, 
	но не рекурсивно(то есть не входит в поддиректории)*/
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*SaveDirectoryName, DirectoryVisitor);
	SaveIds.Sort();

	//метод будет вызываться после того как карта загрузилась
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USaveSubsystem::OnPostLoadMapWithWorld);
}

void USaveSubsystem::Deinitialize()
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::Deinitialize(): %s"), *GetNameSafe(this));

	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Deinitialize();
}

void USaveSubsystem::SerializeLevel(const ULevel* Level, const ULevelStreaming* StreamingLevel /*= nullptr*/)
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::SerializeLevel(): %s, Level: %s, StreamingLevel: %s"), *GetNameSafe(this), *GetNameSafe(Level), *GetNameSafe(StreamingLevel));

	FLevelSaveData* LevelSaveData = nullptr;
	LevelSaveData = &GameSaveData.Level;

	TArray<FActorSaveData>& ActorsSaveData = LevelSaveData->ActorsSaveData;
	ActorsSaveData.Empty();

	for (AActor* Actor : Level->Actors)
	{
		//Implements Эта функция проверяет, поддерживает ли объект (Actor) определенный интерфейс (USaveSubsystemInterface)
		if (!IsValid(Actor) || !Actor->Implements<USaveSubsystemInterface>())
		{
			continue;
		}

		FActorSaveData& ActorSaveData = ActorsSaveData[ActorsSaveData.AddUnique(FActorSaveData(Actor))];
		ActorSaveData.Transform = Actor->GetTransform();
		TArray<FObjectSaveData>& ComponentsSaveData = ActorSaveData.ComponentsSaveData;
		ComponentsSaveData.Empty();//записываем все с нуля
		for (UActorComponent* ActorComponent : Actor->GetComponents())//получаем все компоненты эктора
		{
			if (ActorComponent->Implements<USaveSubsystemInterface>())
			{
				FObjectSaveData& ComponentSaveData = ComponentsSaveData[ComponentsSaveData.Emplace(ActorComponent)];
				//создаёт объект FMemoryWriter, который представляет собой поток для записи данных 
				// в память (в частности, в массив байтов). В данном случае
				FMemoryWriter MemoryWriter(ComponentSaveData.RawData, true);
				FSaveSubsystemArchive Archive(MemoryWriter, false);
				/*выполняет сериализацию(запись или чтение) всех свойств компонента в / из архива Archive, 
				который в данном случае обёрнут вокруг FMemoryWriter.Это значит, что все свойства компонента, 
				помеченные для сохранения(например, с флагом SaveGame), 
				будут записаны в память(в массив байтов ComponentSaveData.RawData через MemoryWriter).*/
				ActorComponent->Serialize(Archive);
			}
		}

		FMemoryWriter MemoryWriter(ActorSaveData.RawData, true);
		FSaveSubsystemArchive Archive(MemoryWriter, false);
		Actor->Serialize(Archive);
	}
}

void USaveSubsystem::DeserializeLevel(ULevel* Level, const ULevelStreaming* StreamingLevel /*= nullptr*/)
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::DeserializeLevel(): %s, Level: %s, StreamingLevel: %s"), *GetNameSafe(this), *GetNameSafe(Level), *GetNameSafe(StreamingLevel));

	FLevelSaveData* LevelSaveData = nullptr;
	LevelSaveData = &GameSaveData.Level;

	if (LevelSaveData == nullptr)
	{
		// There is no save for this level yet. Call OnLevelDeserialized() in all cases!
		USaveSubsystemUtils::BroadcastOnLevelDeserialized(Level);
		return;
	}

	TArray<AActor*> ActorsToNotify;

	TArray<FActorSaveData*> ActorsSaveData;
	//Reserve - Он резервирует внутренний буфер массива так, чтобы он мог вместить как минимум указанное число элементов
	ActorsSaveData.Reserve(LevelSaveData->ActorsSaveData.Num());
	for (FActorSaveData& ActorSaveData : LevelSaveData->ActorsSaveData)
	{
		ActorsSaveData.Add(&ActorSaveData);
	}

	for (TArray<AActor*>::TIterator ActorIterator = Level->Actors.CreateIterator(); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (!IsValid(Actor) || !Actor->Implements<USaveSubsystemInterface>())
		{
			continue;
		}

		FActorSaveData* ActorSaveData = nullptr;
		for (TArray<FActorSaveData*>::TIterator ActorSaveDataIterator = ActorsSaveData.CreateIterator(); ActorSaveDataIterator; ++ActorSaveDataIterator)
		{
			if ((*ActorSaveDataIterator)->Name == Actor->GetFName())
			{
				ActorSaveData = *ActorSaveDataIterator;
				ActorSaveDataIterator.RemoveCurrent();
				break;
			}
		}

		if (ActorSaveData == nullptr)
		{
			UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::DeserializeLevel(): %s, ActorSaveData not found! Destroy actor: %s"), *GetNameSafe(this), *GetNameSafe(Actor));
			Actor->Destroy();
		}
		else
		{
			DeserializeActor(Actor, ActorSaveData);

			if (Actor->Implements<USaveSubsystemInterface>())
			{
				ActorsToNotify.Add(Actor);
			}
		}
	}

	UWorld* const World = GetWorld();

	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.OverrideLevel = Level;
	ActorSpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (FActorSaveData* ActorSaveData : ActorsSaveData)
	{
		UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::DeserializeLevel(): %s, Spawn new actor with name: %s"), *GetNameSafe(this), *ActorSaveData->Name.ToString());

		ActorSpawnParameters.Name = ActorSaveData->Name;

		// @bIgnoreOnActorSpawnedCallbackFast is hook to avoid double @OnLevelDeserialized invocation!
		BoolScopeWrapper OnActorSpawnedHook(bIgnoreOnActorSpawnedCallback, true);

		AActor* Actor = World->SpawnActor(ActorSaveData->Class.Get(), &ActorSaveData->Transform, ActorSpawnParameters);

		if (!IsValid(Actor))
		{
			// This is valid case. Actor can be IsPendingKill due some actor's logic started from overlaps.
			UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::DeserializeLevel(): %s, Failed to spawn new actor with name: %s"), *GetNameSafe(this), *ActorSaveData->Name.ToString());
			continue;
		}

		// Actor name can change so update it.
		ActorSaveData->Name = Actor->GetFName();

		DeserializeActor(Actor, ActorSaveData);

		if (Actor->Implements<USaveSubsystemInterface>())
		{
			ActorsToNotify.Add(Actor);
		}
	}

	for (AActor* Actor : ActorsToNotify)
	{
		NotifyActorsAndComponents(Actor);
	}
}

void USaveSubsystem::NotifyActorsAndComponents(AActor* Actor)
{
	ISaveSubsystemInterface::Execute_OnLevelDeserialized(Actor);
	for (UActorComponent* ActorComponent : Actor->GetComponents())
	{
		if (ActorComponent->Implements<USaveSubsystemInterface>())
		{
			//это означает вызов функции интерфейса на объекте, который реализует определённый интерфейс (в данном случае — ISaveSubsystemInterface).
			ISaveSubsystemInterface::Execute_OnLevelDeserialized(ActorComponent);
		}
	}
}

void USaveSubsystem::SerializeGame()
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::SerializeGame(): %s"), *GetNameSafe(this));

	UGameInstance* GameInstance = GetGameInstance();
	GameSaveData.GameInstance = FObjectSaveData(GetGameInstance());
	FMemoryWriter MemoryWriter(GameSaveData.GameInstance.RawData, true);
	FSaveSubsystemArchive Archive(MemoryWriter, false);
	GameInstance->Serialize(Archive);

	const UWorld* World = GetWorld();
	FString LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	if (World->IsPlayInEditor())
	{
		LevelName = UWorld::RemovePIEPrefix(LevelName);
	}

	GameSaveData.LevelName = FName(LevelName);

	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (IsValid(PlayerCharacter))
	{
		GameSaveData.StartTransform = PlayerCharacter->GetTransform();
	}

	SerializeLevel(World->PersistentLevel);
}

void USaveSubsystem::DeserializeGame()
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::DeserializeGame(): %s"), *GetNameSafe(this));

	if (!GameSaveData.bIsSerialized)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	FMemoryReader MemoryReader(GameSaveData.GameInstance.RawData, true);
	FSaveSubsystemArchive Archive(MemoryReader, false);
	GameInstance->Serialize(Archive);//так как появился reader, вызывает операцию чтения

	const UWorld* World = GetWorld();

	DeserializeLevel(World->PersistentLevel);
}

void USaveSubsystem::WriteSaveToFile()
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::WriteSaveToFile(): %s"), *GetNameSafe(this));

	const int32 SaveId = GetNextSaveId();
	SaveIds.AddUnique(SaveId);

	GameSaveData.Name = FName(FString::FromInt(SaveId));

	TArray<uint8> SaveBytes;
	FMemoryWriter MemoryWriter(SaveBytes);
	FObjectAndNameAsStringProxyArchive WriterArchive(MemoryWriter, false);
	GameSaveData.Serialize(WriterArchive);

	FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*GetSaveFilePath(SaveId));

	if (bUseCompressedSaves)
	{
		TArray<uint8> CompressedSaveBytes;
		//FArchiveSaveCompressedProxy — это специальный архив (обёртка над FArchive), который сжимает данные при записи
		//CompressedSaveBytes — это массив байтов (TArray<uint8>), куда будут записаны сжатые данные.
		//NAME_Zlib — указывает, что для сжатия используется алгоритм Zlib (один из стандартных алгоритмов сжатия).
		FArchiveSaveCompressedProxy CompressedArchive(CompressedSaveBytes, NAME_Zlib);
		//Оператор << здесь перегружен для работы с архивами.Эта строка записывает(сериализует) данные из массива SaveBytes в архив CompressedArchive.
		CompressedArchive << SaveBytes;
		//Метод Flush() завершает процесс записи и сжатия, гарантируя, что все буферизированные данные будут записаны в CompressedSaveBytes.
		CompressedArchive.Flush();

		*FileWriter << CompressedSaveBytes;
	}
	else
	{
		*FileWriter << SaveBytes;
	}

	FileWriter->Close();
	delete FileWriter;
}

void USaveSubsystem::LoadSaveFromFile(int32 SaveId)
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::LoadSaveFromFile(): %s, SaveId %i"), *GetNameSafe(this), SaveId);

	FArchive* FileReader = IFileManager::Get().CreateFileReader(*GetSaveFilePath(SaveId));
	TArray<uint8> SaveBytes;

	if (bUseCompressedSaves)
	{
		TArray<uint8> CompressedSaveBytes;
		*FileReader << CompressedSaveBytes;

		FArchiveLoadCompressedProxy DecompressedArchive(CompressedSaveBytes, NAME_Zlib);
		DecompressedArchive << SaveBytes;
		DecompressedArchive.Flush();
	}
	else
	{
		*FileReader << SaveBytes;
	}

	FileReader->Close();
	delete FileReader;

	FMemoryReader MemoryReader(SaveBytes, true);
	FObjectAndNameAsStringProxyArchive ReaderArchive(MemoryReader, true);
	GameSaveData = FGameSaveData();
	GameSaveData.Serialize(ReaderArchive);//считываем информацию из архива
}

void USaveSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::OnPostLoadMapWithWorld(): %s, World: %s"), *GetNameSafe(this), *GetNameSafe(LoadedWorld));
	DeserializeGame();
}

void USaveSubsystem::DeserializeActor(AActor* Actor, const FActorSaveData* ActorSaveData)
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::DeserializeActor(): %s, Actor %s"), *GetNameSafe(this), *GetNameSafe(Actor));

	Actor->SetActorTransform(ActorSaveData->Transform);

	const TArray<FObjectSaveData>& ComponentsSaveData = ActorSaveData->ComponentsSaveData;
	for (UActorComponent* ActorComponent : Actor->GetComponents())
	{
		if (ActorComponent->Implements<USaveSubsystemInterface>())
		{
			//FindByPredicate Здесь перебираются все элементы ComponentsSaveData, и если находится элемент с нужным Name, возвращается указатель на этот элемент.
			//Параметр SaveData в лямбде — это каждый элемент массива ComponentsSaveData, который перебирается внутри метода FindByPredicate.
			const FObjectSaveData* ComponentSaveData = ComponentsSaveData.FindByPredicate([=](const FObjectSaveData& SaveData) { return SaveData.Name == ActorComponent->GetFName(); });
			//FMemoryReader позволяет "читать" эти байты как поток(эмулирует файл в памяти, для процесса десериализации).
			FMemoryReader MemoryReader(ComponentSaveData->RawData, true);
			FSaveSubsystemArchive Archive(MemoryReader, false);
			//Десериализуются данные компонента: он "читает" себя из этих байтов — компонент полностью восстанавливает своё сохранённое состояние.
			ActorComponent->Serialize(Archive);
		}
	}

	FMemoryReader MemoryReader(ActorSaveData->RawData, true);
	FSaveSubsystemArchive Archive(MemoryReader, false);
	Actor->Serialize(Archive);
}

FString USaveSubsystem::GetSaveFilePath(int32 SaveId) const
{
	return SaveDirectoryName / FString::Printf(TEXT("%i.save"), SaveId);
}

int32 USaveSubsystem::GetNextSaveId() const
{
	if (SaveIds.Num() == 0)
	{
		return 1;
	}

	return SaveIds[SaveIds.Num() - 1] + 1;
}

void USaveSubsystem::OnActorSpawned(AActor* SpawnedActor)
{
	UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::OnActorSpawned(): %s, Actor %s"), *GetNameSafe(this), *GetNameSafe(SpawnedActor));
	if (bIgnoreOnActorSpawnedCallback)
	{
		UE_LOG(LogSaveSubsystem, Display, TEXT("USaveSubsystem::OnActorSpawned(): %s, Skipped for actor %s. bIgnoreOnActorSpawnedCallback == true!"), *GetNameSafe(this), *GetNameSafe(SpawnedActor));
		return;
	}

	if (IsValid(SpawnedActor) && SpawnedActor->Implements<USaveSubsystemInterface>())
	{
		// We should notify a runtime spawned actors too.
		NotifyActorsAndComponents(SpawnedActor);
	}
}

