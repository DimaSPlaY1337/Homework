// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GCBaseCharacterMovementComponent.h"
#include "Subsystems/SaveSubsystem/SaveSubsystemInterface.h"
#include "CharacterAttributesComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnDeathEventSignature);
DECLARE_MULTICAST_DELEGATE_OneParam(FOutOfStaminaEventSignature, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOMEWORKPROJECT_API UCharacterAttributesComponent : public UActorComponent, public ISaveSubsystemInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCharacterAttributesComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE UGCBaseCharacterMovementComponent* GetBaseCharacterMovementComponent() const { return GCBaseCharacterMovementComponent; }

	FOnDeathEventSignature OnDeathEvent;

	FOutOfStaminaEventSignature OutOfStamina;

	FOnHealthChanged OnHealthChangedEvent;

	bool IsAlive() { return Health > 0.0f; }

	float GetHealthPercent() const;

	float GetOxygenPercent() const;

	float GetStaminaPercent() const;

	void AddHealth(float HealthToAdd);

	void RestoreFullStamina();

	virtual void OnLevelDeserialized_Implementation() override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
		
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (UIMin = 0.0f))
		float MaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = 0.0f, UIMin = 0.0f))
		float MaxStamina; // ћаксимальное значение выносливости игрока

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = 0.0f, UIMin = 0.0f))
		float StaminaRestoreVelocity; // —корость восстановлени€ выносливости

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = 0.0f, UIMin = 0.0f))
		float SprintStaminaConsumptionVelocity; // —корость потреблени€ выносливости игрока

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = 0.0f, UIMin = 0.0f))
		float MaxOxygen = 50.0f; // ћаксимальный запас кислорода

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = 0.0f, UIMin = 0.0f))
		float OxygenRestoreVelocity = 15.0f; // —корость восстановлени€ кислорода в секунду

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = 0.0f, UIMin = 0.0f))
		float SwimOxygenConsumptionVelocity = 2.0f; // —корость потреблени€ кислорода во врем€ плавани€ под водой в секунду

private:
	UPROPERTY(ReplicatedUsing = OnRep_Health, SaveGame)
	float Health = 0.0f;

	float CurrentStamina;
	float Oxygen;
	float CurrentOxygen;

	UFUNCTION()
	void OnRep_Health();

	void OnHealthChanged();

	UGCBaseCharacterMovementComponent* GCBaseCharacterMovementComponent;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	void DebugDrawAttributes();//отрисовка здоровь€ 
#endif

	UFUNCTION()
		void OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);
		
	void UpdateStaminaValue(float DeltaTime);
	void UpdateOxygenValue(float DeltaTime);
	bool IsSwimmingUnderWater(float HeadLocation, float VolumeLocation) const;

	TWeakObjectPtr<class AGCBaseCharacter> CachedBaseCharacterOwner;//указатель на владельца(примен€ем только на персонажа)
};
