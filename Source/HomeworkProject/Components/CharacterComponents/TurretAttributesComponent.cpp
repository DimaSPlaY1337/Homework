// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CharacterComponents/TurretAttributesComponent.h"
#include "HomeworkProject/Subsystems/DebugSubsystem.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Perception/AISense_Damage.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UTurretAttributesComponent::UTurretAttributesComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UTurretAttributesComponent::BeginPlay()
{
	Super::BeginPlay();

	checkf(MaxHealth > 0.0f, TEXT("max health must be > 0"));
	checkf(GetOwner()->IsA<ATurret>(), TEXT("can be use only with ATurret"));
	CachedTurretOwner = StaticCast<ATurret*>(GetOwner());

	Health = MaxHealth;
	CachedTurretOwner->OnTakeAnyDamage.AddDynamic(this, &UTurretAttributesComponent::OnTakeAnyDamage);//подписываемся на получение любого урона
}

void UTurretAttributesComponent::OnTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsAlive())
	{
		return;
	}

	UE_LOG(LogDamage, Warning, TEXT("OnTakeAnyDamage %s recevied %.2f amout of damage from %s"), *CachedTurretOwner->GetName(), Damage, *DamageCauser->GetName());
	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
	
	if (CachedTurretOwner->GetTurretState() != ETurretState::Firing)
	{
		Shots += 1;
		CachedTurretOwner->SetShots(Shots);
		UAISense_Damage::ReportDamageEvent(GetWorld(), DamagedActor, DamageCauser, Damage, DamageCauser->GetActorLocation(), GetOwner()->GetActorLocation());
	}
	
	if (Health <= 0.0f)
	{
		UE_LOG(LogDamage, Warning, TEXT("Character %s is killed by an actor %s"), *CachedTurretOwner->GetName(), *DamageCauser->GetName());
		if (OnDeathEvent.IsBound())//если делегат привязан
		{
			OnDeathEvent.Broadcast();//оповещвем всех кто привязан к этому событию
			Explode();
		}
	}
}


// Called every frame
void UTurretAttributesComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	DebugDrawAttributes();
#endif
}

void UTurretAttributesComponent::Explode()
{
	if (OnExplosion.IsBound())
		OnExplosion.Broadcast();
}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
void UTurretAttributesComponent::DebugDrawAttributes()
{
	UDebugSubsystem* DebugSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDebugSubsystem>();
	if (!DebugSubsystem->IsCategoryEnabled(DebugCategoryCharacterAttributes))//если в DebugSubsystem не включена категория, то выходим
	{
		return;
	}
	FVector TextLocationForHealth = CachedTurretOwner->GetActorLocation();
	DrawDebugString(GetWorld(), TextLocationForHealth, FString::Printf(TEXT("Health: %.2f"), Health), nullptr, FColor::Green, 0.0f, true);
}
#endif