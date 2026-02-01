#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ChessData.h"
#include "ChessBoardState.h"
#include "ChessRuleSet.h"
#include "ChessGameModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoveApplied, const FChessMove&, Move);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPieceCaptured, int32, PieceId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnChanged, EPieceColor, SideToMove);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameEnded, bool, bIsDraw, EPieceColor, Winner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPieceMaskChanged, int32, PieceId, EPieceType, NewMask);

/**
 * The core game model. Orchestrates the game flow.
 */
UCLASS(BlueprintType)
class CHESSGAME_API UChessGameModel : public UObject
{
	GENERATED_BODY()

public:
	UChessGameModel();

	UPROPERTY(BlueprintReadOnly)
	UChessBoardState* BoardState;

	UPROPERTY(BlueprintReadOnly)
	UChessRuleSet* RuleSet;

	// Events
	UPROPERTY(BlueprintAssignable)
	FOnMoveApplied OnMoveApplied;

	UPROPERTY(BlueprintAssignable)
	FOnPieceCaptured OnPieceCaptured;

	UPROPERTY(BlueprintAssignable)
	FOnTurnChanged OnTurnChanged;

	UPROPERTY(BlueprintAssignable)
	FOnGameEnded OnGameEnded;

	UPROPERTY(BlueprintAssignable)
	FOnPieceMaskChanged OnPieceMaskChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCheckStatusChanged, bool, bInCheck, EPieceColor, SideInCheck);
	UPROPERTY(BlueprintAssignable)
	FOnCheckStatusChanged OnCheckStatusChanged;

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Configuration")
	EChessInitMode InitMode = EChessInitMode::Standard;

	// API
	UFUNCTION(BlueprintCallable)
	void InitializeGame();

	UFUNCTION(BlueprintCallable)
	bool TryApplyMove(FChessMove Move);

	UFUNCTION(BlueprintCallable)
	void GetLegalMovesForPiece(int32 PieceId, TArray<FChessMove>& OutMoves);

	UFUNCTION(BlueprintCallable)
	void GetLegalMovesForCoord(FBoardCoord Coord, TArray<FChessMove>& OutMoves);

	UFUNCTION(BlueprintCallable)
	void SetPieceMask(int32 PieceId, EPieceType NewMask);

protected:
	void ApplyMoveInternal(const FChessMove& Move);
};
