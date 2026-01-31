#include "Presentation/SelectableChessPieceComponent.h"

USelectableChessPieceComponent::USelectableChessPieceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsSelected = false;
}

void USelectableChessPieceComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USelectableChessPieceComponent::SetSelected(bool bNewSelected)
{
	if (bNewSelected && !bIsSelectable)
	{
		return;
	}

	if (bIsSelected != bNewSelected)
	{
		bIsSelected = bNewSelected;
		OnSelectionStateChanged.Broadcast(bIsSelected);
	}
}

USelectableChessPieceComponent* USelectableChessPieceComponent::FindSelectableComponent(AActor* Actor)
{
	if (!Actor) return nullptr;
	return Actor->FindComponentByClass<USelectableChessPieceComponent>();
}
