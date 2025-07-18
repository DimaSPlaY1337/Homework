// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Curves/CurveVector.h"
#include "Actors/Equipment/Throwables/ThrowableItem.h"
#include "Animation/AnimMontage.h"
#include "GenericTeamAgentInterface.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Components/TimelineComponent.h"
#include "UObject/ScriptInterface.h"
#include "Inventory/Items/InventoryItem.h"
#include "Components/CharacterComponents/CharacterInventoryComponent.h"
#include "Subsystems/SaveSubsystem/SaveSubsystemInterface.h"
#include "GCBaseCharacter.generated.h"

DECLARE_DELEGATE_OneParam(FOnInteractableObjectFound, FName);

class IInteractable;
class UWidgetComponent;

USTRUCT(BlueprintType)
struct FMantlingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UAnimMontage* MantlingMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UAnimMontage* FPMantlingMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UCurveVector* MantlingCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AnimationCorrectionXY = 65.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AnimationCorrectionZ = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxHeight = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MinHeight = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxHeightStartTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MinHeightStartTime = 0.5f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAimingStateChanged, bool)
DECLARE_MULTICAST_DELEGATE(FHealthPercentEvent)
DECLARE_MULTICAST_DELEGATE_OneParam(FOxygenPercentEvent, ESlateVisibility)
DECLARE_MULTICAST_DELEGATE_OneParam(FStaminaPercentEvent, ESlateVisibility)
class AInteractiveActor;
class CharacterEquipmentComponent;
class UCharacterAttributesComponent;
class UGCBaseCharacterMovementComponent;
UCLASS(Abstract, NotBlueprintable)
class HOMEWORKPROJECT_API AGCBaseCharacter : public ACharacter, public IGenericTeamAgentInterface, public ISaveSubsystemInterface
{
	GENERATED_BODY()

public:
	AGCBaseCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	virtual void PossessedBy(AController* NewController) override;

	// ISaveSubsystemInterface
	virtual void OnLevelDeserialized_Implementation() override;
	// ISaveSubsystemInterface

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void MoveForward(float value) {};
	virtual void MoveRight(float value) {};
	virtual void Turn(float value) {};
	virtual void LookUp(float value) {};
	virtual void TurnAtRate(float value) {};
	virtual void LookUpAtRate(float value) {};
	virtual void ChangeCrouchState();
	virtual void ChangeProneState();
	virtual void TryToJump();
	virtual void StartSprint();
	virtual void StopSprint();
	virtual void SwimForward(float Value) {};
	virtual void SwimRight(float Value) {};
	virtual void SwimUp(float Value) {};
	virtual void Tick(float DeltaTime) override;
	virtual bool CanJumpInternal_Implementation() const override;

	UFUNCTION(BlueprintCallable)
	void Mantle(bool bForce = false);

	UPROPERTY(ReplicatedUsing = OnRep_IsMantling)
	bool bIsMantling;

	UFUNCTION()
	void OnRep_IsMantling(bool bWasMantling);

	void ClimbLadderUp(float Value);
	void InteractWithLadder();
	void InteractWithZipline();
	void Sliding(bool bForce);
	const class ALadder* GetAvailableLadder() const;
	const class AZipline* GetAvailableZipline() const;
	virtual void Prone();
	virtual void UnProne();
	void SetPlatformLocation(FVector loc)
	{
		Platform = loc;
	}
	void StartFire();
	void StopFire();

	void StartAiming();
	void StopAiming();

	FRotator GetAimOffset();

	bool IsAiming() const;
	void Reload();

	void NextItem();
	void PreviousItem();

	void EquipPrimaryItem();

	void ChangeFireMode();

	FOnAimingStateChanged OnAimingStateChanged;
	FHealthPercentEvent HealthPercentEvent;
	FOxygenPercentEvent OxygenPercentEvent;
	FStaminaPercentEvent StaminaPercentEvent;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character")
		void OnStartAiming();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character")
		void OnStopAiming();

	float GetAimingMovementSpeed() const;

	FORCEINLINE UGCBaseCharacterMovementComponent* GetBaseCharacterMovementComponent() const{ return GCBaseCharacterMovementComponent; }

	const UCharacterEquipmentComponent* GetCharacterEquipmentComponent() const;

	UCharacterEquipmentComponent* GetCharacterEquipmentComponent_Mutable() const;

	const UCharacterAttributesComponent* GetCharacterAttributesComponent() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE	float GetIKRightFootOffset() const { return IKRightFootOffset; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE	float GetIKLeftFootOffset() const { return IKLeftFootOffset; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE	float GetIKPelvisOffset() const { return IKPelvisOffset; }

	void RegisterInteractiveActor(AInteractiveActor* InteractiveActor);
	void UnregisterInteractiveActor(AInteractiveActor* InteractiveActor);

	virtual void Falling() override;
	virtual void NotifyJumpApex() override;
	virtual void Landed(const FHitResult& Hit) override;

	void PrimaryMeleeAttack();
	void SecondaryMeleeAttack();

	virtual FGenericTeamId GetGenericTeamId() const override;

	void Interact();

	FOnInteractableObjectFound OnInteractableObjectFound;

	//метод дл€ инициализации виджета здоровь€
	void InitializeHealthProgress();

	void AddEquipmentItem(const TSubclassOf<AEquipableItem> EquipableItemClass);

	bool PickupItem(TWeakObjectPtr<UInventoryItem> ItemToPickup);

	void UseInventory(APlayerController* PlayerController);

	void AddHealth(float Health);

	void RestoreFullStamina();

	void ConfirmWeaponSelection();

protected:

	UFUNCTION(BlueprintImplementableEvent, Category = "Character | Movement")
	void OnSprintStart();

	virtual void OnSprintStart_Implementation();

	UFUNCTION(BlueprintImplementableEvent, Category = "Character | Movement")
	void OnSprintEnd();

	virtual void OnSprintEnd_Implementation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Controls")
	float BaseTurnRate = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Controls")
	float BaseLookUpRate = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling")
	FMantlingSettings HighMantleSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling")
	FMantlingSettings LowMantleSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float LowMantleMaxHeight= 125.0f;

	virtual bool CanSprint();

	virtual bool CanProne();

	float CalculatePelvisOffset();

	bool CanMantle() const;

	virtual void OnMantle(const FMantlingSettings& MantlingSettings, float MantlingAnimationStartTime);

	UGCBaseCharacterMovementComponent* GCBaseCharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Movement")
	class ULedgeDetectorComponent* LedgeDetectorComponent;

	//Homework
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	class USkeletalMeshComponent* SkeletalMeshComponent;

	//—океты
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|IK settings")
	FName RightFootSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|IK settings")
	FName LeftFootSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|IK settings", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float IKTraceExtendDistance = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|IK settings", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float IKInterpSpeed = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
	UCharacterAttributesComponent* CharacterAttributesComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
	UCharacterInventoryComponent* CharacterInventoryComponent;

	virtual void OnDeath();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Animations")
		class UAnimMontage* OnDeathAnimMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Attributes")
		class UCurveFloat* FallDamageCurve;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
		class UCharacterEquipmentComponent* CharacterEquipmentComponent;

	virtual void OnStartAimingInternal();

	virtual void OnStopAimingInternal();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Team")
		ETeams Team = ETeams::Enemy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Team")
	float LineOfSightDistance = 500.0f;//длина трейса

	void TraceLineOfSight();//метод выполн€ющий поиск по линии взгл€да
	//результат того что мы нашли сохран€ем в указателе
	//TScriptInterface - специальный класс предназначенный дл€ хранени€ указател€ на интерфейс.
	UPROPERTY()
	TScriptInterface<IInteractable> LineOfSightObject;

	//VisibleDefaultsOnly - будем видеть только на дефолтных объектах
	//UWidgetComponent - позвол€ет прив€зывать определЄнные виджеты к некоторым объектам в мире
	UPROPERTY(VisibleDefaultsOnly, Category = "Character | Components")
	UWidgetComponent* HealthBarProgressComponent;

private:
	void TryChangeSprintState(float DeltaTime);

	bool bIsSprintRequested = false;
	bool bIsAiming;

	const FMantlingSettings& GetMantlingSettings(float LedgeHeight) const;

	float GetIKOffsetForASocket(const FName& SocketName);
	float IKRightFootOffset = 0.0f;
	float IKLeftFootOffset = 0.0f;
	float IKPelvisOffset = 0.0f;

	float IKTraceDistance = 0.0f;
	float IKScale = 0.0f;

	float CurrentAimingMovementSpeed;

	FVector Platform;
	TArray<AInteractiveActor*> AvailableInteractiveActors;

	void EnableRagdoll();

	FVector CurrentFallApex;
};
