// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Items/Powerups/Medkit.h"
#include "CharacterAttributesComponent.h"
#include "GCBaseCharacter.h"

bool UMedkit::Consume(AGCBaseCharacter* ConsumeTarget)
{
	ConsumeTarget->AddHealth(Health);
	//ConditionalBeginDestroy() используется в системе управления памятью для объектов, производных от UObject. 
	// Этот метод отвечает за безопасное начало процесса уничтожения объекта и гарантирует, 
	// что объект будет правильно освобожден, если на него больше нет ссылок.
	this->ConditionalBeginDestroy();//удаляет айтемы из инфентаря
	return true;
}
