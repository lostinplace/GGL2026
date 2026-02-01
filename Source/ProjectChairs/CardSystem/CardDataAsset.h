// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CardTypes.h"
#include "CardEffect.h"
#include "Effects/ChessPieceEffect.h"
#include "CardDataAsset.generated.h"

/**
 * Data asset representing a single card with its properties.
 */
UCLASS(BlueprintType)
class PROJECTCHAIRS_API UCardDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Display name shown to the player */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FText DisplayName;

	/** Icon displayed for this card */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TSoftObjectPtr<UTexture2D> Icon;

	/** The movement pattern associated with this card */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	EMoveSet MoveSet;

	/** The effect class to instantiate when this card is played */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TSubclassOf<UCardEffect> EffectClass;

	/** Who this card can target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ETargetType TargetType;

	/** The chess piece effect to apply when targeting a chess piece */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TSubclassOf<UChessPieceEffect> ChessPieceEffectClass;
};
