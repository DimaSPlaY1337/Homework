// Fill out your copyright notice in the Description page of Project Settings.


#include "GCBaseCharacterMovementComponent.h"
#include "PlayerCharacter.h"
#include "Zipline.h"
#include "GCBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "HomeworkProject/GameCodeTypes.h"
#include "Net/UnrealNetwork.h"

UGCBaseCharacterMovementComponent::UGCBaseCharacterMovementComponent()
{
	SetIsReplicatedByDefault(true);
}

void UGCBaseCharacterMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGCBaseCharacterMovementComponent, bIsSliding);
}

FNetworkPredictionData_Client* UGCBaseCharacterMovementComponent::GetPredictionData_Client() const
{
	//создаём новый ClientPredictionData в соответствующем формате если выполняется условие
	if (ClientPredictionData == nullptr)
	{
		//Mutable означает что сняты конст квалификаторы
		UGCBaseCharacterMovementComponent* MutableThis = const_cast<UGCBaseCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Character_GC(*this);
	}
	return ClientPredictionData;
}

void UGCBaseCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	bool bWasMantling = GetBaseCharacterOwner()->bIsMantling;
	bool bWasSliding = bIsSliding;
	//если флаг не равен 0 то флаг заполняется
	bIsSprinting = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bool bIsMantling = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	bool IsSliding = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;

	if (GetBaseCharacterOwner()->GetLocalRole() == ROLE_Authority)
	{
		if (!bWasSliding && IsSliding)
			GetBaseCharacterOwner()->Sliding(true);
	}

	if (GetBaseCharacterOwner()->GetLocalRole() == ROLE_Authority)
	{
		if (!bWasMantling && bIsMantling)
		{
			GetBaseCharacterOwner()->Mantle(true);//говорим что он должен точно подтянуться
		}
	}
}

void UGCBaseCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (bForceRotation)
	{
		FRotator CurrentRotation = UpdatedComponent->GetComponentRotation(); // Normalized
		CurrentRotation.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): CurrentRotation"));

		FRotator DeltaRot = GetDeltaRotation(DeltaTime);
		DeltaRot.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): GetDeltaRotation"));
		// Accumulate a desired new rotation.
		const float AngleTolerance = 1e-3f;

		if (!CurrentRotation.Equals(ForceTargetRotation, AngleTolerance))
		{
			FRotator DesiredRotation = ForceTargetRotation;
			// PITCH
			if (!FMath::IsNearlyEqual(CurrentRotation.Pitch, DesiredRotation.Pitch, AngleTolerance))
			{
				DesiredRotation.Pitch = FMath::FixedTurn(CurrentRotation.Pitch, DesiredRotation.Pitch, DeltaRot.Pitch);
			}

			// YAW
			if (!FMath::IsNearlyEqual(CurrentRotation.Yaw, DesiredRotation.Yaw, AngleTolerance))
			{
				DesiredRotation.Yaw = FMath::FixedTurn(CurrentRotation.Yaw, DesiredRotation.Yaw, DeltaRot.Yaw);
			}

			// ROLL
			if (!FMath::IsNearlyEqual(CurrentRotation.Roll, DesiredRotation.Roll, AngleTolerance))
			{
				DesiredRotation.Roll = FMath::FixedTurn(CurrentRotation.Roll, DesiredRotation.Roll, DeltaRot.Roll);
			}

			// Set the new rotation.
			DesiredRotation.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): DesiredRotation"));
			MoveUpdatedComponent(FVector::ZeroVector, DesiredRotation, /*bSweep*/ false);
		}
		else
		{
			ForceTargetRotation = FRotator::ZeroRotator;
			bForceRotation = false;
		}
		return;
	}

	if (IsOnLadder())
	{
		return;
	}
	Super::PhysicsRotation(DeltaTime);
}

float UGCBaseCharacterMovementComponent::GetMaxSpeed() const
{
	float Result = Super::GetMaxSpeed();
	if (bIsSprinting)
	{
		Result = SprintSpeed;
	}
	else if (IsOnLadder())
	{
		Result = ClimbingOnLadderMaxSpeed;
	}
	else if (bIsOutOfStamina)
	{
		Result = OutOfStaminaSpeed;
	}
	else if (IsWallRunning())
	{
		Result = MaxWallRunSpeed;
	}
	else if (GetBaseCharacterOwner()->IsAiming())
	{
		Result = GetBaseCharacterOwner()->GetAimingMovementSpeed();
	}
	return Result;
}

void UGCBaseCharacterMovementComponent::StartSprint()
{
	bIsSprinting = true;
	bForceMaxAccel = 1;
}

void UGCBaseCharacterMovementComponent::StopSprint()
{
	bIsSprinting = false;
	bForceMaxAccel = 0;
}

void UGCBaseCharacterMovementComponent::StartProne()
{
	DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - OldUnscaledHalfHeight;
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	const float ClampedCrouchedHalfHeight = FMath::Max3(0.f, CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), CrouchedHalfHeight);
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(ProneCapsuleRadius, ProneCapsuleHalfHeight);
	Mesh = CharacterOwner->GetMesh();
	float delta = CrouchedHalfHeight - ProneCapsuleHalfHeight;
	pc = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (CharacterOwner->GetMesh())
	{
		FVector& MeshRelativeLocation = CharacterOwner->GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = CharacterOwner->GetMesh()->GetRelativeLocation().Z + delta;
		pc->OnStartProne(delta);
	}
	if (bCrouchMaintainsBaseLocation)//Если идем по плоскости XY и не падаем то true
	{
		UpdatedComponent->MoveComponent(FVector(0.f, 0.f, -ScaledHalfHeightAdjust), UpdatedComponent->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
	}
}

void UGCBaseCharacterMovementComponent::StopProne()
{
	PawnLocation = UpdatedComponent->GetComponentLocation();
	SweepInflation = KINDA_SMALL_NUMBER * 10.f;
	CurrentCrouchedHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - ScaledHalfHeightAdjust); // Shrink by negative amount, so actually grow it.
	StandingLocation = PawnLocation + FVector(0.f, 0.f, StandingCapsuleShape.GetCapsuleHalfHeight() - CurrentCrouchedHalfHeight);
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), CrouchedHalfHeight);
	float delta = CrouchedHalfHeight - ProneCapsuleHalfHeight;
	if (CharacterOwner->GetMesh())
	{
		FVector& MeshRelativeLocation = CharacterOwner->GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = CharacterOwner->GetMesh()->GetRelativeLocation().Z - (CrouchedHalfHeight - ProneCapsuleHalfHeight);
		pc->OnEndProne(delta);
	}
	if (bCrouchMaintainsBaseLocation)//Если идем по плоскости XY и не падаем то true
	{
		UpdatedComponent->MoveComponent(StandingLocation - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
	}
}

bool UGCBaseCharacterMovementComponent::IsEncroached()
{
	FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CrouchTrace), false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	PawnLocation = UpdatedComponent->GetComponentLocation();
	ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - ScaledHalfHeightAdjust);
	StandingLocation = PawnLocation + FVector(0.f, 0.f, StandingCapsuleShape.GetCapsuleHalfHeight() - CurrentCrouchedHalfHeight);
	bEncroached = GetWorld()->OverlapBlockingTestByChannel(StandingLocation, FQuat::Identity, UpdatedComponent->GetCollisionObjectType(), StandingCapsuleShape, CapsuleParams, ResponseParam);
	if (bEncroached)
	{
		UE_LOG(LogTemp, Warning, TEXT("bEncroached"))
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("!bEncroached"))
	}
	return bEncroached;
}

void UGCBaseCharacterMovementComponent::PhysLadder(float DeltaTime, int32 Iterations)
{
	CalcVelocity(DeltaTime, 1.0f, false, ClimbingOnLadderBrakingDecelaretion);
	FVector Delta = Velocity * DeltaTime;

	FVector NewPos = GetActorLocation() + Delta;
	float NewPosProjection = GetCharacterToCurrentLadderProjection(NewPos);

	if (HasAnimRootMotion())
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), false, Hit);
		return;
	}

	if (NewPosProjection < MinLadderBottomOffset)//низ
	{
		DetachFromLadder(EDetachFromLadderMethod::ReachingTheBottom);
		return;
	}
	else if (NewPosProjection > (CurrentLadder->GetLadderHeight() - MaxLedderTopOffset))//вверх
	{
		DetachFromLadder(EDetachFromLadderMethod::ReachingTheTop);
		return;
	}

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), true, Hit);
}

const ALadder* UGCBaseCharacterMovementComponent::GetCurrentLadder()
{
	return CurrentLadder;
}

void UGCBaseCharacterMovementComponent::PhysMantling(float DeltaTime, int32 Iterations)
{
	float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(MantlingTimer) + CurrentMantlingParameters.StartTime;
	FVector MantlingCurveValue = CurrentMantlingParameters.MantlingCurve->GetVectorValue(ElapsedTime);//с помощью этого интерполируемся между началом и концом
	float PositionAlpha = MantlingCurveValue.X;//равна самой первой прямой которая есть
	float XYCorrectionAlpha = MantlingCurveValue.Y;//интерп вдоль осей xy
	float ZCorrectionAlpha = MantlingCurveValue.Z;

	FVector CorrectedInitialLocation = FMath::Lerp(CurrentMantlingParameters.InitialLocation, CurrentMantlingParameters.InitialAnimationLocation, XYCorrectionAlpha);
	CorrectedInitialLocation.Z = FMath::Lerp(CurrentMantlingParameters.InitialLocation.Z, CurrentMantlingParameters.InitialAnimationLocation.Z, ZCorrectionAlpha);
	FVector NewLocation;

	if (IsNextToPlatform)
	{
		FVector DeltaXYZ = CachedPlatformLoc - CurrentMantlingParameters.TargetLocation;
		NewLocation = OverlapLoc - DeltaXYZ;
	}
	else
	{
		NewLocation = CurrentMantlingParameters.TargetLocation;
	}
	NewLocation = FMath::Lerp(CorrectedInitialLocation, NewLocation, PositionAlpha);
	FRotator NewRotation = FMath::Lerp(CurrentMantlingParameters.InitialRotation, CurrentMantlingParameters.TargetRotation, PositionAlpha);
	FVector Delta;
	Delta = NewLocation - GetActorLocation();
	Velocity = Delta / DeltaTime;

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, NewRotation, false, Hit);
}

void UGCBaseCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (MovementMode == MOVE_Swimming)//текущее состояние
	{
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(SwimmingCapsuleRaduis, SwimmingCapsuleHalfHeight);
		if (IsProning())
		{
			pc->ChangeProneState();
			pc->ChangeCrouchState();
		}
	}
	else if (PreviousMovementMode == MOVE_Swimming)
	{
		DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	}

	if (MovementMode == MOVE_Custom)
	{
		switch (CustomMovementMode)
		{
		case(uint8)ECustomMovementMode::CMOVE_Mantling:
		{
			GetWorld()->GetTimerManager().SetTimer(MantlingTimer, this, &UGCBaseCharacterMovementComponent::EndMantle, CurrentMantlingParameters.Duration, false);
			break;
		}
		case(uint8)ECustomMovementMode::CMOVE_Zipline:
		{
			if (IsValid(ZiplineCurve) && IsValid(CurrentZipline))
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("OnMovementModeChanged"));
				SetZiplineWay(CurrentZipline->GetFirstLocation(), CurrentZipline->GetSecondLocation());
				FirstPostLoc = CurrentZipline->GetFirstLocation();
				SecondPostLoc = CurrentZipline->GetSecondLocation();
				FOnTimelineFloatStatic ZiplineMovementTimelineUpdate;
				ZiplineMovementTimelineUpdate.BindUObject(this, &UGCBaseCharacterMovementComponent::ZiplineTimelineUpdate);
				ZiplineTimeline.AddInterpFloat(ZiplineCurve, ZiplineMovementTimelineUpdate);
				ZiplineTimeline.PlayFromStart();
			}
			break;
		}
		case(uint8)ECustomMovementMode::CMOVE_WallRun:
		{
			CurrentTime = 0;
			break;
		}
		default:
			break;
		}
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == (uint8)ECustomMovementMode::CMOVE_Ladder)
	{
		CurrentLadder = nullptr;
	}
	else if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == (uint8)ECustomMovementMode::CMOVE_Zipline)
	{
		CurrentZipline = nullptr;
	}
}

void UGCBaseCharacterMovementComponent::PhysZipline(float DeltaTime, int32 Iterations)
{
	ZiplineTimeline.TickTimeline(DeltaTime);
}

void UGCBaseCharacterMovementComponent::SetZiplineWay(FVector FirstPost, FVector SecondPost)
{
	if (FirstPost.Z > SecondPost.Z)
	{
		ZiplineWay = int(sqrt(pow(SecondPost.X - GetActorLocation().X, 2) + pow(SecondPost.Y - GetActorLocation().Y, 2) + pow(SecondPost.Z - GetActorLocation().Z, 2)) - GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleRadius());
	}
	else if (FirstPost.Z < SecondPost.Z)
	{
		ZiplineWay = int(sqrt(pow(FirstPost.X - GetActorLocation().X, 2) + pow(FirstPost.Y - GetActorLocation().Y, 2) + pow(FirstPost.Z - GetActorLocation().Z, 2)) - GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleRadius());
	}
}

AGCBaseCharacter* UGCBaseCharacterMovementComponent::GetBaseCharacterOwner() const
{
	return StaticCast<AGCBaseCharacter*>(CharacterOwner);
}

float UGCBaseCharacterMovementComponent::GetCharacterToCurrentLadderProjection(const FVector& Location)
{
	checkf(IsValid(CurrentLadder), TEXT("can be invoked when current ladder is null"));

	FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	FVector LadderToCharacterDistance = Location - CurrentLadder->GetActorLocation();
	return FVector::DotProduct(LadderUpVector, LadderToCharacterDistance);
}

float UGCBaseCharacterMovementComponent::GetLadderSpeedRatio() const
{
	checkf(IsValid(CurrentLadder), TEXT("can be invoked when current ladder is null"));

	FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	return FVector::DotProduct(LadderUpVector, Velocity) / ClimbingOnLadderMaxSpeed;
}

void UGCBaseCharacterMovementComponent::AttachToZipline(const AZipline* zipline)
{
	CurrentZipline = zipline;
	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Zipline);
}

void UGCBaseCharacterMovementComponent::DetachFromZipline()
{
	SetMovementMode(MOVE_Falling);
}

void UGCBaseCharacterMovementComponent::AttachToLadder(const ALadder* Ladder)//начало взаимодействия с лестницей
{
	CurrentLadder = Ladder;

	FRotator TargetOrientationLocation = CurrentLadder->GetActorForwardVector().ToOrientationRotator();
	TargetOrientationLocation.Yaw += 180.0f;

	FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	FVector LadderForwardVector = CurrentLadder->GetActorForwardVector();
	float Projection = GetCharacterToCurrentLadderProjection(GetActorLocation());

	FVector NewCharacterLocation = CurrentLadder->GetActorLocation() + Projection * LadderUpVector + LadderToCharacterOffset * LadderForwardVector;
	if (CurrentLadder->GetIsOnTop())
	{
		NewCharacterLocation = CurrentLadder->GetAttachFromTopAnimMontageStartingLocation();
	}

	GetOwner()->SetActorLocation(NewCharacterLocation);
	GetOwner()->SetActorRotation(TargetOrientationLocation);

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Ladder);
}

bool UGCBaseCharacterMovementComponent::IsOnLadder() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Ladder;
}

void UGCBaseCharacterMovementComponent::DetachFromLadder(EDetachFromLadderMethod DetachFromLadderMethod)//прекращаем взаимодействие с лестницей
{
	switch (DetachFromLadderMethod)
	{
	case EDetachFromLadderMethod::JumpOff:
	{
		FVector JumpDirection = CurrentLadder->GetActorForwardVector();
		SetMovementMode(MOVE_Falling);
		FVector JumpVelocity = JumpDirection * JumpOffFromLadderSpeed;
		ForceTargetRotation = JumpDirection.ToOrientationRotator();
		bForceRotation = true;
		Launch(JumpVelocity);
		break;
	}
	case EDetachFromLadderMethod::ReachingTheTop:
	{
		GetBaseCharacterOwner()->Mantle(true);
		break;
	}
	case EDetachFromLadderMethod::ReachingTheBottom:
	{
		SetMovementMode(MOVE_Walking);
		break;
	}
	case EDetachFromLadderMethod::Fall:
	default:
	{
		SetMovementMode(MOVE_Falling);
		break;
	}
	}

}

bool UGCBaseCharacterMovementComponent::IsOnZipline() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Zipline;
}

void UGCBaseCharacterMovementComponent::ZiplineTimelineUpdate(const float Alpha)
{
	FVector Start;
	FVector End;
	FVector Delta;
	FVector NewLocation;
	FVector ZiplineVector = (FirstPostLoc - SecondPostLoc).GetSafeNormal();
	FVector ZiplineToCharacterDistance = GetActorLocation() - FirstPostLoc;
	float ActorToZiplineProjection = FVector::DotProduct(ZiplineVector, ZiplineToCharacterDistance);

	FVector NewCharacterLocation = FirstPostLoc + ActorToZiplineProjection * ZiplineVector;
	if (FirstPostLoc.Z > SecondPostLoc.Z)
	{
		Start = NewCharacterLocation;
		End = SecondPostLoc;
	}
	else if (FirstPostLoc.Z < SecondPostLoc.Z)
	{
		Start = NewCharacterLocation;
		End = FirstPostLoc;
	}
	Delta = End - Start;

	NewLocation = FMath::Lerp(Start, End, Alpha * ZiplineRollDownSpeed);
	int CurrentPosition = int(sqrt(pow(NewLocation.X - End.X, 2) + pow(NewLocation.Y - End.Y, 2) + pow(NewLocation.Z - End.Z, 2)) - GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleRadius());
	if (ZiplineWay == (ZiplineWay - CurrentPosition))
	{
		DetachFromZipline();
		return;
	}
	GetCharacterOwner()->SetActorLocation(NewLocation);
	FRotator NewRotation = Delta.ToOrientationRotator();
	GetCharacterOwner()->SetActorRelativeRotation(NewRotation);
}

void UGCBaseCharacterMovementComponent::OnPlayerCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	HitNrml = Hit.ImpactNormal;
	if (CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_WallRun)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_WallRun"));
		DetachFromWall();
		return;
	}
	if (!IsSurfaceWallRunable(HitNrml))
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("!IsSurfaceWallRunable(HitNormal)"));
		return;
	}
	if (!IsFalling())
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("!IsFalling()"));
		return;
	}
	EWallRunSide Side = EWallRunSide::None;
	FVector Direction = FVector::ZeroVector;
	GetWallRunSideAndDirection(HitNrml, Side, Direction);

	if (!AreRequiredKeysDown(Side))
	{
		return;
	}
	if (!IsWallRunning())
	{
		AttachToWall(Side, Direction);
	}
}

void UGCBaseCharacterMovementComponent::GetWallRunSideAndDirection(const FVector& HitNormal, EWallRunSide& OutSide, FVector& OutDirection)
{
	if (FVector::DotProduct(HitNormal, GetCharacterOwner()->GetActorRightVector()) > 0.0f)
	{
		OutSide = EWallRunSide::Left;
		OutDirection = FVector::CrossProduct(HitNormal, FVector::UpVector).GetSafeNormal();
	}
	else
	{
		OutSide = EWallRunSide::Right;
		OutDirection = FVector::CrossProduct(FVector::UpVector, HitNormal).GetSafeNormal();
	}
}

bool UGCBaseCharacterMovementComponent::IsWallRunning() const
{
	return CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_WallRun;
}

bool UGCBaseCharacterMovementComponent::IsSprinting() const
{
	return bIsSprinting;
}

void UGCBaseCharacterMovementComponent::DeltaHeight_Implementation(float Delta)
{
	if (bCrouchMaintainsBaseLocation)
	{
		UpdatedComponent->MoveComponent(FVector(0.f, 0.f, Delta), UpdatedComponent->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
	}
}

void UGCBaseCharacterMovementComponent::Server_Launch_Implementation(const FVector SlideVelocity)
{
	Launch(SlideVelocity);
	Multicast_Launch(SlideVelocity);
}

void UGCBaseCharacterMovementComponent::Multicast_Launch_Implementation(const FVector SlideVelocity)
{
	Launch(SlideVelocity);
}

void UGCBaseCharacterMovementComponent::OnRep_IsSliding(bool bWasSliding)
{
	if (GetBaseCharacterOwner()->GetLocalRole() == ROLE_SimulatedProxy && !bWasSliding && bIsSliding)
	{
		GetBaseCharacterOwner()->Sliding(true);
	}
}

void UGCBaseCharacterMovementComponent::AttachToWall(EWallRunSide Side, const FVector& Direction)
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, TEXT("Start"));
	SetMovementMode(EMovementMode::MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_WallRun);

	CurrentWallRunSide = Side;
	CurrentWallRunDirection = Direction;

	GetWorld()->GetTimerManager().SetTimer(WallRunTimer, this, &UGCBaseCharacterMovementComponent::DetachFromWall, MaxWallRunTime, false);
}

void UGCBaseCharacterMovementComponent::DetachFromWall()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, TEXT("End"));
	SetMovementMode(MOVE_Falling);

	CurrentWallRunSide = EWallRunSide::None;
	CurrentWallRunDirection = FVector::ZeroVector;
}


void UGCBaseCharacterMovementComponent::PhysWallRun(float DeltaSeconds, int32 iterations)
{
	CurrentTime += DeltaSeconds;
	float Value = VerticalSpeedForWallRun->GetFloatValue(CurrentTime / (MaxWallRunTime / 4));
	if (!AreRequiredKeysDown(CurrentWallRunSide))
	{
		DetachFromWall();
		return;
	}
	CalcVelocity(DeltaSeconds, 0.0f, false, WallRunBrakingDeceleration);
	FVector UpVector = FVector(0.0f, 0.0f, 1.0f);
	FVector Delta = Velocity * DeltaSeconds + UpVector * Value;
	FHitResult Hit;
	FRotator UpdateRotation = FRotator(0.0f, Delta.ToOrientationRotator().Yaw, Delta.ToOrientationRotator().Roll);
	SafeMoveUpdatedComponent(Delta, UpdateRotation, false, Hit);

	FHitResult HitResult;
	FVector LineTraceDirection = CurrentWallRunSide == EWallRunSide::Right ? GetCharacterOwner()->GetActorRightVector() : -GetCharacterOwner()->GetActorRightVector();
	float LineTraceLenght = 200.0f;

	FVector StartPosition = GetCharacterOwner()->GetActorLocation();
	FVector EndPosition = StartPosition + LineTraceLenght * LineTraceDirection;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetCharacterOwner());
	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartPosition, EndPosition, ECC_WallRunnable, QueryParams))
	{
		EWallRunSide Side = EWallRunSide::None;
		FVector Direction = FVector::ZeroVector;
		GetWallRunSideAndDirection(HitResult.ImpactNormal, Side, Direction);

		if (Side != CurrentWallRunSide)
		{
			DetachFromWall();
		}
		else
		{
			CurrentWallRunDirection = Direction;
			Velocity = GetMaxSpeed() * CurrentWallRunDirection;
		}
	}
	else
	{
		DetachFromWall();
	}
}

bool UGCBaseCharacterMovementComponent::IsSurfaceWallRunable(const FVector& SurfaceNormal) const
{
	if (SurfaceNormal.Z > GetCharacterOwner()->GetCharacterMovement()->GetWalkableFloorZ() || SurfaceNormal.Z < -0.005f)
	{
		return false;
	}
	return true;
}

bool UGCBaseCharacterMovementComponent::AreRequiredKeysDown(EWallRunSide side)
{
	pc = StaticCast<APlayerCharacter*>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	float Right = pc->GetRightAxis();
	float Forward = pc->GetForwardAxis();
	if (Forward < 0.1f)
	{
		return false;
	}
	if (side == EWallRunSide::Right && Right < -0.1f)
	{
		return false;
	}
	if (side == EWallRunSide::Left && Right > 0.1f)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool UGCBaseCharacterMovementComponent::GetWallRunSide()
{
	if (CurrentWallRunSide == EWallRunSide::Left)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void UGCBaseCharacterMovementComponent::JumpFromWall()
{
	if (IsWallRunning())
	{
		FVector JumpDirection;
		if (CurrentWallRunSide == EWallRunSide::Right)
		{
			JumpDirection = FVector::CrossProduct(CurrentWallRunDirection, FVector::UpVector).GetSafeNormal();
		}
		else
		{
			JumpDirection = FVector::CrossProduct(FVector::UpVector, CurrentWallRunDirection).GetSafeNormal();
		}
		JumpDirection += FVector::UpVector;
		GetCharacterOwner()->LaunchCharacter(JumpZVelocity * JumpDirection.GetSafeNormal(), false, true);
		DetachFromWall();
	}
}
/*if (GetOwner()->GetLocalRole() == ROLE_Authority)
	pc->Client_MoveUpMesh(SlideDelta);*/
	/*if (CharacterOwner->GetMesh() && GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		pc->OnStartSlide(SlideDelta);
	}*/
void UGCBaseCharacterMovementComponent::StartSlide()
{
	DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	bOrientRotationToMovement = false;
	SetPlaneConstraintEnabled(true);
	SetPlaneConstraintNormal(FVector::UpVector);

	pc = StaticCast<APlayerCharacter*>(GetOwner());
	SlideDelta = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - SlideCaspsuleHalfHeight;
	FVector CharacterLocation = UpdatedComponent->GetComponentLocation();
	CharacterLocation.Z -= SlideDelta;
	CharacterOwner->GetCapsuleComponent()->SetRelativeLocation(CharacterLocation);
	const FVector SlideDirection = GetCharacterOwner()->GetActorForwardVector();
	const FVector SlideVelocity = SlideDirection * SlideSpeed;

	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), SlideCaspsuleHalfHeight);

	Server_Launch(SlideVelocity);

	pc->OnStartSlide(SlideDelta);//здесь происходит смещение мешки и камеры

	bIsSliding = true;
}

void UGCBaseCharacterMovementComponent::StopSlide_Implementation()
{
	bOrientRotationToMovement = true;
	SetPlaneConstraintEnabled(false);
	SetPlaneConstraintNormal(FVector::ZeroVector);

	const float CompScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float ScaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector CharacterLocation = UpdatedComponent->GetComponentLocation();
	CharacterLocation.Z += SlideDelta;
	CharacterOwner->GetCapsuleComponent()->SetRelativeLocation(CharacterLocation);
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), true);

	pc->OnEndSlide(SlideDelta);
	if (IsEncroached())
		pc->Crouch();

	pc->Client_MoveDownMesh(SlideDelta);

	bIsSliding = false;
}

void UGCBaseCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)//отрабатывает когда включен кастом мод каждый тик
{
	if (GetBaseCharacterOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	switch (CustomMovementMode)
	{
	case(uint8)ECustomMovementMode::CMOVE_WallRun:
	{
		PhysWallRun(DeltaTime, Iterations);
		break;
	}
	case(uint8)ECustomMovementMode::CMOVE_Zipline:
	{
		PhysZipline(DeltaTime, Iterations);
		break;
	}
	case(uint8)ECustomMovementMode::CMOVE_Mantling:
	{
		PhysMantling(DeltaTime, Iterations);
		break;
	}
	case(uint8)ECustomMovementMode::CMOVE_Ladder:
	{
		PhysLadder(DeltaTime, Iterations);
		break;
	}
	default:
		break;
	}
	Super::PhysCustom(DeltaTime, Iterations);
}

void UGCBaseCharacterMovementComponent::EndMantle()
{
	GetBaseCharacterOwner()->bIsMantling = false;
	IsNextToPlatform = 0;
	SetMovementMode(MOVE_Walking);
}

bool UGCBaseCharacterMovementComponent::IsMantling() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Mantling;
}

void UGCBaseCharacterMovementComponent::StartMantle(const FMantlingMovementParameters& MantlingParametres)
{
	CurrentMantlingParameters = MantlingParametres;
	CachedPlatformLoc = OverlapLoc;
	SetMovementMode(EMovementMode::MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Mantling);
}

void FSavedMove_GC::Clear()
{
	Super::Clear();
	bSavedIsSprinting = 0;
	bSavedIsMantling = 0;
	bSavedIsPressingSlide = 0;
}

uint8 FSavedMove_GC::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	//FLAG_Reserved_1 = 0x04,	// Reserved for future use
	//	FLAG_Reserved_2 = 0x08,	// Reserved for future use
	//	// Remaining bit masks are available for custom flags.
	//	FLAG_Custom_0 = 0x10, - Sprinting flag
	//	FLAG_Custom_1 = 0x20, - Mantling
	//	FLAG_Custom_2 = 0x40, - Sliding
	//	FLAG_Custom_3 = 0x80,

	if (bSavedIsSprinting)
		Result |= FLAG_Custom_0;

	if (bSavedIsMantling)
	{
		//инвертирует все биты операнда (меняет 0 на 1 и наоборот).
		//снимаем флаг на прыжок для подтягивания
		Result &= ~FLAG_JumpPressed;
		Result |= FLAG_Custom_1;
	}

	if (bSavedIsPressingSlide)
	{
		Result &= ~FLAG_Custom_0;
		Result |= FLAG_Custom_2;
	}

	return Result;
}

bool FSavedMove_GC::CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* InCharacter, float MaxDelta) const
{
	FSavedMove_GC* NewMove = StaticCast<FSavedMove_GC*>(NewMovePtr.Get());

	if (bSavedIsSprinting != NewMove->bSavedIsSprinting || bSavedIsMantling != NewMove->bSavedIsMantling || bSavedIsPressingSlide != NewMove->bSavedIsPressingSlide)
		return false;

	return Super::CanCombineWith(NewMovePtr, InCharacter, MaxDelta);
}
//NewAccel - новое ускорение
void FSavedMove_GC::SetMoveFor(ACharacter* InCharacter, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(InCharacter, InDeltaTime, NewAccel, ClientData);

	check(InCharacter->IsA<AGCBaseCharacter>());
	AGCBaseCharacter* InBaseCharacter = StaticCast<AGCBaseCharacter*>(InCharacter);
	UGCBaseCharacterMovementComponent* MovementComponent = InBaseCharacter->GetBaseCharacterMovementComponent();
	//сохраняем значение
	bSavedIsSprinting = MovementComponent->bIsSprinting;
	bSavedIsMantling = InBaseCharacter->bIsMantling;
	bSavedIsPressingSlide = MovementComponent->bIsSliding;
}

void FSavedMove_GC::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UGCBaseCharacterMovementComponent* MovementComponent = StaticCast<UGCBaseCharacterMovementComponent*>(Character->GetMovementComponent());

	MovementComponent->bIsSprinting = bSavedIsSprinting;
	MovementComponent->bIsSliding = bSavedIsPressingSlide;
}

FNetworkPredictionData_Client_Character_GC::FNetworkPredictionData_Client_Character_GC(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)//быстро вызываем конструктор базового класса
{
}

FSavedMovePtr FNetworkPredictionData_Client_Character_GC::AllocateNewMove()
{
	//создаёт указатель FSavedMovePtr на новый созданный объект 
	return FSavedMovePtr(new FSavedMove_GC());
}
