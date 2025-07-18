// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "HomeworkProject/GCBasePawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GCBaseCharacter.h"
#include "PlayerCharacter.h"
#include "Curves/CurveFloat.h"
#include "LedgeDetectorComponent.h"
#include "Curves/CurveVector.h"
#include "HomeworkProject/Ladder.h"
#include "Components/TimelineComponent.h"
#include "GCBaseCharacterMovementComponent.generated.h"

UENUM()
enum class EWallRunSide : uint8
{
	Left,
	Right,
	None
};

struct FMantlingMovementParameters
{
	FVector InitialLocation = FVector::ZeroVector;
	FRotator InitialRotation = FRotator::ZeroRotator;

	FVector TargetLocation = FVector::ZeroVector;
	FRotator TargetRotation = FRotator::ZeroRotator;

	FVector InitialAnimationLocation = FVector::ZeroVector;

	float Duration = 1.0f;
	float StartTime = 0.0f;

	UCurveVector* MantlingCurve;
};

UENUM(BlueprintType)
enum class ECustomMovementMode : uint8
{
	CMOVE_None = 0 UMETA(DisplayName = "None"),
	CMOVE_Mantling UMETA(DisplayName = "Mantling"),
	CMOVE_Ladder UMETA(DisplayName = "Ladder"),
	CMOVE_Zipline UMETA(DisplayName = "Zipline"),
	CMOVE_WallRun UMETA(DisplayName = "WallRun"),
	CMOVE_Max UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EDetachFromLadderMethod : uint8
{
	Fall = 0,
	ReachingTheTop,
	ReachingTheBottom,
	JumpOff
};
/**
 *
 */
UCLASS()
class HOMEWORKPROJECT_API UGCBaseCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	//означает что FSavedMove_GC будет иметь доступ ко всем полям класса
	friend class FSavedMove_GC;

public:
	UGCBaseCharacterMovementComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	//считывание из CompressedFlags куда мы записали значение
	virtual void UpdateFromCompressedFlags(uint8 Flags);

	virtual void PhysicsRotation(float DeltaTime) override;

	FORCEINLINE bool IsOutOfStamina() const { return bIsOutOfStamina; }
	void SetIsOutOfStamina(bool bIsOutOfStamina_In)
	{
		bIsOutOfStamina = bIsOutOfStamina_In;
		if (bIsOutOfStamina == true)
		{
			StopSprint();
			SprintSpeed = GetMaxSpeed();
			MaxWalkSpeed = GetMaxSpeed();
			JumpZVelocity = 0;
		}
		else {
			SprintSpeed = CachedSprintSpeed;
			MaxWalkSpeed = CachedMaxWalkSpeed;
			JumpZVelocity = CachedJumpZVelocity;
		}

	}
	FORCEINLINE bool IsProning() const { return bIsWantsToProne; }
	void SetIsProning(bool bIsWantsToProne_In)
	{
		bIsWantsToProne = bIsWantsToProne_In;
		if (bIsWantsToProne == true)
		{
			MaxWalkSpeed = MaxProneSpeed;
		}
		else
		{
			MaxWalkSpeed = CachedMaxWalkSpeed;
		}
	}
	void SetPlatformLocation(FVector loc)
	{
		OverlapLoc = loc;
	}
	FORCEINLINE bool IsSprinting() { return bIsSprinting; }
	FORCEINLINE bool IsSliding() { return bIsSliding; }
	virtual float GetMaxSpeed() const override;

	void StartSprint();
	void StopSprint();

	void StartProne();
	void StopProne();

	void StartMantle(const FMantlingMovementParameters& MantlingParametres);
	void EndMantle();
	bool IsMantling() const;
	bool IsEncroached();
	void AttachToLadder(const class ALadder* Ladder);
	void DetachFromLadder(EDetachFromLadderMethod DetachFromLadderMethod = EDetachFromLadderMethod::Fall);
	bool IsOnLadder() const;
	const class ALadder* GetCurrentLadder();
	float OutOfStaminaSpeed = 300.0f;
	int IsNextToPlatform = 0;
	FMantlingMovementParameters CurrentMantlingParameters;
	float GetCharacterToCurrentLadderProjection(const FVector& Location);
	float GetLadderSpeedRatio() const;

	void AttachToZipline(const class AZipline* zipline);
	void DetachFromZipline();

	bool IsOnZipline() const;

	void ZiplineTimelineUpdate(const float Alpha);

	bool IsWallRunning() const;

	bool IsSprinting() const;

	UFUNCTION()
	void OnPlayerCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void GetWallRunSideAndDirection(const FVector& HitNormal, EWallRunSide& OutSide, FVector& OutDirection);

	bool GetWallRunSide();

	void JumpFromWall();

	void StartSlide();

	UFUNCTION(NetMulticast, Reliable)
	void StopSlide();

	UPROPERTY(Category = "Character Movement: Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	UAnimMontage* SlideMontage;

	UPROPERTY(Category = "Character Movement: Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SlideMaxTime = 2.0f;

protected:

	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	void PhysWallRun(float DeltaSeconds, int32 iterations);

	void PhysMantling(float DeltaTime, int32 Iterations);

	void PhysLadder(float DeltaTime, int32 Iterations);

	void PhysZipline(float DeltaTime, int32 Iterations);

	void SetZiplineWay(FVector FirstPost, FVector SecondPost);

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	bool AreRequiredKeysDown(EWallRunSide side);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: sprint", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float SprintSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: prone", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float ProneCapsuleRadius = 40.0f;// Радуиус капсулы в состоянии лежа

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: prone", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxProneSpeed = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: prone", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float ProneCapsuleHalfHeight = 40.0f; // Половина высоты капсулы в состоянии лежа.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: Swimming", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float SwimmingCapsuleRaduis = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: Swimming", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float SwimmingCapsuleHalfHeight = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: Ladder", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float ClimbingOnLadderMaxSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: Ladder", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float ClimbingOnLadderBrakingDecelaretion = 2048.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: Ladder", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float LadderToCharacterOffset = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: Ladder", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxLedderTopOffset = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: Ladder", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MinLadderBottomOffset = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: Ladder", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float JumpOffFromLadderSpeed = 500.0f;

	class AGCBaseCharacter* GetBaseCharacterOwner() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character movement: Zipline")
	UCurveFloat* ZiplineCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character movement: Zipline")
	float ZiplineRollDownSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character: WallRun", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxWallRunTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character: wall run")
	float MaxWallRunSpeed = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character: wall run")
	float WallRunBrakingDeceleration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character: wall run")
	float JumpOffFromWallSpeed = 500.0f;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = "Character: wall run")
	UCurveFloat* VerticalSpeedForWallRun;

	UPROPERTY(Category = "Character Movement: Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SlideSpeed = 1000.0f;

	UPROPERTY(Category = "Character Movement: Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SlideCaspsuleHalfHeight = 60.0f;

private:
	UFUNCTION(Server, Reliable)
	void Server_Launch(const FVector SlideVelocity);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Launch(const FVector SlideVelocity);

	UPROPERTY(ReplicatedUsing=OnRep_IsSliding)
	bool bIsSliding = false;

	UFUNCTION(NetMulticast, Reliable)
	void DeltaHeight(float Delta);

	UFUNCTION()
	void OnRep_IsSliding(bool bWasSliding);

	float SlideDelta;

	FTimerHandle SlidingTimer;

	int ZiplineWay;
	FVector SecondPostLoc;
	FVector FirstPostLoc;
	FTimeline ZiplineTimeline;

	float CurrentTime;
	void AttachToWall(EWallRunSide Side, const FVector& Direction);
	void DetachFromWall();
	bool bIsWallRunning = false;
	EWallRunSide CurrentWallRunSide = EWallRunSide::None;
	FVector CurrentWallRunDirection = FVector::ZeroVector;
	bool IsSurfaceWallRunable(const FVector& SurfaceNormal) const;
	FTimerHandle WallRunTimer;
	FVector HitNrml;
	float TargetWallRunTime = 1.0f;

	bool bIsSprinting = false;
	bool bIsOutOfStamina = false;
	bool bIsWantsToProne;
	bool bEncroached;
	float CachedSprintSpeed = SprintSpeed;
	float CachedMaxWalkSpeed = MaxWalkSpeed;
	float CachedJumpZVelocity = JumpZVelocity;
	float CachedRadius;
	float SweepInflation;
	const ALadder* CurrentLadder = nullptr;
	UCapsuleComponent* CachedBaseCharacter;
	UCharacterMovementComponent* Component;
	USkeletalMeshComponent* Mesh;
	FVector BaseTranslationOffset;
	FVector StandingLocation;
	const ACharacter* DefaultCharacter;
	FVector PawnLocation;
	float CurrentCrouchedHalfHeight;
	float ComponentScale;
	float OldUnscaledHalfHeight;
	float HalfHeightAdjust;
	float ScaledHalfHeightAdjust;
	FCollisionShape StandingCapsuleShape;
	APlayerCharacter* pc;
	const AZipline* CurrentZipline = nullptr;
	FTimerHandle MantlingTimer;
	float TargetMantlingTime = 1.0f;
	FVector OverlapLoc;
	FVector CachedPlatformLoc;
	FRotator ForceTargetRotation = FRotator::ZeroRotator;
	bool bForceRotation = false;
};
//класс записывает информацию о всех действиях, которые были сделаны во время перемещения на клиенте и отсылает ее на сервер как общую информацию о перемещении.
// Сервер в свою очередь отсылает эту информацию на другие клиенты.
class FSavedMove_GC : public FSavedMove_Character
{
	//делаем так чтобы имя слева можно было использовать ввиде имени справа
	typedef FSavedMove_Character Super;

public:

	//очищает все выставленные значения, то есть очищает структуру.
	virtual void Clear() override;

	//возвращает все флаги, что у нас закодированы в FSavedMove_GC 
	virtual uint8 GetCompressedFlags() const;

	//(про оптимизацию)говорит о том, что мы можем скомбинировать наше перемещение с каким то предыдущим, которое у нас есть
	//FSavedMovePtr - структура обертка над указателями типа SavedMove
	//Character - персонаж для которого все это происходит
	//MaxDelta - различие по времени между тем как оно все произошло
	virtual bool CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* InCharacter, float MaxDelta) const override;

	//метод который позволяет записать значение в данную структуру
	virtual void SetMoveFor(ACharacter* InCharacter, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;

	//читает из данной структуры
	virtual void PrepMoveFor(ACharacter* Character) override;

private:
	//признак того что у нас активирован или деактивирован спринт
	//: 1 - используем 1 байт из 8 битового целого
	//используем для удобной упаковки структуры данных
	//можно 8 булевых значений в одном байте закодировать, они будут выровняны как по 8 битовому числу 
	//и структура будет занимать 8 бит.
	uint8 bSavedIsSprinting : 1;
	uint8 bSavedIsMantling : 1;
	uint8 bSavedIsPressingSlide : 1;
};
//благодаря этому классу мы сможем создавать SavedMove и им сможет пользоваться CharacterMovementComponent
class FNetworkPredictionData_Client_Character_GC : public FNetworkPredictionData_Client_Character
{
	typedef FNetworkPredictionData_Client_Character Super;

public:
	FNetworkPredictionData_Client_Character_GC(const UCharacterMovementComponent& ClientMovement);

	//создаёт новое перемещение
	virtual FSavedMovePtr AllocateNewMove() override;
};