#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ChessData.h"
#include "ChessBoardState.generated.h"

/**
 * Holds the current state of the chess board.
 * Pure data container with helper accessors.
 */
UCLASS(BlueprintType)
class CHESSGAME_API UChessBoardState : public UObject
{
	GENERATED_BODY()

public:
	UChessBoardState();

	// Squares: 0-63. Stores PieceId or -1 if empty.
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> Squares;

	// Pieces: Map PieceId -> PieceInstance
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, FPieceInstance> Pieces;

	UPROPERTY(BlueprintReadOnly)
	EPieceColor SideToMove;

	// En Passant
	UPROPERTY(BlueprintReadOnly)
	bool bHasEnPassantTarget;

	UPROPERTY(BlueprintReadOnly)
	FBoardCoord EnPassantTarget;

	// Move counters
	UPROPERTY(BlueprintReadOnly)
	int32 HalfmoveClock;

	UPROPERTY(BlueprintReadOnly)
	int32 FullmoveNumber;

	// Game Status
	UPROPERTY(BlueprintReadOnly)
	bool bIsGameOver = false;

	UPROPERTY(BlueprintReadOnly)
	EPieceColor Winner = EPieceColor::White;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsDraw = false;

	UPROPERTY(BlueprintReadOnly)
	bool bInCheck = false;

	// Helpers
	UFUNCTION(BlueprintCallable)
	int32 GetPieceIdAt(FBoardCoord Coord) const;

	UFUNCTION(BlueprintCallable)
	void SetPieceIdAt(FBoardCoord Coord, int32 PieceId);

	// Utilities
	void InitializeEmpty();
	void AddPiece(int32 PieceId, EPieceType Type, EPieceColor Color, FBoardCoord Coord);
	void MovePiece(int32 PieceId, FBoardCoord From, FBoardCoord To);
	void RemovePiece(int32 PieceId);
	const FPieceInstance* GetPiece(int32 PieceId) const;

	// Replication Helpers
	FChessBoardStateData ToStruct() const;
	void FromStruct(const FChessBoardStateData& Data);
};
