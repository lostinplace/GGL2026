// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Presentation/ChessPlayerController.h"
#include "ProjectChairsPlayerState.h"
#include "Logic/ChessData.h"
#include "ProjectChairsChessPlayerController.generated.h"

class AProjectChairsPlayerState;
class AChessPieceActor;
class UCardObject;

/**
 * Project Chairs specific chess player controller that handles card targeting.
 * Extends the base chess controller to intercept clicks for card effects.
 */
UCLASS()
class PROJECTCHAIRS_API AProjectChairsChessPlayerController : public AChessPlayerController
{
	GENERATED_BODY()

public:
	AProjectChairsChessPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Our click handler that checks for card targeting first */
	void OnProjectChairsMouseClick();

	/**
	 * Attempts to handle a click as a card targeting action.
	 * @param HitLocation The world location that was clicked
	 * @return True if the click was handled as card targeting, false to fall through to normal chess logic
	 */
	bool TryHandleCardTargeting(const FVector& HitLocation);

	/**
	 * Finds a chess piece at the given world location.
	 * @param HitLocation The world location to check
	 * @return The chess piece at that location, or nullptr if none found
	 */
	AChessPieceActor* FindPieceAtLocation(const FVector& HitLocation);

	/** Get the ProjectChairs-specific player state */
	AProjectChairsPlayerState* GetProjectChairsPlayerState() const;

	/** Called when the selected card changes in PlayerState */
	UFUNCTION()
	void OnSelectedCardChanged(UCardObject* SelectedCard, ECardInteractionMode Mode);

	/** Updates the card target highlights on all pieces based on the selected card */
	void UpdateCardTargetHighlights(UCardObject* SelectedCard);

	/** Clears all card target highlights */
	void ClearCardTargetHighlights();

	/** Called when a chess move is made - resets card played status for the player whose turn it now is */
	void CheckAndResetCardPlayedOnTurnChange();

	/** Tracks the last known side to move, for detecting turn changes */
	EPieceColor LastKnownSideToMove;

	/** Whether we've initialized LastKnownSideToMove */
	bool bHasInitializedTurnTracking;
};
