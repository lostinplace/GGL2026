#pragma once

#include "CoreMinimal.h"
#include "ChessData.generated.h"

// Enums

UENUM(BlueprintType)
enum class EPieceType : uint8
{
	Pawn,
	Knight,
	Bishop,
	Rook,
	Queen,
	King,
	None
};

UENUM(BlueprintType)
enum class EPieceColor : uint8
{
	White,
	Black
};

UENUM(BlueprintType)
enum class ESpecialMoveType : uint8
{
	Normal,
	Promotion,
	Castling,
	EnPassant
};

UENUM(BlueprintType)
enum class EChessInitMode : uint8
{
	Standard,
	Empty,
	Test_KingsOnly,
	Random960
};

/**
 * Represents a coordinate on the chess board (0-7, 0-7)
 */
USTRUCT(BlueprintType)
struct CHESSGAME_API FBoardCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 File = -1; // Column (0-7, A-H)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Rank = -1; // Row (0-7, 1-8)

	FBoardCoord() {}
	FBoardCoord(int32 InFile, int32 InRank) : File(InFile), Rank(InRank) {}

	bool operator==(const FBoardCoord& Other) const
	{
		return File == Other.File && Rank == Other.Rank;
	}

	bool operator!=(const FBoardCoord& Other) const
	{
		return !(*this == Other);
	}

	int32 ToIndex() const
	{
		return Rank * 8 + File;
	}

	static FBoardCoord FromIndex(int32 Index)
	{
		return FBoardCoord(Index % 8, Index / 8);
	}

	bool IsValid() const
	{
		return File >= 0 && File <= 7 && Rank >= 0 && Rank <= 7;
	}

	FString ToString() const
	{
		if (!IsValid()) return TEXT("Invalid");
		// Simple Algebraic-ish: (File, Rank)
		return FString::Printf(TEXT("(%d, %d)"), File, Rank);
	}
};

/**
 * Instance of a chess piece
 */
USTRUCT(BlueprintType)
struct CHESSGAME_API FPieceInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 PieceId = -1;

	UPROPERTY(BlueprintReadOnly)
	EPieceType Type = EPieceType::None;

	UPROPERTY(BlueprintReadOnly)
	EPieceColor Color = EPieceColor::White;

	UPROPERTY(BlueprintReadOnly)
	bool bHasMoved = false;

	FPieceInstance() {}
	FPieceInstance(int32 InId, EPieceType InType, EPieceColor InColor)
		: PieceId(InId), Type(InType), Color(InColor), bHasMoved(false) {}
};

/**
 * Represents a move in Chess
 */
USTRUCT(BlueprintType)
struct CHESSGAME_API FChessMove
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FBoardCoord From;

	UPROPERTY(BlueprintReadWrite)
	FBoardCoord To;

	UPROPERTY(BlueprintReadWrite)
	int32 MovingPieceId = -1;

	UPROPERTY(BlueprintReadWrite)
	int32 CapturedPieceId = -1; // -1 if no capture

	UPROPERTY(BlueprintReadWrite)
	ESpecialMoveType SpecialType = ESpecialMoveType::Normal;
	
	UPROPERTY(BlueprintReadWrite)
	EPieceType PromotionType = EPieceType::None; // Only valid if SpecialType == Promotion

	FChessMove() {}
};

/**
 * Minimal state needed to undo a move
 */
USTRUCT(BlueprintType)
struct CHESSGAME_API FMoveUndoRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FChessMove Move;

	UPROPERTY()
	FBoardCoord PreviousEnPassantTarget;

	UPROPERTY()
	bool bPreviousHasMoved = false; // For the moving piece

	UPROPERTY()
	bool bPreviousRookHasMoved = false; // Only relevant for Castling

	UPROPERTY()
	int32 CapturedPieceId = -1;
	
	UPROPERTY()
	FBoardCoord CapturedPieceCoord; // Where the piece was (might be different from To in EnPassant)

	FMoveUndoRecord() {}
};

/**
 * Replicated Board State
 */
USTRUCT(BlueprintType)
struct CHESSGAME_API FChessBoardStateData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<int32> Squares; // 64

	// For efficiency, we might not replicate the full Pieces map if we can rebuild it,
	// but rebuilding from just Squares loses "Moved" status and ID tracking if not careful.
	// We need Pieces too.
	// TMap is not directly supported in fast replication unless we wrap it or use arrays.
	// Since PieceId is key, we can use an array of structs.
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FPieceInstance> PiecesArray;

	UPROPERTY(BlueprintReadOnly)
	EPieceColor SideToMove = EPieceColor::White;

	UPROPERTY(BlueprintReadOnly)
	bool bHasEnPassantTarget = false;

	UPROPERTY(BlueprintReadOnly)
	FBoardCoord EnPassantTarget;

	// Counters
	UPROPERTY(BlueprintReadOnly)
	int32 HalfmoveClock = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 FullmoveNumber = 1;

	// Game Status
	UPROPERTY(BlueprintReadOnly)
	bool bIsGameOver = false;

	UPROPERTY(BlueprintReadOnly)
	EPieceColor Winner = EPieceColor::White; // Only valid if bIsGameOver and not draw (implied by context or extra bool)
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsDraw = false;

	FChessBoardStateData()
	{
		Squares.Init(-1, 64);
	}
};

