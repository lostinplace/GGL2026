#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ChessBoardState.h"
#include "MoveGenerators.h"
#include "ChessRuleSet.generated.h"

/**
 * Defines the rules of Chess.
 * Handles move generation and validation.
 */
UCLASS(BlueprintType)
class CHESSGAME_API UChessRuleSet : public UObject
{
	GENERATED_BODY()

public:
	UChessRuleSet();

	UFUNCTION(BlueprintCallable)
	void Initialize(UObject* WorldContextObject = nullptr);

	// Setup
	UFUNCTION(BlueprintCallable)
	void SetupInitialBoardState(UChessBoardState* BoardState, EChessInitMode InitMode = EChessInitMode::Standard);

	// Move Generation
	UFUNCTION(BlueprintCallable)
	void GenerateLegalMoves(const UChessBoardState* Board, int32 PieceId, TArray<FChessMove>& OutMoves);

	UFUNCTION(BlueprintCallable)
	bool IsKingInCheck(const UChessBoardState* Board, EPieceColor Color);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EPieceType, TSubclassOf<class AChessMoveRule>> MoveRuleClasses;

	UPROPERTY()
	TMap<EPieceType, class AChessMoveRule*> MoveRules;

	void GeneratePseudoLegalMoves(const UChessBoardState* Board, int32 PieceId, TArray<FChessMove>& OutMoves);
	bool IsMoveLegal(const UChessBoardState* Board, const FChessMove& Move);

	// Helpers
	FBoardCoord FindKing(const UChessBoardState* Board, EPieceColor Color) const;
};
