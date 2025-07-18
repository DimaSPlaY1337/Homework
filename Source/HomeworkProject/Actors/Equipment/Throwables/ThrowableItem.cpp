// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Equipment/Throwables/ThrowableItem.h"
#include "GCBaseCharacter.h"
#include "Net/UnrealNetwork.h"

AThrowableItem::AThrowableItem()
{
	SetReplicates(true);
}

void AThrowableItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AThrowableItem, GrenadePool);
	DOREPLIFETIME(AThrowableItem, CurrentGrenadeIndex);
	DOREPLIFETIME(AThrowableItem, Items);
}

void AThrowableItem::Throw()
{
	AGCBaseCharacter* CharacterOwner = GetCharacterOwner();
	if (!IsValid(CharacterOwner))
	{
		return;
	}

	FVector PlayerViewPoint;
	FRotator PlayerViewRotation;

	if (CharacterOwner->IsPlayerControlled())//позволяет определить, что контролируется ли наш персонаж игроком без необходимости получать его контроллер.
	{
		APlayerController* Controller = CharacterOwner->GetController<APlayerController>();
		if (!IsValid(Controller))
		{
			return;
		}
		Controller->GetPlayerViewPoint(PlayerViewPoint, PlayerViewRotation);
	}
	else
	{
		PlayerViewPoint = CharacterOwner->GetActorLocation();
		PlayerViewRotation = CharacterOwner->GetBaseAimRotation();//как раз для того, чтобы это значение было заполнено выставляли SetFocus.
	}

	FVector ViewDirection = PlayerViewRotation.RotateVector(FVector::ForwardVector);
	FVector ViewUpVector = PlayerViewRotation.RotateVector(FVector::UpVector);
	FTransform PlayerViewTransform(PlayerViewRotation, PlayerViewPoint);

	FVector LaunchDirection = ViewDirection + FMath::Tan(FMath::DegreesToRadians(ThrowableAngle)) * ViewUpVector;
	FVector ThrowableSocketLocation = CharacterOwner->GetMesh()->GetSocketLocation(SocketCharacterThrowable);
	//перевод из мировых координат в локальные
	FVector SocketInViewSpace = PlayerViewTransform.InverseTransformPosition(ThrowableSocketLocation);

	FVector SpawnLocation = PlayerViewPoint + ViewDirection * SocketInViewSpace.X;
	if (CurrentGrenadeIndex > MaxItemsAmount)
		CurrentGrenadeIndex = 0;
	AGCProjectile* Projectile = GrenadePool[CurrentGrenadeIndex];
	//достаём из массива GrenadePool
	if (IsValid(Projectile))
	{
		Server_LaunchProjectile(Projectile, SpawnLocation, LaunchDirection);
		++CurrentGrenadeIndex;
	}
}

int32 AThrowableItem::GetMaxItemsAmount() const
{
	return MaxItemsAmount;
}

int32 AThrowableItem::GetItemsAmount() const
{
	return Items;
}
//_Implementation
void AThrowableItem::SetItemsAmount(int32 NewItems)
{
	Items = NewItems;
	if (OnThrowableItemsAmountChanged.IsBound())
	{
		OnThrowableItemsAmountChanged.Broadcast(Items);
	}
}

void AThrowableItem::BeginPlay()
{
	Super::BeginPlay();
	SetItemsAmount(MaxItemsAmount);

	if (GetLocalRole() < ROLE_Authority)
		return;

	GrenadePool.Reserve(MaxItemsAmount);
	for (int32 i = 0; i < MaxItemsAmount; ++i)
	{
		AGCProjectile* Projectile = GetWorld()->SpawnActor<AGCProjectile>(ProjectileClass, GrenadePoolLocation, FRotator::ZeroRotator);
		APawn* PawnOwner = Cast<APawn>(GetOwner());
		Projectile->SetProjectileActive(false);
		Projectile->SetOwner(PawnOwner);
		GrenadePool.Add(Projectile);
	}
}
void AThrowableItem::Server_LaunchProjectile(AGCProjectile* Projectile, FVector SpawnLocation, FVector LaunchDirection)
{
	Projectile->SetActorLocation(SpawnLocation);
	Projectile->SetActorRotation(LaunchDirection.ToOrientationRotator());
	Projectile->SetProjectileActive(true);
	Projectile->SetOwner(GetOwner());
	Projectile->LaunchProjectile(LaunchDirection.GetSafeNormal());
}

