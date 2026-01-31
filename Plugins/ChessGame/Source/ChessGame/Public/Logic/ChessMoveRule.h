#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChessData.h"
#include "ChessBoardState.h"
#include "ChessMoveRule.generated.h"

/**
 * Base class for move rules. Note: Inherits from AActor.
 */
UCLASS(Abstract, Blueprintable)
class CHESSGAME_API AChessMoveRule : public AActor
{
	GENERATED_BODY()

public:
	AChessMoveRule();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void GenerateMoves(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const;
	virtual void GenerateMoves_Implementation(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const PURE_VIRTUAL(AChessMoveRule::GenerateMoves_Implementation, );

protected:
	// Helpers commonly used in move generation
	void AddMove(TArray<FChessMove>& OutMoves, FBoardCoord From, FBoardCoord To, const FPieceInstance& MovingPiece, int32 CapturedPieceId = -1) const;
	bool IsSameColor(const UChessBoardState* Board, FBoardCoord Coord, EPieceColor Color) const;
	bool IsEnemy(const UChessBoardState* Board, FBoardCoord Coord, EPieceColor Color) const;
	bool IsEmpty(const UChessBoardState* Board, FBoardCoord Coord) const;
};

/**
 * Rule for Sliding pieces (Rook, Bishop, Queen)
 */
UCLASS()
class CHESSGAME_API AChessMoveRule_Sliding : public AChessMoveRule
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDiagonal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOrthogonal = false;

	virtual void GenerateMoves_Implementation(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const override;
};

/**
 * Rule for Knights
 */
UCLASS()
class CHESSGAME_API AChessMoveRule_Knight : public AChessMoveRule
{
	GENERATED_BODY()

public:
	virtual void GenerateMoves_Implementation(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const override;
};

/**
 * Rule for Pawns
 */
UCLASS()
class CHESSGAME_API AChessMoveRule_Pawn : public AChessMoveRule
{
	GENERATED_BODY()

public:
	virtual void GenerateMoves_Implementation(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const override;
};

/**
 * Rule for King
 */
UCLASS()
class CHESSGAME_API AChessMoveRule_King : public AChessMoveRule
{
	GENERATED_BODY()

public:
	virtual void GenerateMoves_Implementation(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const override;
};
