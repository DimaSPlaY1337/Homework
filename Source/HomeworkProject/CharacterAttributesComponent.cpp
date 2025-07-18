// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterAttributesComponent.h"
#include "HomeworkProject/Subsystems/DebugSubsystem.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "GCBaseCharacter.h"
#include "Components/SlateWrapperTypes.h"
#include "GameFramework/PhysicsVolume.h"
#include "Components/CapsuleComponent.h"
#include "GameCodeTypes.h"
#include "Perception/AISense_Damage.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UCharacterAttributesComponent::UCharacterAttributesComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UCharacterAttributesComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCharacterAttributesComponent, Health);
}

// Called every frame
void UCharacterAttributesComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	DebugDrawAttributes();
#endif

	UpdateStaminaValue(DeltaTime);
	UpdateOxygenValue(DeltaTime);
}

float UCharacterAttributesComponent::GetHealthPercent() const
{
	return Health / MaxHealth;
}

float UCharacterAttributesComponent::GetOxygenPercent() const
{
	return CurrentOxygen / MaxOxygen;
}

float UCharacterAttributesComponent::GetStaminaPercent() const
{
	return CurrentStamina / MaxStamina;
}

void UCharacterAttributesComponent::AddHealth(float HealthToAdd)
{
	Health = FMath::Clamp(Health + HealthToAdd, 0.0f, MaxHealth);
	OnHealthChanged();
}

void UCharacterAttributesComponent::BeginPlay()
{
	Super::BeginPlay();

	checkf(MaxHealth > 0.0f, TEXT("max health must be > 0"));
	checkf(GetOwner()->IsA<AGCBaseCharacter>(), TEXT("can be use only with AGCBaseCharacter"));
	CachedBaseCharacterOwner = StaticCast<AGCBaseCharacter*>(GetOwner());
	GCBaseCharacterMovementComponent = StaticCast<UGCBaseCharacterMovementComponent*>(CachedBaseCharacterOwner->GetCharacterMovement());

	Health = MaxHealth;
	CurrentStamina = MaxStamina;
	CurrentOxygen = MaxOxygen;
	if (GetOwner()->HasAuthority())
		CachedBaseCharacterOwner->OnTakeAnyDamage.AddDynamic(this, &UCharacterAttributesComponent::OnTakeAnyDamage);//подписываемся на получение любого урона
}

void UCharacterAttributesComponent::OnRep_Health()
{
	OnHealthChanged();
}

void UCharacterAttributesComponent::OnHealthChanged()
{
	if (OnHealthChangedEvent.IsBound())
	{
		OnHealthChangedEvent.Broadcast(GetHealthPercent());
	}
	if (Health <= 0.0f)
	{
		if (OnDeathEvent.IsBound())//если делегат привязан
		{
			OnDeathEvent.Broadcast();//оповещвем всех кто привязан к этому событию
		}
	}
}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
void UCharacterAttributesComponent::DebugDrawAttributes()
{
	UDebugSubsystem* DebugSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDebugSubsystem>();
	if (!DebugSubsystem->IsCategoryEnabled(DebugCategoryCharacterAttributes))//если в DebugSubsystem не включена категория, то выходим
	{
		return;
	}
	FVector TextLocationForHealth = CachedBaseCharacterOwner->GetActorLocation() + (CachedBaseCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.0f) * FVector::UpVector;
	DrawDebugString(GetWorld(), TextLocationForHealth, FString::Printf(TEXT("Health: %.2f"), Health), nullptr, FColor::Green, 0.0f, true);

	FVector TextLocationForStamina = TextLocationForHealth - (10.0f * FVector::UpVector);
	DrawDebugString(GetWorld(), TextLocationForStamina, FString::Printf(TEXT("Stamina: %.2f"), CurrentStamina), nullptr, FColor::Blue, 0.0f, true);
}
#endif

void UCharacterAttributesComponent::OnTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsAlive())
	{
		return;
	}

	UE_LOG(LogDamage, Warning, TEXT("OnTakeAnyDamage %s recevied %.2f amout of damage from %s"), *CachedBaseCharacterOwner->GetName(), Damage, *DamageCauser->GetName());
	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
	OnHealthChanged();
}

void UCharacterAttributesComponent::UpdateStaminaValue(float DeltaTime)
{
	if (CachedBaseCharacterOwner->StaminaPercentEvent.IsBound())
	{
		if (CurrentStamina != MaxStamina)
		{
			CachedBaseCharacterOwner->StaminaPercentEvent.Broadcast(ESlateVisibility::Visible);
		}
		else
		{
			CachedBaseCharacterOwner->StaminaPercentEvent.Broadcast(ESlateVisibility::Hidden);
		}
	}

	if (GetBaseCharacterMovementComponent()->IsSprinting())
	{
		CurrentStamina -= SprintStaminaConsumptionVelocity * DeltaTime;
		CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
	}
	else
	{
		CurrentStamina += StaminaRestoreVelocity * DeltaTime;
		CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
	}

	if (OutOfStamina.IsBound())
	{
		if (CurrentStamina == 0)
		{
			OutOfStamina.Broadcast(true);
		}
		else if (CurrentStamina == MaxStamina)
		{
			OutOfStamina.Broadcast(false);
		}
	}
}

void UCharacterAttributesComponent::UpdateOxygenValue(float DeltaTime)
{
	const FName Head = FName("head");
	FVector HeadPosition = CachedBaseCharacterOwner->GetMesh()->GetSocketLocation(Head); // FName(“head”) стоит предварительно сохранить как строковую константу.
	UCharacterMovementComponent* Movement = CachedBaseCharacterOwner->GetCharacterMovement();

	if (CachedBaseCharacterOwner->OxygenPercentEvent.IsBound())
	{
		if (CurrentOxygen != MaxOxygen)
		{
			CachedBaseCharacterOwner->OxygenPercentEvent.Broadcast(ESlateVisibility::Visible);
		}
		else
		{
			CachedBaseCharacterOwner->OxygenPercentEvent.Broadcast(ESlateVisibility::Hidden);
		}
	}
	if (Movement->IsSwimming()) // Проверяем, что персонаж находится в состоянии плавания
	{
		APhysicsVolume* Volume = Movement->GetPhysicsVolume(); //Получаем теущий физический объем
		float VolumeTopPlane = Volume->GetActorLocation().Z + Volume->GetBounds().BoxExtent.Z; // Берем Z координату для центра данного объема и добавляем к нему половину высота бокса с учетом масштаба 
		if (IsSwimmingUnderWater(HeadPosition.Z, VolumeTopPlane))
		{
			CurrentOxygen -= SwimOxygenConsumptionVelocity * DeltaTime;
			CurrentOxygen = FMath::Clamp(CurrentOxygen, 0.0f, MaxOxygen);
		}
		else
		{
			CurrentOxygen += OxygenRestoreVelocity * DeltaTime;
			CurrentOxygen = FMath::Clamp(CurrentOxygen, 0.0f, MaxOxygen);
		}
	}
	else
	{
		CurrentOxygen += OxygenRestoreVelocity * DeltaTime;
		CurrentOxygen = FMath::Clamp(CurrentOxygen, 0.0f, MaxOxygen);
	}
}

bool UCharacterAttributesComponent::IsSwimmingUnderWater(float HeadLocation, float VolumeLocation) const
{
	return int(VolumeLocation) <= int(HeadLocation) ? false : true;
}

void UCharacterAttributesComponent::RestoreFullStamina()
{
	CurrentStamina = MaxStamina;
}

void UCharacterAttributesComponent::OnLevelDeserialized_Implementation()
{
	OnHealthChanged();
}
