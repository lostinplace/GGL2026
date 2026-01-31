#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
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

public:
	// The board we are currently interacting with
	// We can find this dynamically or set it
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Chess")
	AChessBoardActor* CurrentBoard;

	// Raycast helper
	AChessBoardActor* FindBoardUnderCursor(FVector& OutHitLocation);
};
