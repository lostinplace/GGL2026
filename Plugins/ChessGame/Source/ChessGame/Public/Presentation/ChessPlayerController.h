#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Logic/ChessData.h"
#include "Presentation/ChessBoardActor.h"
#include "ChessPlayerController.generated.h"

/**
 * Handles Top-Down mouse interaction for Chess
 */
UCLASS()
class CHESSGAME_API AChessPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AChessPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// Actions
	void OnMouseClick();

	// Enhanced Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ClickAction;

	// Turn Validation
	/** Check if this player can interact (is it their turn in multiplayer) */
	bool CanInteract() const;

public:
	/** Get the assigned chess color for this player from PlayerState */
	UFUNCTION(BlueprintPure, Category = "Chess")
	EPieceColor GetAssignedColor() const;

	/** Check if the player has been assigned a color (multiplayer mode) */
	UFUNCTION(BlueprintPure, Category = "Chess")
	bool HasAssignedColor() const;

	/** Event triggered when the controller successfully retrieves its assigned color */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chess")
	void OnColorAssigned(EPieceColor Color);

	/** Get the color of the player whose turn it currently is */
	UFUNCTION(BlueprintPure, Category = "Chess")
	EPieceColor GetActivePlayerColor() const;

	/** Event triggered when the turn changes (active player changes) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chess")
	void OnTurnChanged(EPieceColor NewSideToMove);

	/** Single event to relay rich text status messages (Check, Mate, Draw, etc.) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chess")
	void OnGameStatusMessage(const FText& Message);
	// The board we are currently interacting with
	// We can find this dynamically or set it
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Chess")
	AChessBoardActor* CurrentBoard;

	// Raycast helper
	AChessBoardActor* FindBoardUnderCursor(FVector& OutHitLocation);

	// Server RPC to submit move
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SubmitMove(AChessBoardActor* Board, FChessMove Move);
};
