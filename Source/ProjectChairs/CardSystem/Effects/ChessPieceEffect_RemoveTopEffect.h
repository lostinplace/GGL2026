// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChessPieceEffect.h"
#include "ChessPieceEffect_RemoveTopEffect.generated.h"

/**
 * Instant effect that removes the top effect from the piece's effect stack.
 * Since BaseDuration is 0, this effect executes OnApply but is never added to the stack.
 */
UCLASS(Blueprintable)
class PROJECTCHAIRS_API UChessPieceEffect_RemoveTopEffect : public UChessPieceEffect
{
	GENERATED_BODY()

public:
	UChessPieceEffect_RemoveTopEffect();

	virtual void OnApply_Implementation(AChessPieceActor* TargetPiece) override;
};
