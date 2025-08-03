// Fill out your copyright notice in the Description page of Project Settings.


#include "GCBaseCharacter.h"
#include "Zipline.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GCBaseCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "LedgeDetectorComponent.h"
#include "Actors/Equipment/Weapons/MeleeWeaponItem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "CharacterAttributesComponent.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"
#include "GCBaseCharacterAnimInstance.h"
#include "GameCodeTypes.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "AI/Controllers/AITurretController.h"
#include "Actors/Environment/PlatformTrigger.h"
#include "Net/UnrealNetwork.h"
#include "Actors/Interactive/Interface/Interactive.h"
#include "Components/WidgetComponent.h"
#include "UI/Widget/World/GCAttributeProgressBar.h"
#include "C:/MyProject/ue4/UE_4.26/Engine/Plugins/Runtime/SignificanceManager/Source/SignificanceManager/Public/SignificanceManager.h"


AGCBaseCharacter::AGCBaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGCBaseCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	GCBaseCharacterMovementComponent = StaticCast<UGCBaseCharacterMovementComponent*>(GetCharacterMovement());

	IKScale = GetActorScale3D().Z;
	IKTraceDistance = 200 * IKScale;

	LedgeDetectorComponent = CreateDefaultSubobject<ULedgeDetectorComponent>(TEXT("LedgeDetector"));

	GetMesh()->CastShadow = true;
	GetMesh()->bCastDynamicShadow = true;

	CharacterAttributesComponent = CreateDefaultSubobject<UCharacterAttributesComponent>(TEXT("CharacterAttributes"));
	CharacterEquipmentComponent = CreateDefaultSubobject<UCharacterEquipmentComponent>(TEXT("CharacterEquipment"));
	CharacterInventoryComponent = CreateDefaultSubobject<UCharacterInventoryComponent>(TEXT("CharacterInventory"));

	HealthBarProgressComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarProgressComponent"));
	HealthBarProgressComponent->SetupAttachment(GetCapsuleComponent());
}

void AGCBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCapsuleComponent()->OnComponentHit.AddDynamic(GCBaseCharacterMovementComponent, &UGCBaseCharacterMovementComponent::OnPlayerCapsuleHit);

	CharacterAttributesComponent->OnDeathEvent.AddUObject(this, &AGCBaseCharacter::OnDeath);
	CharacterAttributesComponent->OutOfStamina.AddUObject(GetBaseCharacterMovementComponent(), &UGCBaseCharacterMovementComponent::SetIsOutOfStamina);

	InitializeHealthProgress();

	if (bIsSignificantEnabled)
	{
		USignificanceManager* SignificanceManager = FSignificanceManagerModule::Get(GetWorld());
		if (IsValid(SignificanceManager))
		{
			SignificanceManager->RegisterObject(
				this,//указатель на объект, который мы регистрируем
				SignificanceTagCharacter,//тэг позвол€ющий индентифицировать объект среди всего
				//«апоминает функцию вычислени€ значимости Ч вы передаЄте функцию, 
				// рассчитывающую float-значение значимости объекта в зависимости 
				// от его свойств и положени€ относительно текущих точек обзора (ViewPoint).
				[this](USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& ViewPoint) -> float//возвр float
				{
					return SignificanceFunction(ObjectInfo, ViewPoint);
				},
				//“ип пост-обработки (EPostSignificanceType): Sequential Ч функции вызываютс€ по пор€дку, 
				// что не требует потокобезопасности; Concurrent Ч могут работать параллельно
				USignificanceManager::EPostSignificanceType::Sequential,
				//«апоминает функцию пост-обработки Ч эту функцию Significance Manager вызывает 
				// после очередной оценки значимости, чтобы, например, изменить состо€ние или 
				// детализацию объекта.
				[this](USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
				{
					PostSignificanceFunction(ObjectInfo, OldSignificance, Significance, bFinal);
				}
			);
		}
	}
}

void AGCBaseCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (OnInteractableObjectFound.IsBound())
	{
		OnInteractableObjectFound.Unbind();
	}
	Super::EndPlay(Reason);
}

void AGCBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AAIController* AIController = Cast<AAIController>(NewController);
	if (IsValid(AIController))
	{
		FGenericTeamId TeamId((uint8)Team);
		AIController->SetGenericTeamId(TeamId);
	}
}

void AGCBaseCharacter::OnLevelDeserialized_Implementation()
{

}

void AGCBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGCBaseCharacter, bIsMantling);
}

void AGCBaseCharacter::ChangeCrouchState()
{
	if (GetCharacterMovement()->IsCrouching() && !GCBaseCharacterMovementComponent->IsProning())
	{
		UnCrouch();
	}
	else if (!bIsSprintRequested && !GCBaseCharacterMovementComponent->IsSwimming() && !GCBaseCharacterMovementComponent->IsSliding())
	{
		Crouch();
		if (GCBaseCharacterMovementComponent->IsProning())
		{
			ChangeProneState();
		}
	}
}

void AGCBaseCharacter::ChangeProneState()
{
	if (GCBaseCharacterMovementComponent->IsCrouching() && !GCBaseCharacterMovementComponent->IsProning())
	{
		Prone();
	}
	else if (GCBaseCharacterMovementComponent->IsProning() && !GCBaseCharacterMovementComponent->IsEncroached())
	{
		UnProne();
	}
}

void AGCBaseCharacter::TryToJump()
{
	if (!GCBaseCharacterMovementComponent->IsEncroached() && !GCBaseCharacterMovementComponent->IsProning() && !GCBaseCharacterMovementComponent->IsSwimming() && !GCBaseCharacterMovementComponent->IsCrouching())
	{
		Jump();
	}
	else if (!GCBaseCharacterMovementComponent->IsProning() && GCBaseCharacterMovementComponent->IsCrouching() && !GCBaseCharacterMovementComponent->IsEncroached() && !GCBaseCharacterMovementComponent->IsSwimming())
	{
		ChangeCrouchState();
	}
	else if ((GCBaseCharacterMovementComponent->IsProning() || GCBaseCharacterMovementComponent->IsCrouching()) && !GCBaseCharacterMovementComponent->IsEncroached() && !GCBaseCharacterMovementComponent->IsSwimming())
	{
		ChangeProneState();
		ChangeCrouchState();
	}
}

void AGCBaseCharacter::StartSprint()
{
	bIsSprintRequested = true;
	if (bIsCrouched && !GCBaseCharacterMovementComponent->IsProning())
	{
		UnCrouch();
	}
}

void AGCBaseCharacter::StopSprint()
{
	bIsSprintRequested = false;
}

void AGCBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TryChangeSprintState(DeltaTime);
	IKLeftFootOffset = FMath::FInterpTo(IKLeftFootOffset, GetIKOffsetForASocket(LeftFootSocketName), DeltaTime, IKInterpSpeed);
	IKRightFootOffset = FMath::FInterpTo(IKRightFootOffset, GetIKOffsetForASocket(RightFootSocketName), DeltaTime, IKInterpSpeed);
	IKPelvisOffset = FMath::FInterpTo(IKPelvisOffset, CalculatePelvisOffset(), DeltaTime, IKInterpSpeed);
	if (HealthPercentEvent.IsBound())
	{
		HealthPercentEvent.Broadcast();
	}
	TraceLineOfSight();
}

bool AGCBaseCharacter::CanJumpInternal_Implementation() const
{
	return Super::CanJumpInternal_Implementation() && !GetBaseCharacterMovementComponent()->IsMantling();
}

void AGCBaseCharacter::Mantle(bool bForce /*= false*/)
{
	if (!(CanMantle() || bForce))
	{
		return;
	}

	if (GCBaseCharacterMovementComponent->IsProning())
	{
		ChangeProneState();
	}
	if (GCBaseCharacterMovementComponent->IsCrouching())
	{
		ChangeCrouchState();
	}
	FledgeDescription LedgeDescription;
	if (LedgeDetectorComponent->DetectLedge(LedgeDescription) && !GCBaseCharacterMovementComponent->IsMantling())
	{
		bIsMantling = true;

		FMantlingMovementParameters MantlingParamters;
		MantlingParamters.InitialRotation = GetActorRotation();
		MantlingParamters.TargetRotation = LedgeDescription.Rotation;
		MantlingParamters.TargetLocation = LedgeDescription.Location;
		MantlingParamters.InitialLocation = GetActorLocation();
		float MantlingHeight = (MantlingParamters.TargetLocation - MantlingParamters.InitialLocation).Z;
		float LedgeHeight = MantlingHeight - LedgeDetectorComponent->CharacterBottom.Z;
		const FMantlingSettings& MantlingSettings = GetMantlingSettings(LedgeHeight);

		float MinRange;
		float MaxRange;
		MantlingSettings.MantlingCurve->GetTimeRange(MinRange, MaxRange);

		MantlingParamters.Duration = MaxRange - MinRange;

		MantlingParamters.MantlingCurve = MantlingSettings.MantlingCurve;
		//интервалы высоты и времени, высота преп€тстви€
		FVector2D SourceRange(MantlingSettings.MinHeight, MantlingSettings.MaxHeight);
		FVector2D TargetRange(MantlingSettings.MinHeightStartTime, MantlingSettings.MaxHeightStartTime);
		MantlingParamters.StartTime = FMath::GetMappedRangeValueClamped(SourceRange, TargetRange, MantlingHeight);

		MantlingParamters.InitialAnimationLocation = MantlingParamters.TargetLocation - MantlingSettings.AnimationCorrectionZ * FVector::UpVector + MantlingSettings.AnimationCorrectionXY * LedgeDescription.LedgeNormal;

		//сделано с целью чтобы несколько раз не запускать mantle
		if (IsLocallyControlled() || GetLocalRole() == ROLE_Authority)
			GCBaseCharacterMovementComponent->StartMantle(MantlingParamters);

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		AnimInstance->Montage_Play(MantlingSettings.MantlingMontage, 1.0f, EMontagePlayReturnType::Duration, MantlingParamters.StartTime);
		OnMantle(MantlingSettings, MantlingParamters.StartTime);
	}
}

void AGCBaseCharacter::RegisterInteractiveActor(AInteractiveActor* InteractiveActor)
{
	AvailableInteractiveActors.AddUnique(InteractiveActor);
}

void AGCBaseCharacter::UnregisterInteractiveActor(AInteractiveActor* InteractiveActor)
{
	AvailableInteractiveActors.RemoveSingleSwap(InteractiveActor);
}

void AGCBaseCharacter::Falling()
{
	GetBaseCharacterMovementComponent()->bNotifyApex = true;
}

void AGCBaseCharacter::NotifyJumpApex()
{
	Super::NotifyJumpApex();
	CurrentFallApex = GetActorLocation();
}

void AGCBaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	float FallHeight = (CurrentFallApex - GetActorLocation()).Z * 0.01f;
	if (IsValid(FallDamageCurve))
	{
		float DamageAmount = FallDamageCurve->GetFloatValue(FallHeight);
		TakeDamage(DamageAmount, FDamageEvent(), GetController(), Hit.Actor.Get());
	}
}

void AGCBaseCharacter::PrimaryMeleeAttack()
{
	AMeleeWeaponItem* CurrentMeleeWeapon = CharacterEquipmentComponent->GetCurrentMeleeWeapon();
	if (IsValid(CurrentMeleeWeapon))
	{
		CurrentMeleeWeapon->StartAttack(EMeleeAttackTypes::PrimaryAttack);
	}
}

void AGCBaseCharacter::SecondaryMeleeAttack()
{
	AMeleeWeaponItem* CurrentMeleeWeapon = CharacterEquipmentComponent->GetCurrentMeleeWeapon();
	if (IsValid(CurrentMeleeWeapon))
	{
		CurrentMeleeWeapon->StartAttack(EMeleeAttackTypes::SecondaryAttack);
	}
}

FGenericTeamId AGCBaseCharacter::GetGenericTeamId() const
{
	return FGenericTeamId((uint8)Team);
}

void AGCBaseCharacter::Interact()
{
	if (LineOfSightObject.GetInterface())
		LineOfSightObject->Interact(this);
}

void AGCBaseCharacter::InitializeHealthProgress()
{
	UGCAttributeProgressBar* Widget = Cast<UGCAttributeProgressBar>(HealthBarProgressComponent->GetUserWidgetObject());
	if (!IsValid(Widget))
	{
		HealthBarProgressComponent->SetVisibility(false);
		return;
	}

	if (IsPlayerControlled() && IsLocallyControlled())
	{
		HealthBarProgressComponent->SetVisibility(false);
	}

	CharacterAttributesComponent->OnHealthChangedEvent.AddUObject(Widget, &UGCAttributeProgressBar::SetProgressPercantage);
	//≈сли надо получить все внешние переменные из области, где определено л€мбда-выражение, по значению, то в квадратных скобках указываетс€ символ "равно" =. 
	CharacterAttributesComponent->OnDeathEvent.AddLambda([=]() {HealthBarProgressComponent->SetVisibility(false); });
	Widget->SetProgressPercantage(CharacterAttributesComponent->GetHealthPercent());
}


bool AGCBaseCharacter::PickupItem(TWeakObjectPtr<UInventoryItem> ItemToPickup)
{
	bool Result = false;
	if (CharacterInventoryComponent->HasFreeSlot())
	{
		CharacterInventoryComponent->AddItem(ItemToPickup, 1);
		Result = true;
	}
		
	return Result;
}

void AGCBaseCharacter::UseInventory(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
		return;

	if (!CharacterInventoryComponent->IsViewVisible())
	{
		CharacterInventoryComponent->OpenViewInventory(PlayerController);
		CharacterEquipmentComponent->OpenViewEquipment(PlayerController);
		PlayerController->SetInputMode(FInputModeGameAndUI{});
		PlayerController->bShowMouseCursor = true;
	}
	else
	{
		CharacterInventoryComponent->CloseViewInventory();
		CharacterEquipmentComponent->CloseViewEquipment(/*PlayerController*/);
		PlayerController->SetInputMode(FInputModeGameOnly{});
		PlayerController->bShowMouseCursor = false;
	}
}

void AGCBaseCharacter::AddHealth(float Health)
{
	CharacterAttributesComponent->AddHealth(Health);
}

void AGCBaseCharacter::RestoreFullStamina()
{
	CharacterAttributesComponent->RestoreFullStamina();
}

void AGCBaseCharacter::ConfirmWeaponSelection()
{
	if (CharacterEquipmentComponent->IsSelectingWeapon())
	{
		CharacterEquipmentComponent->ConfirmWeaponSelection();
	}
}

void AGCBaseCharacter::OnRep_IsMantling(bool bWasMantling)
{
	if (GetLocalRole() == ROLE_SimulatedProxy && !bWasMantling && bIsMantling)
	{
		Mantle(true);
	}

}

void AGCBaseCharacter::ClimbLadderUp(float Value)
{
	if (GetBaseCharacterMovementComponent()->IsOnLadder() && !FMath::IsNearlyZero(Value))
	{
		FVector LadderUpVector = GetBaseCharacterMovementComponent()->GetCurrentLadder()->GetActorUpVector();
		AddMovementInput(LadderUpVector, Value);
	}
}

void AGCBaseCharacter::InteractWithLadder()
{
	if (GetBaseCharacterMovementComponent()->IsOnLadder())
	{
		GetBaseCharacterMovementComponent()->DetachFromLadder(EDetachFromLadderMethod::JumpOff);
	}
	else
	{
		const ALadder* AvailableLadder = GetAvailableLadder();
		if (IsValid(AvailableLadder))
		{
			if (AvailableLadder->GetIsOnTop())
			{
				PlayAnimMontage(AvailableLadder->GetAttachFromTopAnimMontage());
			}
			GetBaseCharacterMovementComponent()->AttachToLadder(AvailableLadder);
		}
	}
}

void AGCBaseCharacter::InteractWithZipline()
{
	if (GetBaseCharacterMovementComponent()->IsOnZipline())
	{
		GetBaseCharacterMovementComponent()->DetachFromZipline();
	}
	else
	{
		const AZipline* AvailableZipline = GetAvailableZipline();
		if (IsValid(AvailableZipline))
		{
			GetBaseCharacterMovementComponent()->AttachToZipline(AvailableZipline);
		}
	}
}

void AGCBaseCharacter::Sliding(bool bForce)
{
	if ((GCBaseCharacterMovementComponent->IsSprinting() && !GCBaseCharacterMovementComponent->IsSliding()) || bForce)
	{
		GCBaseCharacterMovementComponent->StartSlide();
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		AnimInstance->Montage_Play(GCBaseCharacterMovementComponent->SlideMontage, GCBaseCharacterMovementComponent->SlideMaxTime, EMontagePlayReturnType::Duration, 0.0f);
	}
}

const ALadder* AGCBaseCharacter::GetAvailableLadder() const
{
	const ALadder* Result = nullptr;
	for (const AInteractiveActor* InteractiveActor : AvailableInteractiveActors)
	{
		if (InteractiveActor->IsA<ALadder>())
		{
			Result = StaticCast<const ALadder*>(InteractiveActor);
			break;
		}
	}
	return Result;
}

const AZipline* AGCBaseCharacter::GetAvailableZipline() const
{
	const AZipline* Result = nullptr;
	for (const AInteractiveActor* InteractiveActor : AvailableInteractiveActors)
	{
		if (InteractiveActor->IsA<AZipline>())
		{
			Result = StaticCast<const AZipline*>(InteractiveActor);
			break;
		}
	}
	return Result;
}

void AGCBaseCharacter::OnSprintStart_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("AGCBaseCharacter::OnSprintStart_Implementation()"));
}

void AGCBaseCharacter::OnSprintEnd_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("AGCBaseCharacter::OnSprintStart_Implementation()"));
}

void AGCBaseCharacter::StartFire()
{
	if (CharacterEquipmentComponent->IsSelectingWeapon())
		return;

	if (CharacterEquipmentComponent->IsEquipping())
	{
		return;
	}
	ARangeWeaponItem* CurrentRangeWeapon = CharacterEquipmentComponent->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StartFire();
	}
}

void AGCBaseCharacter::StopFire()
{
	ARangeWeaponItem* CurrentRangeWeapon = CharacterEquipmentComponent->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopFire();
	}
}

void AGCBaseCharacter::StartAiming()
{
	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();
	if (!IsValid(CurrentRangeWeapon))
	{
		return;
	}

	bIsAiming = true;
	CurrentAimingMovementSpeed = CurrentRangeWeapon->GetAimMovementMaxSpeed();
	CurrentRangeWeapon->StartAim();
	OnStartAiming();
}

void AGCBaseCharacter::StopAiming()
{
	if (!bIsAiming)
	{
		return;
	}

	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopAim();
	}
	bIsAiming = false;
	CurrentAimingMovementSpeed = 0.0f;
	OnStopAiming();
}

//возвращает текущее значение поворота в локальной системе координат
FRotator AGCBaseCharacter::GetAimOffset()
{
	//Vector() тоже самое что GetForwardVector()
	FVector AimDirectionWorld = GetBaseAimRotation().Vector();
	FVector AimDirectionLocal = GetTransform().InverseTransformVectorNoScale(AimDirectionWorld);
	FRotator Result = AimDirectionLocal.ToOrientationRotator();

	return Result;
}

bool AGCBaseCharacter::IsAiming() const
{
	return bIsAiming;
}

void AGCBaseCharacter::Reload()
{
	if (IsValid(CharacterEquipmentComponent->GetCurrentRangeWeapon()))
	{
		CharacterEquipmentComponent->ReloadCurrentWeapon();
	}
}

void AGCBaseCharacter::NextItem()
{
	CharacterEquipmentComponent->EquipNextItem();
}

void AGCBaseCharacter::PreviousItem()
{
	CharacterEquipmentComponent->EquipPreviousItem();
}

void AGCBaseCharacter::EquipPrimaryItem()
{
	CharacterEquipmentComponent->EquipItemInSlot(EEquipmentSlots::PrimaryItemSlot);
}

void AGCBaseCharacter::ChangeFireMode()
{
	CharacterEquipmentComponent->ChangeFireMode();
}

void AGCBaseCharacter::OnStartAiming_Implementation()
{
	OnStartAimingInternal();
}

void AGCBaseCharacter::OnStopAiming_Implementation()
{
	OnStopAimingInternal();
}

float AGCBaseCharacter::GetAimingMovementSpeed() const
{
	return CurrentAimingMovementSpeed;
}

const UCharacterEquipmentComponent* AGCBaseCharacter::GetCharacterEquipmentComponent() const
{
	return CharacterEquipmentComponent;
}

UCharacterEquipmentComponent* AGCBaseCharacter::GetCharacterEquipmentComponent_Mutable() const
{
	return CharacterEquipmentComponent;
}

const UCharacterAttributesComponent* AGCBaseCharacter::GetCharacterAttributesComponent() const
{
	return CharacterAttributesComponent;
}

bool AGCBaseCharacter::CanSprint()
{
	return GetMovementComponent()->IsMovingOnGround();
}

bool AGCBaseCharacter::CanProne()
{
	return bIsCrouched;
}

void AGCBaseCharacter::Prone()
{
	if (GCBaseCharacterMovementComponent)
	{
		if (CanProne())
		{
			GCBaseCharacterMovementComponent->SetIsProning(true);
			GCBaseCharacterMovementComponent->StartProne();
		}
	}
}

void AGCBaseCharacter::UnProne()
{
	if (GCBaseCharacterMovementComponent)
	{
		GCBaseCharacterMovementComponent->SetIsProning(false);
		GCBaseCharacterMovementComponent->StopProne();
	}
}

float AGCBaseCharacter::CalculatePelvisOffset()
{
	return -FMath::Abs(IKRightFootOffset - IKLeftFootOffset);
}

bool AGCBaseCharacter::CanMantle() const
{
	GetBaseCharacterMovementComponent()->IsOnZipline();
	return !GetBaseCharacterMovementComponent()->IsOnLadder() && !GetBaseCharacterMovementComponent()->IsOnZipline() && !(GetBaseCharacterMovementComponent()->MovementMode == MOVE_Falling);
}

void AGCBaseCharacter::OnMantle(const FMantlingSettings& MantlingSettings, float MantlingAnimationStartTime)
{

}

void AGCBaseCharacter::OnDeath()
{
	GetCharacterMovement()->DisableMovement();//MoveMode_NONE
	float Duration = PlayAnimMontage(OnDeathAnimMontage);
	if (Duration == 0.0f)
	{
		EnableRagdoll();
	}
}

void AGCBaseCharacter::OnStartAimingInternal()
{
	if (OnAimingStateChanged.IsBound())
	{
		OnAimingStateChanged.Broadcast(true);
	}
}

void AGCBaseCharacter::OnStopAimingInternal()
{
	if (OnAimingStateChanged.IsBound())
	{
		OnAimingStateChanged.Broadcast(false);
	}
}

void AGCBaseCharacter::TraceLineOfSight()
{
	if (!IsPlayerControlled())
		return;

	FVector ViewLocation;
	FRotator ViewRotation;

	APlayerController* PlayerController = GetController<APlayerController>();
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FVector ViewDirection = ViewRotation.Vector();
	FVector TraceEnd = ViewLocation + ViewDirection * LineOfSightDistance;

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, ViewLocation, TraceEnd, ECC_Visibility);

	if (LineOfSightObject.GetObject() != HitResult.Actor)
	{
		LineOfSightObject = HitResult.Actor.Get();

		FName ActionName;
		//проверка что мы можем реализовывать интерфейс
		if (LineOfSightObject.GetInterface())
		{
			ActionName = LineOfSightObject->GetActionEventName();
		}
		else
		{
			ActionName = NAME_None;
		}
		OnInteractableObjectFound.ExecuteIfBound(ActionName);
	}
}

float AGCBaseCharacter::SignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& ViewPoint)
{
	if (ObjectInfo->GetTag() == SignificanceTagCharacter)
	{
		AGCBaseCharacter* Character = StaticCast<AGCBaseCharacter*>(ObjectInfo->GetObject());
		if (!IsValid(Character))
		{
			return 0.0f;//????
		}
		if (Character->IsPlayerControlled() && Character->IsLocallyControlled())
		{
			return SignificanceValueVeryHigh;
		}

		float DistToSquared = FVector::DistSquared(Character->GetActorLocation(), ViewPoint.GetLocation());
		if (DistToSquared <= FMath::Square(VeryHighSignificanceDistance))
		{
			return SignificanceValueVeryHigh;
		}
		else if (DistToSquared <= FMath::Square(HighSignificanceDistance))
		{
			return SignificanceValueHigh;
		}
		else if (DistToSquared <= FMath::Square(MediumSignificanceDistance))
		{
			return SignificanceValueMedium;
		}
		else if (DistToSquared <= FMath::Square(LowSignificanceDistance))
		{
			return SignificanceValueLow;
		}
		else
		{
			return SignificanceValueVeryLow;
		}
	}
	return VeryHighSignificanceDistance;
}

void AGCBaseCharacter::PostSignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
{
	if (OldSignificance == Significance)
	{
		return;
	}
	if (ObjectInfo->GetTag() != SignificanceTagCharacter)
	{
		return;
	}

	AGCBaseCharacter* Character = StaticCast<AGCBaseCharacter*>(ObjectInfo->GetObject());
	if (!IsValid(Character))
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	AAIController* AIController = Character->GetController<AAIController>();

	if (Significance == SignificanceValueVeryHigh)
	{
		MovementComponent->SetComponentTickInterval(0.0f);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(0.0f);//замедл€ем обновдение меш AI
		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(0.0f);

		}
	}
	else if (Significance == SignificanceValueHigh)
	{
		MovementComponent->SetComponentTickInterval(0.0f);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(0.05f);//замедл€ем обновдение меш AI
		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(0.0f);

		}
	}
	else if (Significance == SignificanceValueMedium)
	{
		MovementComponent->SetComponentTickInterval(0.1f);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(0.1f);//замедл€ем обновдение меш AI
		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(0.1f);

		}
	}
	else if (Significance == SignificanceValueLow)
	{
		MovementComponent->SetComponentTickInterval(1.0f);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(1.0f);//замедл€ем обновдение меш AI
		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(1.1f);

		}
	}
	else if (Significance == SignificanceValueVeryLow)
	{
		MovementComponent->SetComponentTickInterval(5.0f);
		Character->GetMesh()->SetComponentTickEnabled(false);
		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(10.0f);

		}
	}

}

void AGCBaseCharacter::TryChangeSprintState(float DeltaTime)
{
	if (bIsSprintRequested && !GCBaseCharacterMovementComponent->IsSprinting() && CanSprint() && !GCBaseCharacterMovementComponent->IsProning())
	{
		GCBaseCharacterMovementComponent->StartSprint();
		OnSprintStart();
	}
	if (!bIsSprintRequested && GCBaseCharacterMovementComponent->IsSprinting())
	{
		GCBaseCharacterMovementComponent->StopSprint();
		OnSprintEnd();
	}

	
}

const FMantlingSettings& AGCBaseCharacter::GetMantlingSettings(float LedgeHeight) const
{
	return LedgeHeight > LowMantleMaxHeight ? HighMantleSettings : LowMantleSettings;
}

//homework
float AGCBaseCharacter::GetIKOffsetForASocket(const FName& SocketName)
{
	float Result = 0.0f;
	FVector SocketLocation = GetMesh()->GetSocketLocation(SocketName);
	//New
	float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	//Old
	FVector TraceStart(SocketLocation.X, SocketLocation.Y, GetActorLocation().Z);
	FVector TraceEnd = TraceStart - (CapsuleHalfHeight + IKTraceDistance) * FVector::UpVector;

	FHitResult HitResult;
	ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	if (UKismetSystemLibrary::LineTraceSingle(GetWorld(), TraceStart, TraceEnd, TraceType, true, TArray<AActor*>(), EDrawDebugTrace::ForOneFrame, HitResult, true))
	{
		float CharacterBottom = TraceStart.Z - CapsuleHalfHeight;
		Result = CharacterBottom - HitResult.Location.Z;
	}

	return Result;
}

void AGCBaseCharacter::EnableRagdoll()
{
	GetMesh()->SetCollisionProfileName(CollisionProfileRagdoll);
	GetMesh()->SetSimulatePhysics(true);
}

