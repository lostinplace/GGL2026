// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Presentation/ChessPlayerController.h"
#include "ProjectChairsChessPlayerController.generated.h"

class AProjectChairsPlayerState;
class AProjectChairsChessPieceActor;

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
	AProjectChairsChessPieceActor* FindPieceAtLocation(const FVector& HitLocation);

	/** Get the ProjectChairs-specific player state */
	AProjectChairsPlayerState* GetProjectChairsPlayerState() const;
};
