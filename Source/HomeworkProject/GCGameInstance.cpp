// Fill out your copyright notice in the Description page of Project Settings.


#include "GCGameInstance.h"
#include "Net/UnrealNetwork.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemTypes.h"
#include "Kismet/GameplayStatics.h"

UGCGameInstance::UGCGameInstance()
{
	OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGCGameInstance::OnCreateSessionComplete);
	OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGCGameInstance::OnStartOnlineGameComplete);
	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UGCGameInstance::OnFindSessionsComplete);
	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UGCGameInstance::OnJoinSessionComplete);
	OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UGCGameInstance::OnDestroySessionComplete);
}

void UGCGameInstance::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGCGameInstance, ServerName);
	DOREPLIFETIME(UGCGameInstance, MaxPlayers);
}

bool UGCGameInstance::ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor)
{
	bool bResult = Super::ProcessConsoleExec(Cmd, Ar, Executor);

	//если консольная команда не отработалась, то нужно вызвать этот метод у всех подсистем
	if (!bResult)
	{
		TArray<UGameInstanceSubsystem*> Subsystems = GetSubsystemArray<UGameInstanceSubsystem>();
		for (UGameInstanceSubsystem* Subsystem : Subsystems)
		{
			bResult |= Subsystem->ProcessConsoleExec(Cmd, Ar, Executor);//результат обьединения
		}
	}
	return bResult;
}

void UGCGameInstance::Init()
{
	Super::Init();
	//NetworkFailureEvent-Сообщение об ошибке событие, связанное с сетевыми сбоями
	//TravelFailureEvent-Событие с сообщением об ошибке, связанное со сбоями в работе сервера
	OnNetworkFailureEventHandle = GEngine->NetworkFailureEvent.AddUFunction(this, "OnNetworkFailure");
	OnTravelFailureEventHandle = GEngine->TravelFailureEvent.AddUFunction(this, "OnTravelFailure");
}

void UGCGameInstance::Shutdown()
{
	//handle позволяет удалить конкретное событие
	GEngine->NetworkFailureEvent.Remove(OnNetworkFailureEventHandle);
	GEngine->TravelFailureEvent.Remove(OnTravelFailureEventHandle);
	Super::Shutdown();
}

void UGCGameInstance::LaunchLobby(uint32 MaxPlayers_in, FName ServerName_in, bool bIsLAN)
{
	MaxPlayers = MaxPlayers_in;
	ServerName = ServerName_in;
	HostSession(GetPrimaryPlayerUniqueId(), ServerName, bIsLAN, true, MaxPlayers);
	//GetPrimaryPlayerUniqueId - передаёт уникальный идентификатор основного игрока на этом компьютере
}

void UGCGameInstance::FindAMatch(bool bIsLAN)
{
	FindSessions(GetPrimaryPlayerUniqueId(), bIsLAN, true);
}

void UGCGameInstance::JoinOnlineGame()
{
	// Класс FOnlineSessionSearchResult содержит информацию о найденных сессиях, таких как ID сессии, 
	// показатели качества (например, количество игроков и максимальное количество игроков), состояние сессии и т. д.
	FOnlineSessionSearchResult SearchResult;

	TSharedPtr<const FUniqueNetId> PlayerUniqueNetId = GetPrimaryPlayerUniqueId();

	if (SessionSearch->SearchResults.Num() > 0)
	{
		for (int32 i = 0; i < SessionSearch->SearchResults.Num(); i++)
		{
			// обрабатываем ситуацию чтобы мы не подключались сами к себе
			if (SessionSearch->SearchResults[i].Session.OwningUserId != PlayerUniqueNetId)
			{
				SearchResult = SessionSearch->SearchResults[i];

				// Once we found sounce a Session that is not ours, just join it. Instead of using a for loop, you could
				// use a widget where you click on and have a reference for the GameSession it represents which you can use
				if (!JoinFoundOnlineSession(PlayerUniqueNetId, GameSessionName, SearchResult))
				{
					DisplayNetworkErrorMessage("Failed to join a session! Please try again!");
				}
				break;
			}
		}
	}
}

bool UGCGameInstance::HostSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers)
{
	// Get the Online Subsystem to work with
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		// Get the Session Interface, so we can call the "CreateSession" function on it
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid() && UserId.IsValid())
		{
			/*
				Fill in all the Session Settings that we want to use.

				There are more with SessionSettings.Set(...);
				For example the Map or the GameMode/Type.
			*/
			SessionSettings = MakeShareable(new FOnlineSessionSettings());

			SessionSettings->bIsLANMatch = bIsLAN;
			SessionSettings->bUsesPresence = bIsPresence;
			SessionSettings->NumPublicConnections = MaxNumPlayers;
			SessionSettings->NumPrivateConnections = 0;
			SessionSettings->bAllowInvites = true;
			SessionSettings->bAllowJoinInProgress = true;
			SessionSettings->bShouldAdvertise = true;
			SessionSettings->bAllowJoinViaPresence = true;
			SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;
			//выставляем карту
			SessionSettings->Set(SETTING_MAPNAME, LobbyMapName.ToString(), EOnlineDataAdvertisementType::ViaOnlineService);

			// Set the delegate to the Handle of the SessionInterface
			OnCreateSessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

			// Our delegate should get called when this is complete (doesn't need to be successful!)
			return Sessions->CreateSession(*UserId, SessionName, *SessionSettings);
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("No OnlineSubsytem found!"));
	}

	return false;
}

void UGCGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OnCreateSessionComplete %s, %d"), *SessionName.ToString(), bWasSuccessful));

	if (!bWasSuccessful)
	{
		DisplayNetworkErrorMessage("Failed to create session! Please try again");
		return;
	}

	// Get the OnlineSubsystem so we can get the Session Interface
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get the Session Interface to call the StartSession function
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid())
		{
			// Очищаем дескриптор делегата завершения сеанса, так как мы завершили этот вызов, чтобы если что иметь возможность подписаться
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
			if (bWasSuccessful)
			{
				// Set the StartSession delegate handle. Привязываем другой делегат на событие старт сессии.
				OnStartSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegate);

				// Our StartSessionComplete delegate should get called after this
				Sessions->StartSession(SessionName);
			}
		}
	}
}

void UGCGameInstance::OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OnStartSessionComplete %s, %d"), *SessionName.ToString(), bWasSuccessful));

	if (!bWasSuccessful)
	{
		DisplayNetworkErrorMessage(TEXT("OnStartSessionCompletCannot start online game! Please try again"));
		return;
	}

	// Get the Online Subsystem so we can get the Session Interface
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get the Session Interface to clear the Delegate
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			// Очищаем делегат, так как мы закончили с этим вызовом
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
		}
	}

	//Если запуск прошел успешно, мы можем открыть новую карту, если захотим. Обязательно используйте "listen" в качестве параметра!
	UGameplayStatics::OpenLevel(GetWorld(), LobbyMapName, true, "listen");//listen - нужен чтобы к карте LobbyMapName подключаться
}

void UGCGameInstance::FindSessions(TSharedPtr<const FUniqueNetId> UserId, bool bIsLAN, bool bIsPresence)
{
	// Get the OnlineSubsystem we want to work with
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		// Get the SessionInterface from our OnlineSubsystem
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid() && UserId.IsValid())
		{
			/*
				Fill in all the SearchSettings, like if we are searching for a LAN game and how many results we want to have!
				Заполните все настройки поиска, например, ищем ли мы игру по локальной сети и сколько результатов мы хотим получить!
			*/ //создаём запрос поиска сессии
			SessionSearch = MakeShareable(new FOnlineSessionSearch());

			SessionSearch->bIsLanQuery = bIsLAN;
			SessionSearch->MaxSearchResults = 20;
			SessionSearch->PingBucketSize = 50;

			// We only want to set this Query Setting if "bIsPresence" is true
			if (bIsPresence)
			{
				SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, bIsPresence, EOnlineComparisonOp::Equals);
			}
			//TSharedRef это по сути TSharedPtr, который гарантируется что он не пустой
			TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SessionSearch.ToSharedRef();

			// Set the Delegate to the Delegate Handle of the FindSession function
			OnFindSessionsCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

			// Finally call the SessionInterface function. The Delegate gets called once this is finished
			//Наконец, вызовите функцию интерфейса сеанса. Делегат будет вызван, как только это будет завершено
			Sessions->FindSessions(*UserId, SearchSettingsRef);
		}
	}
	else
	{
		// If something goes wrong, just call the Delegate Function directly with "false".
		OnFindSessionsComplete(false);
	}
}

void UGCGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OFindSessionsComplete bSuccess: %d"), bWasSuccessful));

	bool bIsMatchfound = false;//флаг нашли мы подходящий матч или нет
	// Get OnlineSubsystem we want to work with
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get SessionInterface of the OnlineSubsystem
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			// Clear the Delegate handle, since we finished this call
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);

			// Just debugging the Number of Search results. Can be displayed in UMG or something later on
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Num Search Results: %d"), SessionSearch->SearchResults.Num()));

			// If we have found at least 1 session, we just going to debug them. You could add them to a list of UMG Widgets, like it is done in the BP version!
			//Если мы найдем хотя бы 1 сессию, мы просто отладим их. Вы могли бы добавить их в список виджетов UMG, как это сделано в версии BP!
			if (SessionSearch->SearchResults.Num() > 0)
			{
				// "SessionSearch->SearchResults" is an Array that contains all the information. You can access the Session in this and get a lot of information.
				//								  это массив, содержащий всю информацию. Вы можете получить доступ к сеансу в этом разделе и получить много информации.
				// This can be customized later on with your own classes to add more information that can be set and displayed
				// Позже это можно будет настроить с помощью ваших собственных классов, чтобы добавить больше информации, которую можно будет настраивать и отображать
				bIsMatchfound = true;
				for (int32 SearchIdx = 0; SearchIdx < SessionSearch->SearchResults.Num(); SearchIdx++)
				{
					// OwningUserName is just the SessionName for now. I guess you can create your own Host Settings class and GameSession Class and add a proper GameServer Name here.
					// На данный момент OwningUserName - это просто название сеанса. Я думаю, вы можете создать свой собственный класс настроек хоста и класс GameSession и добавить сюда правильное название игрового сервера.
					// This is something you can't do in Blueprint for example!
					// Это то, чего вы не можете сделать, например, в Blueprint!
					GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Session Number: %d | Sessionname: %s "), SearchIdx + 1, *(SessionSearch->SearchResults[SearchIdx].Session.OwningUserName)));
				}
			}
		}
	}

	if (OnMatchFound.IsBound())
	{
		OnMatchFound.Broadcast(bIsMatchfound);
	}
}

bool UGCGameInstance::JoinFoundOnlineSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, const FOnlineSessionSearchResult& SearchResult)
{
	// Return bool
	bool bSuccessful = false;

	// Get OnlineSubsystem we want to work with
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		// Get SessionInterface from the OnlineSubsystem
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid() && UserId.IsValid())
		{
			// Set the Handle again
			OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

			// Call the "JoinSession" Function with the passed "SearchResult". The "SessionSearch->SearchResults" can be used to get such a
			// "FOnlineSessionSearchResult" and pass it. Pretty straight forward!
			bSuccessful = Sessions->JoinSession(*UserId, SessionName, SearchResult);
		}
	}

	return bSuccessful;
}

void UGCGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OnJoinSessionComplete %s, %d"), *SessionName.ToString(), static_cast<int32>(Result)));

	// Get the OnlineSubsystem we want to work with
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get SessionInterface from the OnlineSubsystem
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid())
		{
			// Clear the Delegate again
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

			// Get the first local PlayerController, so we can call "ClientTravel" to get to the Server Map
			// This is something the Blueprint Node "Join Session" does automatically!
			//Получите первый локальный PlayerController, чтобы мы могли вызвать "ClientTravel", чтобы перейти к карте сервера. 
			// Это то, что автоматически выполняет узел Blueprint "Join Session"!
			APlayerController* const PlayerController = GetFirstLocalPlayerController();

			// We need a FString to use ClientTravel and we can let the SessionInterface contruct such a
			// String for us by giving him the SessionName and an empty String. We want to do this, because
			// Every OnlineSubsystem uses different TravelURLs
			//Нам нужна строка для использования Client Travel, 
			// и мы можем позволить интерфейсу сеанса создать такую строку для нас, 
			// указав ему имя сеанса и пустую строку. Мы хотим сделать это, потому что каждая онлайн-подсистема 
			// использует разные TravelURLs
			FString TravelURL;

			if (IsValid(PlayerController) && Sessions->GetResolvedConnectString(SessionName, TravelURL))
			{
				// Finally call the ClienTravel. If you want, you could print the TravelURL to see
				// how it really looks like
				//ClientTravel - переводит клиента на новую сессию, загружает у него уровень и позволяет ему
				//присоединиться к готовой игре
				PlayerController->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
			}
		}
	}
}

void UGCGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OnDestroySessionComplete %s, %d"), *SessionName.ToString(), bWasSuccessful));

	// Get the OnlineSubsystem we want to work with
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get the SessionInterface from the OnlineSubsystem
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid())
		{
			// Clear the Delegate
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);

			// If it was successful, we just load another level (could be a MainMenu!)
			if (bWasSuccessful)
			{
				UGameplayStatics::OpenLevel(GetWorld(), MainMenuMapName, true);
			}
		}
	}
}

void UGCGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorMessage)
{
	DisplayNetworkErrorMessage(ErrorMessage);
}

void UGCGameInstance::OnTravelFailure(UWorld* World, ENetworkFailure::Type FailureType, const FString& ErrorMessage)
{
	DisplayNetworkErrorMessage(ErrorMessage);
}

void UGCGameInstance::DisplayNetworkErrorMessage(FString ErrorMessage)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, ErrorMessage);
}
