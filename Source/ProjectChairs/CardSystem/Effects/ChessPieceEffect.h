// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ChessPieceEffect.generated.h"

class AChessPieceActor;
class AChessMoveRule;
class UChessPieceEffectComponent;

/**
 * Base class for chess piece effects that can be stacked on pieces.
 * Effects can modify movement rules and have configurable durations.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class PROJECTCHAIRS_API UChessPieceEffect : public UObject
{
	GENERATED_BODY()

public:
	UChessPieceEffect();

	/** Designer-set duration in turns. 0 = instant effect (executes OnApply but never goes on stack) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	int32 BaseDuration;

	/** Runtime tracking of remaining turns */
	UPROPERTY(BlueprintReadOnly, Category = "Effect")
	int32 RemainingDuration;

	/** Reference to the component managing this effect */
	UPROPERTY(BlueprintReadOnly, Category = "Effect")
	UChessPieceEffectComponent* OwningComponent;

	/** Returns true if this effect is instant (BaseDuration <= 0) */
	UFUNCTION(BlueprintCallable, Category = "Effect")
	bool IsInstant() const;

	/** Called when the effect is applied to a piece */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect")
	void OnApply(AChessPieceActor* TargetPiece);

	/** Called when the effect is removed or expires */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect")
	void OnRemove(AChessPieceActor* TargetPiece);

	/** Returns the move rule class this effect provides, or nullptr for no override */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect")
	TSubclassOf<AChessMoveRule> GetMoveRuleOverride() const;

	/** Initialize the effect with its owning component */
	void Initialize(UChessPieceEffectComponent* InOwningComponent);
};
