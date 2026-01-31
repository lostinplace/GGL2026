// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Logic/ChessData.h"
#include "ChessPieceEffectComponent.generated.h"

class UChessPieceEffect;
class AChessMoveRule;
class AChessPieceActor;

/**
 * Component that manages a stack of effects on a chess piece.
 * Effects are stored in LIFO order (index 0 = bottom, last index = top).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTCHAIRS_API UChessPieceEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UChessPieceEffectComponent();

	/** The stack of active effects (index 0 = bottom, last = top) */
	UPROPERTY(BlueprintReadOnly, Category = "Effects")
	TArray<UChessPieceEffect*> EffectStack;

	/** The original move rule class before any effects were applied */
	UPROPERTY(BlueprintReadOnly, Category = "Effects")
	TSubclassOf<AChessMoveRule> OriginalMoveRuleClass;

	/**
	 * Creates and applies an effect of the specified class.
	 * Instant effects (BaseDuration <= 0) execute OnApply but are not added to the stack.
	 * @return The created effect, or nullptr if creation failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	UChessPieceEffect* ApplyEffect(TSubclassOf<UChessPieceEffect> EffectClass);

	/** Removes the top effect from the stack */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	void RemoveTopEffect();

	/** Removes a specific effect from the stack */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	void RemoveEffect(UChessPieceEffect* Effect);

	/**
	 * Returns the effective move rule class based on the effect stack.
	 * Traverses the stack top-to-bottom, returning the first non-null override.
	 * If no override is found, returns the original move rule class.
	 */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	TSubclassOf<AChessMoveRule> GetEffectiveMoveRuleClass() const;

	/**
	 * Called when the turn changes. Decrements duration when the piece's turn ends.
	 * @param NewTurnColor The color of the player whose turn is starting
	 */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	void OnTurnChanged(EPieceColor NewTurnColor);

	/**
	 * Refreshes the move rule instance based on the effective move rule class.
	 * Destroys the old instance and spawns a new one if the class has changed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	void RefreshMoveRule();

	/** Gets the owning chess piece actor */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	AChessPieceActor* GetOwningPiece() const;

protected:
	virtual void BeginPlay() override;

private:
	/** Internal helper to remove an effect at a specific index */
	void RemoveEffectAtIndex(int32 Index);

	/** Cached reference to the owning piece */
	UPROPERTY()
	AChessPieceActor* CachedOwningPiece;
};
