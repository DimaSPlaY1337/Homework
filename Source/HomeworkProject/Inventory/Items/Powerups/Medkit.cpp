// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Items/Powerups/Medkit.h"
#include "CharacterAttributesComponent.h"
#include "GCBaseCharacter.h"

bool UMedkit::Consume(AGCBaseCharacter* ConsumeTarget)
{
	ConsumeTarget->AddHealth(Health);
	//ConditionalBeginDestroy() ������������ � ������� ���������� ������� ��� ��������, ����������� �� UObject. 
	// ���� ����� �������� �� ���������� ������ �������� ����������� ������� � �����������, 
	// ��� ������ ����� ��������� ����������, ���� �� ���� ������ ��� ������.
	this->ConditionalBeginDestroy();//������� ������ �� ���������
	return true;
}
