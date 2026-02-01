// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChessPieceEffect.h"
#include "ChessPieceEffect_RemoveMask.generated.h"

/**
 * Effect that removes a visual mask from a chess piece.
 * Reveals the piece's true identity to opponents.
 * This is an instant effect (applies immediately, does not go on the stack).
 */
UCLASS(Blueprintable)
class PROJECTCHAIRS_API UChessPieceEffect_RemoveMask : public UChessPieceEffect
{
	GENERATED_BODY()

public:
	UChessPieceEffect_RemoveMask();

	virtual void OnApply_Implementation(AChessPieceActor* TargetPiece) override;
};
