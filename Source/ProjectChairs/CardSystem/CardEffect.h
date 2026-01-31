// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CardEffect.generated.h"

/**
 * Base class for card effects. Subclass this to create specific card effects.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class PROJECTCHAIRS_API UCardEffect : public UObject
{
	GENERATED_BODY()

public:
	UCardEffect();

	/** Execute the card effect on the target */
	UFUNCTION(BlueprintNativeEvent, Category = "Card Effect")
	void ExecuteEffect(AActor* Instigator, AActor* Target);
	virtual void ExecuteEffect_Implementation(AActor* Instigator, AActor* Target);
};
