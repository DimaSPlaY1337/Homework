// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Projectiles/GCProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GCProjectile.h"
#include "GameCodeTypes.h"

// Sets default values
AGCProjectile::AGCProjectile()
{
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(5.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);//создать отдельный пресет для коллизии персонажа
	SetRootComponent(CollisionComponent);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	ProjectileMovementComponent->InitialSpeed = 2000.0f;
	//ProjectileMovementComponent->bAutoActivate = false;//если мы не будем делать, то в момент создания наш проджектайл полетит в любом направлении

	SetReplicates(true);
	SetReplicateMovement(true);
}

void AGCProjectile::LaunchProjectile(FVector Direction)
{
	UE_LOG(LogDamage, Warning, TEXT("Explode"));
	ProjectileMovementComponent->Velocity = Direction * ProjectileMovementComponent->InitialSpeed;
	CollisionComponent->IgnoreActorWhenMoving(GetOwner(), true);
	if (!ExplodeWhenHit)
	{
		OnProjectileLaunched();
	}
}

void AGCProjectile::SetProjectileActive_Implementation(bool bIsProjectileActive)
{
	ProjectileMovementComponent->SetActive(bIsProjectileActive);
}

void AGCProjectile::BeginPlay()
{
	Super::BeginPlay();
	CollisionComponent->OnComponentHit.AddDynamic(this, &AGCProjectile::OnCollisionHit);
}

void AGCProjectile::OnProjectileLaunched()
{
}

void AGCProjectile::Explode()
{
	//добавляем в массив GrenadePool
}

void AGCProjectile::OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OnProjectileHit.IsBound() && !ExplodeWhenHit)
	{
		OnProjectileHit.Broadcast(this, Hit, ProjectileMovementComponent->Velocity.GetSafeNormal());
	}

	if (ExplodeWhenHit)
	{
		Explode();
	}
}
