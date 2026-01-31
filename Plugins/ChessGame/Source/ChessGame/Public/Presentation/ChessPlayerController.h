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

	/** Get the assigned chess color for this player from PlayerState */
	EPieceColor GetAssignedColor() const;

	/** Check if the player has been assigned a color (multiplayer mode) */
	bool HasAssignedColor() const;

public:
	// The board we are currently interacting with
	// We can find this dynamically or set it
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Chess")
	AChessBoardActor* CurrentBoard;

	// Raycast helper
	AChessBoardActor* FindBoardUnderCursor(FVector& OutHitLocation);
};
