#include "Logic/ChessGameModel.h"

UChessGameModel::UChessGameModel()
{
}

void UChessGameModel::InitializeGame()
{
	BoardState = NewObject<UChessBoardState>(this);
	RuleSet = NewObject<UChessRuleSet>(this);
	RuleSet->Initialize(this);
	RuleSet->SetupInitialBoardState(BoardState, InitMode);

	OnTurnChanged.Broadcast(BoardState->SideToMove);
}

void UChessGameModel::GetLegalMovesForPiece(int32 PieceId, TArray<FChessMove>& OutMoves)
{
	if (BoardState && RuleSet)
	{
		RuleSet->GenerateLegalMoves(BoardState, PieceId, OutMoves);
	}
}

void UChessGameModel::GetLegalMovesForCoord(FBoardCoord Coord, TArray<FChessMove>& OutMoves)
{
	if (BoardState)
	{
		int32 PieceId = BoardState->GetPieceIdAt(Coord);
		if (PieceId != -1)
		{
			GetLegalMovesForPiece(PieceId, OutMoves);
		}
	}
}

bool UChessGameModel::TryApplyMove(FChessMove Move)
{
	if (!BoardState || !RuleSet) return false;

	// Validate ownership
	int32 MovingPieceId = BoardState->GetPieceIdAt(Move.From);
	if (MovingPieceId == -1 || MovingPieceId != Move.MovingPieceId) return false;

	const FPieceInstance* Piece = BoardState->Pieces.Find(MovingPieceId);
	if (!Piece || Piece->Color != BoardState->SideToMove) return false;

	// Validate legality
	// We can either regenerate moves or assume the UI passed a valid move from pseudo-moves
	// But `RuleSet->IsMoveLegal` is protected.
	// Best practice: Regenerate legal moves for this piece and check equality.
	TArray<FChessMove> LegalMoves;
	GetLegalMovesForPiece(MovingPieceId, LegalMoves);

	bool bValid = false;
	FChessMove ValidatedMove;
	for (const FChessMove& Legal : LegalMoves)
	{
		if (Legal.From == Move.From && Legal.To == Move.To)
		{
			// Check promotion type matching if applicable
			if (Legal.SpecialType == ESpecialMoveType::Promotion)
			{
				if (Legal.PromotionType == Move.PromotionType)
				{
					ValidatedMove = Legal;
					bValid = true;
					break;
				}
			}
			else
			{
				ValidatedMove = Legal;
				bValid = true;
				break;
			}
		}
	}

	if (!bValid) return false;

	ApplyMoveInternal(ValidatedMove);
	return true;
}

void UChessGameModel::ApplyMoveInternal(const FChessMove& Move)
{
	// Log move
	// UE_LOG(LogTemp, Log, TEXT("Move: %s -> %s"), *Move.From.ToString(), *Move.To.ToString());

	// Update Board State
	BoardState->SetPieceIdAt(Move.From, -1);
	BoardState->SetPieceIdAt(Move.To, Move.MovingPieceId);
	
	// Update Piece State
	if (FPieceInstance* Piece = BoardState->Pieces.Find(Move.MovingPieceId))
	{
		Piece->bHasMoved = true;
		if (Move.SpecialType == ESpecialMoveType::Promotion)
		{
			Piece->Type = Move.PromotionType;
		}
	}

	// Capture
	if (Move.CapturedPieceId != -1)
	{
		BoardState->RemovePiece(Move.CapturedPieceId);
		OnPieceCaptured.Broadcast(Move.CapturedPieceId);
	}

	// Special Moves
	if (Move.SpecialType == ESpecialMoveType::EnPassant)
	{
		FBoardCoord CapturedPawnCoord(Move.To.File, Move.From.Rank);
		// Note: CapturedPieceId is already handled above if we set it correctly in generator
		// Move generator sets CapturedPieceId for EnPassant too.
		// But we need to clear the square.
		BoardState->SetPieceIdAt(CapturedPawnCoord, -1);
	}
	else if (Move.SpecialType == ESpecialMoveType::Castling)
	{
		// Move Rook
		int32 Rank = Move.From.Rank;
		if (Move.To.File == 6) // King Side
		{
			int32 RookId = BoardState->GetPieceIdAt(FBoardCoord(7, Rank));
			if (RookId != -1)
			{
				BoardState->MovePiece(RookId, FBoardCoord(7, Rank), FBoardCoord(5, Rank));
			}
		}
		else if (Move.To.File == 2) // Queen Side
		{
			int32 RookId = BoardState->GetPieceIdAt(FBoardCoord(0, Rank));
			if (RookId != -1)
			{
				BoardState->MovePiece(RookId, FBoardCoord(0, Rank), FBoardCoord(3, Rank));
			}
		}
	}

	// Update En Passant Target
	BoardState->bHasEnPassantTarget = false;
	if (FPieceInstance* Piece = BoardState->Pieces.Find(Move.MovingPieceId))
	{
		if (Piece->Type == EPieceType::Pawn && FMath::Abs(Move.To.Rank - Move.From.Rank) == 2)
		{
			BoardState->bHasEnPassantTarget = true;
			BoardState->EnPassantTarget = FBoardCoord(Move.From.File, (Move.From.Rank + Move.To.Rank) / 2);
		}
	}

	// Switch Turn
	BoardState->SideToMove = (BoardState->SideToMove == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White;

	// Calculate Check Status
	bool bInCheck = RuleSet->IsKingInCheck(BoardState, BoardState->SideToMove);
	BoardState->bInCheck = bInCheck;

	// Broadcast updates
	OnMoveApplied.Broadcast(Move);
	OnTurnChanged.Broadcast(BoardState->SideToMove); // Notify Turn First
	OnCheckStatusChanged.Broadcast(bInCheck, BoardState->SideToMove); // Notify Check Status

	// Check Game End (Checkmate/Stalemate)
	bool bAnyLegalMove = false;
	for (const auto& Pair : BoardState->Pieces)
	{
		if (Pair.Value.Color == BoardState->SideToMove)
		{
			TArray<FChessMove> Moves;
			RuleSet->GenerateLegalMoves(BoardState, Pair.Key, Moves);
			if (Moves.Num() > 0)
			{
				bAnyLegalMove = true;
				break;
			}
		}
	}

	if (!bAnyLegalMove)
	{
		if (bInCheck)
		{
			// Checkmate
			// Winner is opposite side (who moved last)
			EPieceColor Winner = (BoardState->SideToMove == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White;
			
			BoardState->bIsGameOver = true;
			BoardState->bIsDraw = false;
			BoardState->Winner = Winner;
			
			OnGameEnded.Broadcast(false, Winner);
		}
		else
		{
			// Stalemate
			BoardState->bIsGameOver = true;
			BoardState->bIsDraw = true;
			
			OnGameEnded.Broadcast(true, EPieceColor::White); // Draw
		}
	}
}

void UChessGameModel::SetPieceMask(int32 PieceId, EPieceType NewMask)
{
	if (BoardState)
	{
		if (FPieceInstance* Piece = BoardState->Pieces.Find(PieceId))
		{
			Piece->MaskType = NewMask;
			OnPieceMaskChanged.Broadcast(PieceId, NewMask);
		}
	}
}
