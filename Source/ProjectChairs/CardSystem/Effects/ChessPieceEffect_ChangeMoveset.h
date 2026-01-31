// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChessPieceEffect.h"
#include "CardSystem/CardTypes.h"
#include "ChessPieceEffect_ChangeMoveset.generated.h"

/**
 * Effect that changes a piece's moveset to that of another piece type.
 * Uses sliding rules for Queen/Rook/Bishop with appropriate diagonal/orthogonal flags.
 */
UCLASS(Blueprintable)
class PROJECTCHAIRS_API UChessPieceEffect_ChangeMoveset : public UChessPieceEffect
{
	GENERATED_BODY()

public:
	UChessPieceEffect_ChangeMoveset();

	/** The moveset to grant to the target piece */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	EMoveSet TargetMoveSet;

	/** Cached diagonal flag based on TargetMoveSet */
	UPROPERTY(BlueprintReadOnly, Category = "Effect")
	bool bDiagonal;

	/** Cached orthogonal flag based on TargetMoveSet */
	UPROPERTY(BlueprintReadOnly, Category = "Effect")
	bool bOrthogonal;

	virtual void OnApply_Implementation(AChessPieceActor* TargetPiece) override;
	virtual TSubclassOf<AChessMoveRule> GetMoveRuleOverride_Implementation() const override;
};
