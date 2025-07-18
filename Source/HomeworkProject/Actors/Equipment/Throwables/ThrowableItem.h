// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Equipment/EquipableItem.h"
#include "UI/Widget/PlayerHUDWidget.h"
#include "Actors/Projectiles/GCProjectile.h"
#include "ThrowableItem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(ThrowableItemsAmountChanged, int32);

class AGCProjectile;
UCLASS(Blueprintable)
class HOMEWORKPROJECT_API AThrowableItem : public AEquipableItem
{
	GENERATED_BODY()

public:
	AThrowableItem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;

	//UFUNCTION(Server, Reliable)
	void Throw();

	ThrowableItemsAmountChanged OnThrowableItemsAmountChanged;

	int32 GetMaxItemsAmount() const;

	int32 GetItemsAmount() const;

	//UFUNCTION(Client, Reliable)
	void SetItemsAmount(int32 NewItems);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwables")
	TSubclassOf<AGCProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwables | Parametres | Items")
	int32 MaxItemsAmount = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Throwables", meta = (UIMin = -90.0f, UIMax = 90.0f, ClampMin = -90.0f, ClampMax = 90.0f))
	float ThrowableAngle = 0.0f;

private:
	UPROPERTY(Replicated)
	TArray<AGCProjectile*> GrenadePool;

	UPROPERTY(Replicated)
	int32 CurrentGrenadeIndex;

	const FVector GrenadePoolLocation = FVector(0.0f, 0.0f, -20.0f);

	UPROPERTY(Replicated)
	int32 Items;

	//UFUNCTION(Server, Reliable)
	void Server_LaunchProjectile(AGCProjectile* Projectile, FVector SpawnLocation, FVector LaunchDirection);
};