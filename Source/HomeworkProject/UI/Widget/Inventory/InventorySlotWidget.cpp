// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/Inventory/InventorySlotWidget.h"
#include "Components/CharacterComponents/CharacterInventoryComponent.h"
#include "Inventory/Items/InventoryItem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"

void UInventorySlotWidget::InitializeItemSlot(FInventorySlot& InventorySlot)
{
	//сохран€ем указатель на наш слот
	LinkedSlot = &InventorySlot;

	FInventorySlot::FInventorySlotUpdate OnInventorySlotUpdate;
	OnInventorySlotUpdate.BindUObject(this, &UInventorySlotWidget::UpdateView);
	LinkedSlot->BindOnInventorySlotUpdate(OnInventorySlotUpdate);
}

void UInventorySlotWidget::UpdateView()
{
	if (LinkedSlot == nullptr)
	{
		ImageItemIcon->SetBrushFromTexture(nullptr);
		return;
	}	

	if (LinkedSlot->Item.IsValid())
	{
		const FInventoryItemDescription& Description = LinkedSlot->Item->GetDescription();
		ImageItemIcon->SetBrushFromTexture(Description.Icon);//задаЄм иконку
	}
	else
	{
		ImageItemIcon->SetBrushFromTexture(nullptr);
	}
}
void UInventorySlotWidget::SetItemIcon(UTexture2D* Icon)
{
	ImageItemIcon->SetBrushFromTexture(Icon);
}
//InGeometry - позвол€ет описать где произошло событие
FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (LinkedSlot == nullptr)//если не прив€зан никакой слот, то результат: вещь обработана
		return FReply::Handled();

	if(!LinkedSlot->Item.IsValid())
		return FReply::Handled();

	FKey MouseBtn = InMouseEvent.GetEffectingButton();//получаем кнопку от мыши
	if (MouseBtn == EKeys::RightMouseButton)
	{
		TWeakObjectPtr<UInventoryItem> LinkedSlotItem = LinkedSlot->Item;
		AGCBaseCharacter* ItemOwner = Cast<AGCBaseCharacter>(LinkedSlotItem->GetOuter());

		//потребление предмета в инвентаре, если успешно, то очищаем наш слот
		if (LinkedSlotItem->Consume(ItemOwner))
			LinkedSlot->ClearSlot();

		return FReply::Handled();
	}
	//если не срабатиывает ни одно условие, то это
	//UWidgetBlueprintLibrary::DetectDragIfPressed(): Ёто статический метод библиотеки UWidgetBlueprintLibrary,
	// который предназначен дл€ проверки, было ли нажатие мыши использовано дл€ начала перетаскивани€ элемента.
	//NativeReply необходимо дл€ предоставлени€ информации о результатах обработки событи€.
	FEventReply Reply = UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton);
	return Reply.NativeReply;
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	//создаЄм драгендроп операцию
	UDragDropOperation* DragOperation = Cast<UDragDropOperation>(UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass()));

	//создаЄм новый UInventorySlotWidget виджет
	UInventorySlotWidget* DragWidget = CreateWidget<UInventorySlotWidget>(GetOwningPlayer(), GetClass());
	DragWidget->ImageItemIcon->SetBrushFromTexture(LinkedSlot->Item->GetDescription().Icon);//выставл€ем цвет иконки

	DragOperation->DefaultDragVisual = DragWidget;
	DragOperation->Pivot = EDragPivot::MouseDown;//выставл€ем ось за что оно закрепл€етс€
	//Payload представл€ет собой данные, которые передаютс€ вместе с операцией перетаскивани€
	// «десь `DragOperation` представл€ет собой объект, который отвечает за операцию перетаскивани€. 
	// `LinkedSlot->Item.Get()` используетс€ дл€ получени€ ссылки на объект, который вы хотите передать 
	// вместе с операцией перетаскивани€, и назначаетс€ свойству `Payload` этой операции. “аким образом, 
	// когда пользователи начинают перетаскивание, `Payload` содержит информацию о том, что именно перетаскиваетс€.
	DragOperation->Payload = LinkedSlot->Item.Get();
	OutOperation = DragOperation;

	LinkedSlot->ClearSlot();//очищаем слот айтем виджета
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!LinkedSlot->Item.IsValid())
	{
		LinkedSlot->Item = TWeakObjectPtr<UInventoryItem>(Cast<UInventoryItem>(InOperation->Payload));
		LinkedSlot->UpdateSlotState();
		return true;
	}

	return false;
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	//если мы вытащили за какие то пределы и у нас ничего не получилось, то мы возращаем элемент нашего айтема в текущий элемент драгондроба(то есть саму в себ€)
	//и обновл€ем 
	LinkedSlot->Item = TWeakObjectPtr<UInventoryItem>(Cast<UInventoryItem>(InOperation->Payload));
	LinkedSlot->UpdateSlotState();
}
