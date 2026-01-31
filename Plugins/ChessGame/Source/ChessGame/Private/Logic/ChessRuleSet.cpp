#include "Logic/ChessRuleSet.h"
#include "Logic/ChessMoveRule.h"

UChessRuleSet::UChessRuleSet()
{
}

void UChessRuleSet::Initialize(UObject* WorldContextObject)
{
	// Default mappings if not set
	if (MoveRuleClasses.Num() == 0)
	{
		MoveRuleClasses.Add(EPieceType::Rook, AChessMoveRule_Sliding::StaticClass());
		MoveRuleClasses.Add(EPieceType::Bishop, AChessMoveRule_Sliding::StaticClass());
		MoveRuleClasses.Add(EPieceType::Queen, AChessMoveRule_Sliding::StaticClass());
		MoveRuleClasses.Add(EPieceType::Knight, AChessMoveRule_Knight::StaticClass());
		MoveRuleClasses.Add(EPieceType::Pawn, AChessMoveRule_Pawn::StaticClass());
		MoveRuleClasses.Add(EPieceType::King, AChessMoveRule_King::StaticClass());
	}
	
	// Spawn instances
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : GetWorld();
	if (!World) return;
	// Wait, RuleSet is UObject. GetWorld might return null or outer's world.
	// If RuleSet is owned by GameModel (UObject), GetWorld() requires implementation or outer chain.
	
	// Assuming RuleSet is created within an Actor's world context (GameModel created in BoardActor).
	
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;

	for (const auto& Pair : MoveRuleClasses)
	{
		if (Pair.Value)
		{
			AChessMoveRule* Rule = World->SpawnActor<AChessMoveRule>(Pair.Value, Params);
			if (Rule)
			{
				// Configure Sliding rules specifically if using the generic class
				if (AChessMoveRule_Sliding* Sliding = Cast<AChessMoveRule_Sliding>(Rule))
				{
					if (Pair.Key == EPieceType::Rook) Sliding->bOrthogonal = true;
					if (Pair.Key == EPieceType::Bishop) Sliding->bDiagonal = true;
					if (Pair.Key == EPieceType::Queen) { Sliding->bOrthogonal = true; Sliding->bDiagonal = true; }
				}
				MoveRules.Add(Pair.Key, Rule);
			}
		}
	}
}

void UChessRuleSet::SetupInitialBoardState(UChessBoardState* BoardState, EChessInitMode InitMode)
{
	BoardState->InitializeEmpty();

	// Helper to add pieces
	int32 NextId = 0;
	auto AddPiece = [&](EPieceType Type, EPieceColor Color, int32 File, int32 Rank) {
		BoardState->AddPiece(NextId++, Type, Color, FBoardCoord(File, Rank));
	};

	if (InitMode == EChessInitMode::Standard)
	{
		// White Pieces (Rank 0 and 1)
		for (int i = 0; i < 8; ++i) AddPiece(EPieceType::Pawn, EPieceColor::White, i, 1);
		AddPiece(EPieceType::Rook, EPieceColor::White, 0, 0);
		AddPiece(EPieceType::Knight, EPieceColor::White, 1, 0);
		AddPiece(EPieceType::Bishop, EPieceColor::White, 2, 0);
		AddPiece(EPieceType::Queen, EPieceColor::White, 3, 0);
		AddPiece(EPieceType::King, EPieceColor::White, 4, 0);
		AddPiece(EPieceType::Bishop, EPieceColor::White, 5, 0);
		AddPiece(EPieceType::Knight, EPieceColor::White, 6, 0);
		AddPiece(EPieceType::Rook, EPieceColor::White, 7, 0);

		// Black Pieces (Rank 7 and 6)
		for (int i = 0; i < 8; ++i) AddPiece(EPieceType::Pawn, EPieceColor::Black, i, 6);
		AddPiece(EPieceType::Rook, EPieceColor::Black, 0, 7);
		AddPiece(EPieceType::Knight, EPieceColor::Black, 1, 7);
		AddPiece(EPieceType::Bishop, EPieceColor::Black, 2, 7);
		AddPiece(EPieceType::Queen, EPieceColor::Black, 3, 7);
		AddPiece(EPieceType::King, EPieceColor::Black, 4, 7);
		AddPiece(EPieceType::Bishop, EPieceColor::Black, 5, 7);
		AddPiece(EPieceType::Knight, EPieceColor::Black, 6, 7);
		AddPiece(EPieceType::Rook, EPieceColor::Black, 7, 7);
	}
	else if (InitMode == EChessInitMode::Test_KingsOnly)
	{
		// Just Kings for testing
		AddPiece(EPieceType::King, EPieceColor::White, 4, 0);
		AddPiece(EPieceType::King, EPieceColor::Black, 4, 7);
	}
	// Empty: do nothing
}

void UChessRuleSet::GeneratePseudoLegalMoves(const UChessBoardState* Board, int32 PieceId, TArray<FChessMove>& OutMoves)
{
	const FPieceInstance* Piece = Board->Pieces.Find(PieceId);
	if (!Piece) return;

	if (AChessMoveRule** RulePtr = MoveRules.Find(Piece->Type))
	{
		AChessMoveRule* Rule = *RulePtr;
		if (!Rule) return;

		// Find coord
		FBoardCoord From;
		bool bFound = false;
		for (int32 i = 0; i < 64; ++i)
		{
			if (Board->Squares[i] == PieceId)
			{
				From = FBoardCoord::FromIndex(i);
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			Rule->GenerateMoves(Board, From, *Piece, OutMoves);
		}
	}
}

void UChessRuleSet::GenerateLegalMoves(const UChessBoardState* Board, int32 PieceId, TArray<FChessMove>& OutMoves)
{
	TArray<FChessMove> PseudoMoves;
	GeneratePseudoLegalMoves(Board, PieceId, PseudoMoves);

	for (const FChessMove& Move : PseudoMoves)
	{
		if (IsMoveLegal(Board, Move))
		{
			OutMoves.Add(Move);
		}
	}
}

bool UChessRuleSet::IsMoveLegal(const UChessBoardState* Board, const FChessMove& Move)
{
	// 1. Clone board
	UChessBoardState* TempBoard = NewObject<UChessBoardState>();
	TempBoard->Squares = Board->Squares;
	TempBoard->Pieces = Board->Pieces;
	TempBoard->SideToMove = Board->SideToMove;
	TempBoard->bHasEnPassantTarget = Board->bHasEnPassantTarget;
	TempBoard->EnPassantTarget = Board->EnPassantTarget;
	
	// Apply Move to TempBoard
	TempBoard->SetPieceIdAt(Move.From, -1);
	TempBoard->SetPieceIdAt(Move.To, Move.MovingPieceId);
	
	// Handle capture
	if (Move.CapturedPieceId != -1)
	{
		TempBoard->RemovePiece(Move.CapturedPieceId);
	}
	// Note: EnPassant capture removes a different pawn
	if (Move.SpecialType == ESpecialMoveType::EnPassant)
	{
		FBoardCoord CapturedCoord(Move.To.File, Move.From.Rank);
		TempBoard->SetPieceIdAt(CapturedCoord, -1);
		// Assuming proper captured piece ID handling in generator/logic
	}
	
	// Check if King is in Check
	const FPieceInstance* MovingPiece = Board->Pieces.Find(Move.MovingPieceId);
	if (MovingPiece)
	{
		return !IsKingInCheck(TempBoard, MovingPiece->Color);
	}
	
	return false;
}

FBoardCoord UChessRuleSet::FindKing(const UChessBoardState* Board, EPieceColor Color) const
{
	for (const auto& Pair : Board->Pieces)
	{
		if (Pair.Value.Type == EPieceType::King && Pair.Value.Color == Color)
		{
			// Need location
			for (int32 i = 0; i < 64; ++i)
			{
				if (Board->Squares[i] == Pair.Key)
				{
					return FBoardCoord::FromIndex(i);
				}
			}
		}
	}
	return FBoardCoord(); // Invalid
}

bool UChessRuleSet::IsKingInCheck(const UChessBoardState* Board, EPieceColor Color)
{
	FBoardCoord KingPos = FindKing(Board, Color);
	if (!KingPos.IsValid()) return false; 

	EPieceColor EnemyColor = (Color == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White;

	for (const auto& Pair : Board->Pieces)
	{
		if (Pair.Value.Color == EnemyColor)
		{
			if (AChessMoveRule** RulePtr = MoveRules.Find(Pair.Value.Type))
			{
				AChessMoveRule* Rule = *RulePtr;
				if (!Rule) continue; 

				// Get location of enemy
				FBoardCoord EnemyPos;
				bool bFound = false;
				for (int32 i = 0; i < 64; ++i)
				{
					if (Board->Squares[i] == Pair.Value.PieceId)
					{
						EnemyPos = FBoardCoord::FromIndex(i);
						bFound = true;
						break;
					}
				}

				if (bFound)
				{
					TArray<FChessMove> Moves;
					Rule->GenerateMoves(Board, EnemyPos, Pair.Value, Moves);
					for (const FChessMove& Move : Moves)
					{
						if (Move.To == KingPos)
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}
