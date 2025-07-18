// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "GCBaseCharacter.h"
#include "Actors/Equipment/Throwables/ThrowableItem.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "GameCodeTypes.h"
#include "Net/UnrealNetwork.h"
#include "UI/Widget/Equipment/EquipmentViewWidget.h"
#include "UI/Widget/Equipment/WeaponWheelWidget.h"


UCharacterEquipmentComponent::UCharacterEquipmentComponent()
{
	//рекомендуемый метод для вызова из компонент
	SetIsReplicatedByDefault(true);
}

void UCharacterEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCharacterEquipmentComponent, CurrentEquippedSlot);
	DOREPLIFETIME(UCharacterEquipmentComponent, AmunitionArray);
	DOREPLIFETIME(UCharacterEquipmentComponent, ItemsArray);
}

EEquippedItemType UCharacterEquipmentComponent::GetCurrentEquippedItemType() const
{
	EEquippedItemType Result = EEquippedItemType::None;
	if (IsValid(CurrentEquippedItem))
	{
		Result = CurrentEquippedItem->GetItemType();
	}
	return Result;
}

ARangeWeaponItem* UCharacterEquipmentComponent::GetCurrentRangeWeapon() const
{
	return CurrentEquippedWeapon;
}

AMeleeWeaponItem* UCharacterEquipmentComponent::GetCurrentMeleeWeapon() const
{
	return CurrentMeleeWeapon;
}

void UCharacterEquipmentComponent::ReloadCurrentWeapon()
{
	check(IsValid(CurrentEquippedWeapon));
	int32 AvailableAmunition = GetAvailableAmunitionForCurrentWeapon();
	if (AvailableAmunition <= 0)
	{
		return;
	}

	CurrentEquippedWeapon->Server_StartReload();
}

//экипируем текущее оружие предыдущее удаляем
void UCharacterEquipmentComponent::EquipItemInSlot(EEquipmentSlots Slot)
{
	if (bIsEquipping)
	{
		return;
	}

	UnEquipCurrentItem();
	InitialiseCurrentObjects(Slot);

	if (IsValid(CurrentThrowableItem))
	{
		if (CurrentThrowableItem->GetItemsAmount() == 0)
		{
			EquipItemInSlot(PreviousEquippedSlot);//криво выглядит. Переставить вверх InitialiseCurrentObjects вызывает ошибку.
			return;
		}
	}

	if (IsValid(CurrentEquippedItem))
	{
		UAnimMontage* EquipMontage = CurrentEquippedItem->GetCharacterEquipAnimMontage();
		if (IsValid(EquipMontage))
		{
			//Multicast_PlayMontage(EquipMontage);
			bIsEquipping = true;
			float EquipDuration = CachedBaseCharacter->PlayAnimMontage(EquipMontage);
			GetWorld()->GetTimerManager().SetTimer(EquipTimer, this, &UCharacterEquipmentComponent::EquipAnimationFinished, EquipDuration, false);
		}
		else
		{
			AttachCurrentItemToEquippedSocket();
		}
		CurrentEquippedSlot = Slot;
		CurrentEquippedItem->Equip();
	}

	if (IsValid(CurrentEquippedWeapon))
	{
		CurrentAmunitionType = CurrentEquippedWeapon->GetAmmoType();
		OnCurrentWeaponAmmoChangedHandle = CurrentEquippedWeapon->OnAmmoChanged.AddUFunction(this, FName("OnCurrentWeaponAmmoChanged"));
		OnCurrentWeaponReloadedHandle = CurrentEquippedWeapon->OnReloadComplete.AddUFunction(this, FName("OnWeaponReloadComplete"));
		if (GetCurrentRangeWeapon()->AvaliableOptionalAmmo && IsAmmoChanged)
		{
			OnCurrentWeaponAmmoChanged(CurrentEquippedWeapon->GetOptionalAmmo());
		}
		else
		{
			OnCurrentWeaponAmmoChanged(CurrentEquippedWeapon->GetAmmo());
		}
	}

	if (IsValid(CurrentThrowableItem))
	{
		CurrentThrowableItem->OnThrowableItemsAmountChanged.AddUObject(this, &UCharacterEquipmentComponent::OnCurrentThrowableItemsAmountChanged);
	}

	if (OnEquippedItemChanged.IsBound())
	{
		OnEquippedItemChanged.Broadcast(CurrentEquippedItem);
	}

	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_EquipItemInSlot(CurrentEquippedSlot);
	}
}

void UCharacterEquipmentComponent::EquipNextItem()
{
	if (CachedBaseCharacter->IsPlayerControlled())
	{
		if (IsSelectingWeapon())
			WeaponWheelWidget->NextSegment();
		else
		{
			APlayerController* PlayerController = CachedBaseCharacter->GetController<APlayerController>();
			OpenWeaponWheel(PlayerController);
		}
		return;
	}

	uint32 CurrentSlotIndex = (uint32)CurrentEquippedSlot;
	uint32 NextSlotIndex = NextItemsArraySlotIndex(CurrentSlotIndex);

	while (CurrentSlotIndex != NextSlotIndex
		&& IgnoreSlotsWhileSwitching.Contains((EEquipmentSlots)NextSlotIndex)
		&& IsValid(ItemsArray[NextSlotIndex]))
	{
		NextSlotIndex = NextItemsArraySlotIndex(NextSlotIndex);
	}
	if (CurrentSlotIndex != NextSlotIndex)
	{
		EquipItemInSlot((EEquipmentSlots)NextSlotIndex);
	}
}

void UCharacterEquipmentComponent::EquipPreviousItem()
{
	if (CachedBaseCharacter->IsPlayerControlled())
	{
		if (IsSelectingWeapon())
			WeaponWheelWidget->PreviousSegment();
		else
		{
			APlayerController* PlayerController = CachedBaseCharacter->GetController<APlayerController>();
			OpenWeaponWheel(PlayerController);
		}
		return;
	}//6:32

	uint32 CurrentSlotIndex = (uint32)CurrentEquippedSlot;
	uint32 PreviousSlotIndex = PreviousItemsArraySlotIndex(CurrentSlotIndex);

	while (CurrentSlotIndex != PreviousSlotIndex
		&& IgnoreSlotsWhileSwitching.Contains((EEquipmentSlots)PreviousSlotIndex)
		&& IsValid(ItemsArray[PreviousSlotIndex]))
	{
		PreviousSlotIndex = PreviousItemsArraySlotIndex(PreviousSlotIndex);
	}
	if (CurrentSlotIndex != PreviousSlotIndex)
	{
		EquipItemInSlot((EEquipmentSlots)PreviousSlotIndex);
	}
}

bool UCharacterEquipmentComponent::AddEquipmentItemToSlot(const TSubclassOf<AEquipableItem> EquipableItemClass, int32 SlotIndex)
{
	if (!IsValid(EquipableItemClass))
		return false;

	AEquipableItem* DefaultItemObject = EquipableItemClass->GetDefaultObject<AEquipableItem>();
	if (!DefaultItemObject->IsSlotCompatible((EEquipmentSlots)SlotIndex))//проверка можем ли мы положить наше оружие в наш слот
		return false;
	
	if (!IsValid(ItemsArray[SlotIndex]))
	{
		AEquipableItem* Item = GetWorld()->SpawnActor<AEquipableItem>(EquipableItemClass);
		Item->AttachToComponent(CachedBaseCharacter->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Item->GetUnEquippedSocketName());//прикрепляем оружие к сокету
		Item->SetOwner(CachedBaseCharacter.Get());//выставляем владельца
		Item->UnEquip();
		ItemsArray[SlotIndex] = Item;
	}
	else if(DefaultItemObject->IsA<ARangeWeaponItem>())
	{
		ARangeWeaponItem* RangeWeaponObject = StaticCast<ARangeWeaponItem*>(DefaultItemObject);
		int32 AmmoSlotIndex = (int32)RangeWeaponObject->GetAmmoType();
		AmunitionArray[SlotIndex] += RangeWeaponObject->GetMaxAmmo();
	}

	return true;
}

void UCharacterEquipmentComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	if ((int32)CurrentEquippedSlot == SlotIndex)
		UnEquipCurrentItem();
	ItemsArray[SlotIndex]->Destroy();
	ItemsArray[SlotIndex] = nullptr;
}

void UCharacterEquipmentComponent::OpenViewEquipment(APlayerController* PlayerController)
{
	if (!IsValid(ViewWidget))
	{
		CreateEquipmentWidgets(PlayerController);
	}

	if (!ViewWidget->IsVisible())
	{
		ViewWidget->AddToViewport();
	}
}

void UCharacterEquipmentComponent::CloseViewEquipment()
{
	if (ViewWidget->IsVisible())
	{
		ViewWidget->RemoveFromParent();
	}
}

bool UCharacterEquipmentComponent::IsViewVisible() const
{
	bool Result = false;
	if (IsValid(ViewWidget))
		Result = ViewWidget->IsVisible();

	return Result;
}

const TArray<AEquipableItem*>& UCharacterEquipmentComponent::GetItems() const
{
	return ItemsArray;
}

void UCharacterEquipmentComponent::OpenWeaponWheel(APlayerController* PlayerController)
{
	if (!IsValid(WeaponWheelWidget))
		CreateEquipmentWidgets(PlayerController);

	if (!WeaponWheelWidget->IsVisible())
		WeaponWheelWidget->AddToViewport();
}

bool UCharacterEquipmentComponent::IsSelectingWeapon() const
{
	return IsValid(WeaponWheelWidget) && WeaponWheelWidget->IsVisible();
}

void UCharacterEquipmentComponent::ConfirmWeaponSelection() const
{
	WeaponWheelWidget->ConfirmSelection();
}

void UCharacterEquipmentComponent::OnLevelDeserialized_Implementation()
{
	EquipItemInSlot(CurrentEquippedSlot);
}

void UCharacterEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->IsA<AGCBaseCharacter>(), TEXT("can be used only with a BaseCharacter"));
	CachedBaseCharacter = StaticCast<AGCBaseCharacter*>(GetOwner());//StaticCast т.к мы знаем что работаем только с 1 объектом
	CreateLoadout();
	AutoEquip();
}

void UCharacterEquipmentComponent::UnEquipCurrentItem()
{
	if (IsValid(CurrentEquippedItem))
	{
		CurrentEquippedItem->AttachToComponent(CachedBaseCharacter->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, CurrentEquippedItem->GetUnEquippedSocketName());
		CurrentEquippedItem->UnEquip();
	}
	if (IsValid(CurrentEquippedWeapon))
	{
		CurrentEquippedWeapon->StopFire();
		CurrentEquippedWeapon->EndReload(false);
		CurrentEquippedWeapon->OnAmmoChanged.Remove(OnCurrentWeaponAmmoChangedHandle);
		CurrentEquippedWeapon->OnReloadComplete.Remove(OnCurrentWeaponReloadedHandle);
	}
	PreviousEquippedSlot = CurrentEquippedSlot;
	CurrentEquippedSlot = EEquipmentSlots::None;
}

void UCharacterEquipmentComponent::AttachCurrentItemToEquippedSocket()
{
	if (IsValid(CurrentEquippedItem))
	{
		CurrentEquippedItem->AttachToComponent(CachedBaseCharacter->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, CurrentEquippedItem->GetEquippedSocketName());
	}
}

void UCharacterEquipmentComponent::LaunchCurrentThrowableItem_Implementation()
{
	if (CurrentThrowableItem)
	{
		CurrentThrowableItem->Throw();
		bIsEquipping = false;//выключаем в тот момент когда бросаем, чтобы корректно сработало EquipItemInSlot
		int32 ItemsAmount = CurrentThrowableItem->GetItemsAmount();
		ItemsAmount -= 1;
		OnCurrentThrowableItemsAmountChanged(ItemsAmount);
		CurrentThrowableItem->SetItemsAmount(ItemsAmount);
		EquipItemInSlot(PreviousEquippedSlot);
	}
}

bool UCharacterEquipmentComponent::CanChangeAmmoType() const
{
	if (CurrentEquippedItem->GetItemType() == EEquippedItemType::Rifle && CurrentEquippedWeapon->AvaliableOptionalAmmo)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void UCharacterEquipmentComponent::ChangeFireMode()
{
	if (CanChangeAmmoType())
	{
		CurrentEquippedWeapon->ChangeAmmoType();
		if (OnCurrentWeaponAmmoChangedEvent.IsBound())
		{
			if (!IsAmmoChanged)
			{
				CurrentEquippedWeapon->SetCurrentSlotsType(EAmunitionType::RifleGranades);
				CurrentAmunitionType = EAmunitionType::RifleGranades;
				uint32 Ammo = GetCurrentRangeWeapon()->GetOptionalAmmo();
				int32 ClipAmount = *MaxAmunitionAmount.Find(EAmunitionType::RifleGranades);
				OnCurrentWeaponAmmoChangedEvent.Broadcast(Ammo, ClipAmount);
				CurrentEquippedWeapon->SetOptionalAmmo(Ammo);
				IsAmmoChanged = true;
			}
			else
			{
				CurrentEquippedWeapon->SetCurrentSlotsType(EAmunitionType::Rifle);
				CurrentAmunitionType = EAmunitionType::Rifle;
				uint32 Ammo = GetCurrentRangeWeapon()->GetAmmo();
				int32 ClipAmount = *MaxAmunitionAmount.Find(EAmunitionType::Rifle);
				OnCurrentWeaponAmmoChangedEvent.Broadcast(Ammo, ClipAmount);
				CurrentEquippedWeapon->SetAmmo(Ammo);
				IsAmmoChanged = false;
			}
		}
	}
}

bool UCharacterEquipmentComponent::IsEquipping() const
{
	return bIsEquipping;
}

void UCharacterEquipmentComponent::Server_EquipItemInSlot_Implementation(EEquipmentSlots Slot)
{
	EquipItemInSlot(Slot);
}

void UCharacterEquipmentComponent::CreateEquipmentWidgets(APlayerController* PlayerController)
{
	checkf(IsValid(ViewWidgetClass), TEXT("view widget"));

	if (!IsValid(PlayerController))
		return;

	ViewWidget = CreateWidget<UEquipmentViewWidget>(PlayerController, ViewWidgetClass);
	ViewWidget->InitializeEquipmentWidget(this);

	WeaponWheelWidget = CreateWidget<UWeaponWheelWidget>(PlayerController, WeaponWheelClass);
	WeaponWheelWidget->InitializeWeaponWheelWidget(this);
}

void UCharacterEquipmentComponent::OnRep_ItemsArray()
{
	for (AEquipableItem* Item : ItemsArray)
		if (IsValid(Item))
			Item->UnEquip();
}

//текущее количество доступной амуниции
int32 UCharacterEquipmentComponent::GetAvailableAmunitionForCurrentWeapon()
{
	check(GetCurrentRangeWeapon());
	return AmunitionArray[(int32)GetCurrentRangeWeapon()->GetAmmoType()];
}

void UCharacterEquipmentComponent::AutoEquip()
{
	if (AutoEquipItemInSlot != EEquipmentSlots::None)
	{
		EquipItemInSlot(AutoEquipItemInSlot);
	}
}

void UCharacterEquipmentComponent::InitialiseCurrentObjects(EEquipmentSlots Slot)
{
	CurrentEquippedItem = ItemsArray[(uint32)Slot];
	CurrentEquippedWeapon = Cast<ARangeWeaponItem>(CurrentEquippedItem);
	CurrentThrowableItem = Cast<AThrowableItem>(CurrentEquippedItem);
	CurrentMeleeWeapon = Cast<AMeleeWeaponItem>(CurrentEquippedItem);
}

//разгрузка персонажа. Базовые предметы которые есть у персонажа
//работает не с одним свойством а с целым массивом
void UCharacterEquipmentComponent::CreateLoadout()
{
	//эктор должен спавниться только на сервере и с сервера передавать инфу
	//без этого на каждом инстансе будет по 3 персонажа суммарно получается 9
	if (GetOwner()->GetLocalRole() < ROLE_Authority)
		return;

	AmunitionArray.AddZeroed((uint32)EAmunitionType::MAX);
	for (const TPair<EAmunitionType, int32>& AmmoPair : MaxAmunitionAmount)
	{
		AmunitionArray[(uint32)AmmoPair.Key] = FMath::Max(AmmoPair.Value, 0);//защита от дурака, чтобы дизайнеры не вводили отрицательные значения
	}

	ItemsArray.AddZeroed((uint32)EEquipmentSlots::MAX);
	for (const TPair<EEquipmentSlots, TSubclassOf<AEquipableItem>>& ItemPair : ItemsLoadout)
	{
		if (!IsValid(ItemPair.Value))//проверка, заполнен ли
		{
			continue;
		}

		AddEquipmentItemToSlot(ItemPair.Value, (int32)ItemPair.Key);
	}
	InitialiseCurrentObjects(EEquipmentSlots::PrimaryItemSlot);
	if (IsValid(CurrentEquippedItem))
	{
		OnCurrentThrowableItemsAmountChanged(CurrentThrowableItem->GetMaxItemsAmount());
	}
}

void UCharacterEquipmentComponent::EquipAnimationFinished()
{
	bIsEquipping = false;
	AttachCurrentItemToEquippedSocket();
}

uint32 UCharacterEquipmentComponent::NextItemsArraySlotIndex(uint32 CurrentSlotIndex)
{
	if (CurrentSlotIndex == ItemsArray.Num() - 1)
	{
		return 0;
	}
	else
	{
		return CurrentSlotIndex + 1;
	}
}

uint32 UCharacterEquipmentComponent::PreviousItemsArraySlotIndex(uint32 CurrentSlotIndex)
{
	if (CurrentSlotIndex == 0)
	{
		return ItemsArray.Num() - 1;
	}
	else
	{
		return CurrentSlotIndex - 1;
	}
}



void UCharacterEquipmentComponent::ReloadAmmoInCurrentWeapon(int32 NumberOfAmmo, bool bCheckIsFull)
{
	int32 AvailableAmunition = GetAvailableAmunitionForCurrentWeapon();
	int32 CurrentAmmo;
	int32 AmmoToReload;
	if (IsAmmoChanged)
	{
		CurrentAmmo = CurrentEquippedWeapon->GetOptionalAmmo();
		AmmoToReload = CurrentEquippedWeapon->GetMaxOptionalAmmo() - CurrentEquippedWeapon->GetOptionalAmmo();
	}
	else
	{
		CurrentAmmo = CurrentEquippedWeapon->GetAmmo();
		AmmoToReload = CurrentEquippedWeapon->GetMaxAmmo() - CurrentEquippedWeapon->GetAmmo();
	}

	int32 ReloadedAmmo = FMath::Min(AvailableAmunition, AmmoToReload);
	if (NumberOfAmmo > 0)
	{
		ReloadedAmmo = FMath::Min(ReloadedAmmo, NumberOfAmmo);
	}

	AmunitionArray[(uint32)GetCurrentRangeWeapon()->GetAmmoType()] -= ReloadedAmmo;

	if (IsAmmoChanged)
	{
		CurrentEquippedWeapon->SetOptionalAmmo(ReloadedAmmo + CurrentAmmo);
	}
	else
	{
		CurrentEquippedWeapon->SetAmmo(ReloadedAmmo + CurrentAmmo);
	}

	if (bCheckIsFull)
	{
		AvailableAmunition = AmunitionArray[(uint32)CurrentEquippedWeapon->GetAmmoType()];
		bool bIsFullyReloaded = CurrentEquippedWeapon->GetAmmo() == CurrentEquippedWeapon->GetMaxAmmo();
		if (AvailableAmunition == 0 || bIsFullyReloaded)
		{
			CurrentEquippedWeapon->EndReload(true);
		}
	}
}

void UCharacterEquipmentComponent::ChangeThrowingStatus(bool Status)
{
	ThrowingStatus = Status;
}

void UCharacterEquipmentComponent::Multicast_PlayMontage_Implementation(UAnimMontage* EquipMontage)
{
	bIsEquipping = true;
	float EquipDuration = CachedBaseCharacter->PlayAnimMontage(EquipMontage);
	GetWorld()->GetTimerManager().SetTimer(EquipTimer, this, &UCharacterEquipmentComponent::EquipAnimationFinished, EquipDuration, false);
}

void UCharacterEquipmentComponent::OnWeaponReloadComplete()
{
	ReloadAmmoInCurrentWeapon(0, false);
}

void UCharacterEquipmentComponent::OnCurrentWeaponAmmoChanged(int32 Ammo)
{
	if (OnCurrentWeaponAmmoChangedEvent.IsBound())
	{
		OnCurrentWeaponAmmoChangedEvent.Broadcast(Ammo, GetAvailableAmunitionForCurrentWeapon());
	}
	if (GetCurrentRangeWeapon()->AvaliableOptionalAmmo && Ammo == 0)
	{
		ReloadCurrentWeapon();
	}
}

void UCharacterEquipmentComponent::OnCurrentThrowableItemsAmountChanged_Implementation(int32 Items)
{
	if (OnCurrentThrowableItemsAmountChangedEvent.IsBound())
	{
		OnCurrentThrowableItemsAmountChangedEvent.Broadcast(Items);
	}
}

void UCharacterEquipmentComponent::OnRep_CurrentEquippedSlot(EEquipmentSlots CurrentEquippedSlot_Old)
{
	//экипировка происходит только на клиенте в тот момент когда св-во меняет значения
	EquipItemInSlot(CurrentEquippedSlot);
}
