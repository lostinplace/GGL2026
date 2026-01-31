// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CardDataAsset.h"
#include "CardObject.generated.h"

/**
 * Runtime instance of a card, referencing its data asset.
 */
UCLASS(BlueprintType)
class PROJECTCHAIRS_API UCardObject : public UObject
{
	GENERATED_BODY()

public:
	UCardObject();

	/** Initialize this card from a data asset */
	UFUNCTION(BlueprintCallable, Category = "Card")
	void InitializeFromDataAsset(UCardDataAsset* InCardData);

	/** Get the card data asset */
	UFUNCTION(BlueprintCallable, Category = "Card")
	UCardDataAsset* GetCardData() const { return CardData; }

	/** Get the display name */
	UFUNCTION(BlueprintCallable, Category = "Card")
	FText GetDisplayName() const;

	/** Get the move set */
	UFUNCTION(BlueprintCallable, Category = "Card")
	EMoveSet GetMoveSet() const;

	/** Get the target type */
	UFUNCTION(BlueprintCallable, Category = "Card")
	ETargetType GetTargetType() const;

	/** Get the effect class */
	UFUNCTION(BlueprintCallable, Category = "Card")
	TSubclassOf<UCardEffect> GetEffectClass() const;

	/** Get the icon (will load if not already loaded) */
	UFUNCTION(BlueprintCallable, Category = "Card")
	UTexture2D* GetIcon() const;

protected:
	/** Reference to the card data asset */
	UPROPERTY(BlueprintReadOnly, Category = "Card")
	UCardDataAsset* CardData;
};
