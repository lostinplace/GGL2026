#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ChessData.h"
#include "ChessBoardState.h"
#include "MoveGenerators.generated.h"

/**
 * Base class for move generators.
 */
UCLASS(Abstract)
class CHESSGAME_API UMoveGeneratorBase : public UObject
{
	GENERATED_BODY()

public:
	virtual void GeneratePseudoMoves(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const PURE_VIRTUAL(UMoveGeneratorBase::GeneratePseudoMoves, );
	
protected:
	void AddMove(TArray<FChessMove>& OutMoves, FBoardCoord From, FBoardCoord To, const FPieceInstance& MovingPiece, int32 CapturedPieceId = -1) const;
	bool IsSameColor(const UChessBoardState* Board, FBoardCoord Coord, EPieceColor Color) const;
	bool IsEnemy(const UChessBoardState* Board, FBoardCoord Coord, EPieceColor Color) const;
	bool IsEmpty(const UChessBoardState* Board, FBoardCoord Coord) const;
};

/**
 * Generator for Sliding pieces (Rook, Bishop, Queen)
 */
UCLASS()
class CHESSGAME_API UMoveGenerator_Sliding : public UMoveGeneratorBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bDiagonal = false;

	UPROPERTY()
	bool bOrthogonal = false;

	virtual void GeneratePseudoMoves(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const override;
};

/**
 * Generator for Knights
 */
UCLASS()
class CHESSGAME_API UMoveGenerator_Knight : public UMoveGeneratorBase
{
	GENERATED_BODY()

public:
	virtual void GeneratePseudoMoves(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const override;
};

/**
 * Generator for Pawns
 */
UCLASS()
class CHESSGAME_API UMoveGenerator_Pawn : public UMoveGeneratorBase
{
	GENERATED_BODY()

public:
	virtual void GeneratePseudoMoves(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const override;
};

/**
 * Generator for King
 */
UCLASS()
class CHESSGAME_API UMoveGenerator_King : public UMoveGeneratorBase
{
	GENERATED_BODY()

public:
	virtual void GeneratePseudoMoves(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const override;
};
