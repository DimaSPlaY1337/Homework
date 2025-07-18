// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Environment/Door.h"
#include "GameCodeTypes.h"

// Sets default values
ADoor::ADoor()
{
	USceneComponent* DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorRoot"));
	SetRootComponent(DefaultSceneRoot);

	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(GetRootComponent());

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DoorPivot);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

// Called when the game starts or when spawned
void ADoor::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(DoorAnimationCurve))
	{
		//Сценарии использования: FOnTimelineEventStatic предназначен для обработки событий 
		// без необходимости в дополнительных данных, в то время как FOnTimelineFloatStatic используется, когда необходимо передавать информацию о текущем состоянии временной линии.
		FOnTimelineFloatStatic DoorAnimationDelegate;
		DoorAnimationDelegate.BindUObject(this, &ADoor::UpdateDoorAnimation);
		DoorOpenAnimTimeline.AddInterpFloat(DoorAnimationCurve, DoorAnimationDelegate);

		FOnTimelineEventStatic DoorOpenedDelegate;
		DoorOpenedDelegate.BindUObject(this, &ADoor::OnDoorAnimationFinished);
		//Функция SetTimelineFinishedFunc в Unreal Engine используется для назначения делегата, который будет вызван по завершении временной линии (Timeline).
		//  Это позволяет вам определить и привязать функцию, которая будет автоматически выполнена, когда временная линия достигнет своей конечной точки.
		DoorOpenAnimTimeline.SetTimelineFinishedFunc(DoorOpenedDelegate);
	}
}

void ADoor::InteractWithDoor()
{
	SetActorTickEnabled(true);
	if (bIsOpened)
		DoorOpenAnimTimeline.Reverse();
	else
		DoorOpenAnimTimeline.Play();
	bIsOpened = !bIsOpened;
}

void ADoor::UpdateDoorAnimation(float Alpha)
{
	float YawAngle = FMath::Lerp(AngleClosed, AngleOpened, FMath::Clamp(Alpha, 0.0f, 1.0f));
	DoorPivot->SetRelativeRotation(FRotator(0.0f, YawAngle, 0.0f));
}

void ADoor::OnDoorAnimationFinished()
{
	//Функция SetActorTickEnabled в Unreal Engine используется 
	// для включения или выключения тиков (tick) для конкретного актора.
	SetActorTickEnabled(false);
}

// Called every frame
void ADoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	DoorOpenAnimTimeline.TickTimeline(DeltaTime);

}

void ADoor::Interact(AGCBaseCharacter* Character)
{
	//ensureMsgf — это макрос, используемый в Unreal Engine для отладки, который проверяет, 
	// выполняется ли условие, и выводит сообщение, если это условие не выполняется.
	ensureMsgf(IsValid(DoorAnimationCurve), TEXT("DoorAnimationCurve is not set"));
	InteractWithDoor();
	if (OnInteractionEvent.IsBound())
		OnInteractionEvent.Broadcast();
}

FName ADoor::GetActionEventName() const
{
	return ActionInteract;
}

bool ADoor::HasOnInteractionCallback() const
{
	return true;
}

FDelegateHandle ADoor::AddOnInteractionUFunction(UObject* Object, const FName& FunctionName)
{
	return OnInteractionEvent.AddUFunction(Object, FunctionName);
}

void ADoor::RemoveOnInteractionDelegate(FDelegateHandle DelegateHandle)
{
	OnInteractionEvent.Remove(DelegateHandle);
}

void ADoor::OnLevelDeserialized_Implementation()
{
	float YawAngle = bIsOpened ? AngleOpened : AngleClosed;
	DoorPivot->SetRelativeRotation(FRotator(0.0f, YawAngle, 0.0f));
}

