// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChessPieceEffect.h"
#include "Logic/ChessData.h"
#include "ChessPieceEffect_ApplyMask.generated.h"

/**
 * Effect that applies a visual mask to a chess piece.
 * The mask disguises the piece's true identity to opponents.
 * This is an instant effect (applies immediately, does not go on the stack).
 */
UCLASS(Blueprintable)
class PROJECTCHAIRS_API UChessPieceEffect_ApplyMask : public UChessPieceEffect
{
	GENERATED_BODY()

public:
	UChessPieceEffect_ApplyMask();

	/** The piece type to use as the mask (what opponents will see) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	EPieceType MaskPieceType;

	virtual void OnApply_Implementation(AChessPieceActor* TargetPiece) override;
};
