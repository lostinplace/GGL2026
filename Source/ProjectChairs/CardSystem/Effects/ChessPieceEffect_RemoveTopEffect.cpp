// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessPieceEffect_RemoveTopEffect.h"
#include "ChessPieceEffectComponent.h"

UChessPieceEffect_RemoveTopEffect::UChessPieceEffect_RemoveTopEffect()
{
	BaseDuration = 0; // Instant effect
}

void UChessPieceEffect_RemoveTopEffect::OnApply_Implementation(AChessPieceActor* TargetPiece)
{
	Super::OnApply_Implementation(TargetPiece);

	if (OwningComponent)
	{
		OwningComponent->RemoveTopEffect();
	}
}
