// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/GCDataTableUtils.h"
#include "GameCodeTypes.h"
#include "GCDataTableUtils.h"


FWeaponTableRow* GCDataTableUtils::FindWeaponData(const FName WeaponID)
{
    static const FString ContextString(TEXT("Find Weapon Data"));
    //������ ������ ������, ���� ���� ������� �� ��������������.
    UDataTable* WeaponDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/GenericShooter/Core/Data/DataTables/DT_WeaponList.DT_WeaponList"));
    if (WeaponDataTable == nullptr) 
        return nullptr;
    //FindRow - ��������� ����� ������������ ������ � ������� ������
    //ContextString - �������� ContextString ������������ ��� �������������� ��������������� ��������� ��� ���������� ������� ������. 
    // ���� FindRow �� ������ ����� ������ � ��������� WeaponID, ���� �������� ����� ���� ������� ��� ������� � ����������� ������. 
    // ��������, �� ����� ���������, � ����� ����� ���� ���� ������� �������, ��� ������� ������������� ������� �������� � ���������� ������.
    //WeaponID - ���� �������� ������������ ����� ���������� ������������� ��� ������ ���������� ������ � ������� ������. 
    // ���� �������, ��������, �������� ����� ������� ��� ��������� ����� ������, � WeaponID � ��� ��������, ������� ��������� ���������� ���������������� �������� ������. 
    // ������ ��� ������ ��� �����, ������� �������������� � �����-�� ���������� ������� � ����.
    return WeaponDataTable->FindRow<FWeaponTableRow>(WeaponID, ContextString);
}

FItemTableRow* GCDataTableUtils::FindInventoryItemData(const FName ItemID)
{
    static const FString ContextString(TEXT("Find Item Data"));
    //������ ������ ������, ���� ���� ������� �� ��������������.
    UDataTable* InventoryItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/GenericShooter/Core/Data/DataTables/DT_InventoryItemList.DT_InventoryItemList"));
    if (InventoryItemDataTable == nullptr)
        return nullptr;

    UE_LOG(LogTemp, Warning, TEXT("�������� ItemID: %s"), *ItemID.ToString());
    return InventoryItemDataTable->FindRow<FItemTableRow>(ItemID, ContextString);
}
