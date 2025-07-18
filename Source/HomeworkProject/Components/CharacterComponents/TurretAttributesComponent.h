// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurretAttributesComponent.generated.h"

class ATurret;
DECLARE_MULTICAST_DELEGATE(FOnDeathEventSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExp);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOMEWORKPROJECT_API UTurretAttributesComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTurretAttributesComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void Explode();

	UPROPERTY(BlueprintAssignable)
	FOnExp OnExplosion;

	FOnDeathEventSignature OnDeathEvent;

	bool IsAlive() { return Health > 0.0f; }

	float GetHealthPercent() const;

	int Shots = 0;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (UIMin = 0.0f))
	float MaxHealth = 100.0f;

private:
	float Health = 0.0f;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	void DebugDrawAttributes();//отрисовка здоровья 
#endif

	UFUNCTION()
	void OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	TWeakObjectPtr<class ATurret> CachedTurretOwner;//указатель на владельца(применяем только на персонажа)
};
