// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/GCDataTableUtils.h"
#include "GameCodeTypes.h"
#include "GCDataTableUtils.h"


FWeaponTableRow* GCDataTableUtils::FindWeaponData(const FName WeaponID)
{
    static const FString ContextString(TEXT("Find Weapon Data"));
    //список нашего оружия, нашу дату таблицу мы захардкоживаем.
    UDataTable* WeaponDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/GenericShooter/Core/Data/DataTables/DT_WeaponList.DT_WeaponList"));
    if (WeaponDataTable == nullptr) 
        return nullptr;
    //FindRow - выполняет поиск определенной строки в таблице данных
    //ContextString - Параметр ContextString используется для предоставления дополнительного контекста при выполнении функции поиска. 
    // Если FindRow не сможет найти строку с указанным WeaponID, этот контекст может быть полезен для отладки и логирования ошибок. 
    // Например, он может указывать, в каком месте кода была вызвана функция, что поможет разработчикам быстрее находить и исправлять ошибки.
    //WeaponID - Этот параметр представляет собой уникальный идентификатор для поиска конкретной строки в таблице данных. 
    // Ваша таблица, вероятно, содержит много записей для различных видов оружия, и WeaponID – это значение, которое позволяет однозначно идентифицировать желаемую запись. 
    // Обычно это строка или число, которое сопоставляется с каким-то конкретным оружием в игре.
    return WeaponDataTable->FindRow<FWeaponTableRow>(WeaponID, ContextString);
}

FItemTableRow* GCDataTableUtils::FindInventoryItemData(const FName ItemID)
{
    static const FString ContextString(TEXT("Find Item Data"));
    //список нашего оружия, нашу дату таблицу мы захардкоживаем.
    UDataTable* InventoryItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/GenericShooter/Core/Data/DataTables/DT_InventoryItemList.DT_InventoryItemList"));
    if (InventoryItemDataTable == nullptr)
        return nullptr;

    UE_LOG(LogTemp, Warning, TEXT("Значение ItemID: %s"), *ItemID.ToString());
    return InventoryItemDataTable->FindRow<FItemTableRow>(ItemID, ContextString);
}
