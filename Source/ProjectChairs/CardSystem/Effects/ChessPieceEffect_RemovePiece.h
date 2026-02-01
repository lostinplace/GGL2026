// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChessPieceEffect.h"
#include "ChessPieceEffect_RemovePiece.generated.h"

/**
 * Effect that fully removes a chess piece from the board.
 * The piece is removed from the game state and visually removed.
 * This is an instant effect (applies immediately, does not go on the stack).
 */
UCLASS(Blueprintable)
class PROJECTCHAIRS_API UChessPieceEffect_RemovePiece : public UChessPieceEffect
{
	GENERATED_BODY()

public:
	UChessPieceEffect_RemovePiece();

	virtual void OnApply_Implementation(AChessPieceActor* TargetPiece) override;
};
