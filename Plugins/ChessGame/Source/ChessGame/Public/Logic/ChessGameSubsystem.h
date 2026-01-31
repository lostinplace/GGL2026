#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Logic/ChessData.h"
#include "Logic/ChessGameModel.h"
#include "ChessGameSubsystem.generated.h"

// Interaction State
UENUM(BlueprintType)
enum class EChessInteractionState : uint8
{
	Idle,
	PieceSelected,
	Animating, // Optional
	GameOver
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectionUpdated, FBoardCoord, SelectedCoord);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionStateChanged, EChessInteractionState, NewState);

/**
 * Subsystem to manage the global state of the Chess session.
 * Tracks selection, turn, and active game model.
 */
UCLASS()
class CHESSGAME_API UChessGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Active Game ---
	
	// Register the active game model (usually called by BoardActor)
	UFUNCTION(BlueprintCallable, Category = "Chess Subsystem")
	void RegisterGame(UChessGameModel* InGameModel);

	UFUNCTION(BlueprintPure, Category = "Chess Subsystem")
	UChessGameModel* GetActiveGame() const { return ActiveGameModel; }

	// --- State Machine ---

	UPROPERTY(BlueprintReadOnly, Category = "Chess Subsystem")
	EChessInteractionState CurrentInteractionState;

	UPROPERTY(BlueprintReadOnly, Category = "Chess Subsystem")
	FBoardCoord CurrentSelectedCoord;

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "Chess Subsystem")
	FOnSelectionUpdated OnSelectionUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Chess Subsystem")
	FOnInteractionStateChanged OnInteractionStateChanged;

	// --- Actions ---

	UFUNCTION(BlueprintCallable, Category = "Chess Subsystem")
	void SetSelectedCoord(FBoardCoord Coord);

	UFUNCTION(BlueprintCallable, Category = "Chess Subsystem")
	void ClearSelection();

	// Helpers
	UFUNCTION(BlueprintPure, Category = "Chess Subsystem")
	EPieceColor GetSideToMove() const;

private:
	UPROPERTY()
	UChessGameModel* ActiveGameModel;
};
