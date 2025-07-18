// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Items/Powerups/Adrenaline.h"
#include "CharacterAttributesComponent.h"
#include "GCBaseCharacter.h"

bool UAdrenaline::Consume(AGCBaseCharacter* ConsumeTarget)
{
	ConsumeTarget->RestoreFullStamina();
	this->ConditionalBeginDestroy();
	return true;
}
