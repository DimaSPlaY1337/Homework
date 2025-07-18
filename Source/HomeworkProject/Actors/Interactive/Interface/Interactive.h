// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactive.generated.h"

class AGCBaseCharacter;
UINTERFACE(MinimalAPI)//чистый интерфейс с точки зрения программирования(нельзя использовать в блупринтах)
class UInteractable : public UInterface
{
	//генерирует необходимый код
	GENERATED_BODY()

};
/**
 * 
 */
class HOMEWORKPROJECT_API IInteractable
{
	GENERATED_BODY()
public:
	DECLARE_MULTICAST_DELEGATE(FOnInteraction);

	virtual void Interact(AGCBaseCharacter* Character) PURE_VIRTUAL(IInteractable::Interact, );//нет параметров поэтому пусто после ,
	//нужно для того чтобы на различные объекты вешать различные варианты взаимодействия, 
	//например, где то взаимодействуем просто по нажатию кнопки
	virtual FName GetActionEventName() const PURE_VIRTUAL(IInteractable::GetActionEventName, return FName(NAME_None););
	//добавим критерий, что реализует ли наш текущий интерактивный объект OnInteractionCallback, 
	// если дизайнер виберет объект, у которого не будет Interactable или не реализует этот callback, то вернет false
	virtual bool HasOnInteractionCallback() const PURE_VIRTUAL(IInteractable::HasOnInteractionCallback, return false;);
	//даёт ссылку на привязку, которую потом будем удалять в Remove 
	virtual FDelegateHandle AddOnInteractionUFunction(UObject* Object, const FName& FunctionName) PURE_VIRTUAL(IInteractable::AddOnInteractionDelegate, return FDelegateHandle(););
	virtual void RemoveOnInteractionDelegate(FDelegateHandle DelegateHandle) PURE_VIRTUAL(IInteractable::RemoveOnInteractionDelegate, );
};
