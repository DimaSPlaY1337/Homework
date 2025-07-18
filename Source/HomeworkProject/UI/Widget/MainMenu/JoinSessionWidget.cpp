// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/MainMenu/JoinSessionWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GCGameInstance.h"

void UJoinSessionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	//кешируем указатель на гейм инстанс
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());
	check(GameInstance->IsA<UGCGameInstance>());
	GCGameInstance = StaticCast<UGCGameInstance*>(GetGameInstance());
}

void UJoinSessionWidget::FindOnlineSession()
{
	GCGameInstance->OnMatchFound.AddUFunction(this, FName("OnMatchFound"));
	GCGameInstance->FindAMatch(bIsLAN);
	SearchingSessionState = ESearchingSessionState::Searching;
}

void UJoinSessionWidget::JoinOnlineSession()
{
	GCGameInstance->JoinOnlineGame();
}

void UJoinSessionWidget::CloseWidget()
{
	GCGameInstance->OnMatchFound.RemoveAll(this);
	Super::CloseWidget();//так как метод на закрытие рекомендуетс€ писать в конце
	//после всех отписок
}

void UJoinSessionWidget::OnMatchFound_Implementation(bool bIsSuccesful)
{
	SearchingSessionState = bIsSuccesful ? ESearchingSessionState::SessionIsFound : ESearchingSessionState::None;
	GCGameInstance->OnMatchFound.RemoveAll(this);//отписываем все делегаты с этого UObject
}
