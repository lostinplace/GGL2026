#include "Presentation/ChessBoardActor.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AChessBoardActor::AChessBoardActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SelectedCoord = FBoardCoord(-1, -1);
}

void AChessBoardActor::BeginPlay()
{
	Super::BeginPlay();

	// Initialize Model
	GameModel = NewObject<UChessGameModel>(this);
	GameModel->InitializeGame();

	// Bind Events
	GameModel->OnMoveApplied.AddDynamic(this, &AChessBoardActor::OnMoveApplied);
	GameModel->OnPieceCaptured.AddDynamic(this, &AChessBoardActor::OnPieceCaptured);
	GameModel->OnGameEnded.AddDynamic(this, &AChessBoardActor::OnGameEnded);

	// Initial Sync
	if (HasAuthority())
	{
		ReplicatedState = GameModel->BoardState->ToStruct();
	}
	SyncVisuals();
}

void AChessBoardActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChessBoardActor, ReplicatedState);
}

FVector AChessBoardActor::CoordToWorld(FBoardCoord Coord) const
{
	float OffsetX = SquareSize * 0.5f;
	float OffsetY = SquareSize * 0.5f;

	if (bCenterBoard)
	{
		// Center is (0,0). Board width is 8 * Size. Corner A1 starts at -4 * Size.
		OffsetX -= SquareSize * 4.0f;
		OffsetY -= SquareSize * 4.0f;
	}

	float X = Coord.File * SquareSize + OffsetX;
	float Y = Coord.Rank * SquareSize + OffsetY;
	return GetActorTransform().TransformPosition(FVector(X, Y, PieceHeightOffset));
}

FBoardCoord AChessBoardActor::WorldToCoord(FVector WorldLoc) const
{
	FVector LocalLoc = GetActorTransform().InverseTransformPosition(WorldLoc);
	
	float OffsetX = 0.0f;
	float OffsetY = 0.0f;
	if (bCenterBoard)
	{
		OffsetX = SquareSize * 4.0f;
		OffsetY = SquareSize * 4.0f;
	}

	int32 File = FMath::FloorToInt((LocalLoc.X + OffsetX) / SquareSize);
	int32 Rank = FMath::FloorToInt((LocalLoc.Y + OffsetY) / SquareSize);
	return FBoardCoord(File, Rank);
}

void AChessBoardActor::HandleSquareClicked(FBoardCoord Coord)
{
	if (!Coord.IsValid()) return;

	// Check if we selected a piece or a target
	int32 ClickedPieceId = GameModel->BoardState->GetPieceIdAt(Coord);
	
	// If we have a selection, try to move to clicked square
	if (SelectedCoord.IsValid())
	{
		// Check if we clicked the same square (deselect)
		if (SelectedCoord == Coord)
		{
			SelectedCoord = FBoardCoord(-1, -1);
			OnClearHighlights();
			return;
		}

		// Try to move
		// Wait, did we click a friendly piece? If so, select it instead.
		// Unless it's a valid move (e.g. castling might involve clicking rook? No, standard click-click is move king)
		// Or capture.
		
		// If clicking own piece, switch selection usually, unless we want to support click-drag.
		// Standard RTS/Chess click:
		// 1. Select A
		// 2. Click B. 
		//    If B can be reached from A -> Move.
		//    Else if B is own piece -> Select B.
		//    Else -> Deselect.

		bool bMoved = false;
		
		// Build a move. We need to find if there is a legal move from Selected to Coord.
		TArray<FChessMove> LegalMoves;
		GameModel->GetLegalMovesForCoord(SelectedCoord, LegalMoves);

		FChessMove TargetMove;
		bool bIsLegal = false;
		for (const FChessMove& Move : LegalMoves)
		{
			if (Move.To == Coord)
			{
				TargetMove = Move;
				bIsLegal = true;
				break;
			}
		}

		if (bIsLegal)
		{
			// Promotion UI Check
			if (TargetMove.SpecialType == ESpecialMoveType::Promotion)
			{
				// Simplest MVP: Auto-promote to Queen
				TargetMove.PromotionType = EPieceType::Queen;
				// TODO: Logic to ask UI
			}

			// Call Server RPC
			Server_TryMove(TargetMove);
			// Optimistic local update? For now, wait for server/multicast to avoid desync
			// Deselect immediately to frame UI
			SelectedCoord = FBoardCoord(-1, -1);
			OnClearHighlights();
		}
		
		if (!bMoved)
		{
			// If not a move, maybe selecting a different piece?
			if (ClickedPieceId != -1)
			{
				const FPieceInstance* ClickedPiece = GameModel->BoardState->GetPiece(ClickedPieceId);
				if (ClickedPiece && ClickedPiece->Color == GameModel->BoardState->SideToMove)
				{
					SelectedCoord = Coord;
					OnClearHighlights();
					GameModel->GetLegalMovesForCoord(SelectedCoord, LegalMoves);
					OnHighlightMoves(LegalMoves);
				}
				else
				{
					// Clicked empty or enemy, but not a valid move -> Deselect
					SelectedCoord = FBoardCoord(-1, -1);
					OnClearHighlights();
				}
			}
			else
			{
				SelectedCoord = FBoardCoord(-1, -1);
				OnClearHighlights();
			}
		}
	}
	else
	{
		// No selection, try to select
		if (ClickedPieceId != -1)
		{
			const FPieceInstance* ClickedPiece = GameModel->BoardState->GetPiece(ClickedPieceId);
			if (ClickedPiece && ClickedPiece->Color == GameModel->BoardState->SideToMove)
			{
				SelectedCoord = Coord;
				TArray<FChessMove> LegalMoves;
				GameModel->GetLegalMovesForCoord(SelectedCoord, LegalMoves);
				OnHighlightMoves(LegalMoves);
			}
		}
	}
}

void AChessBoardActor::SyncVisuals()
{
	// Destroy all existing
	for (auto& Pair : PieceActors)
	{
		if (Pair.Value) Pair.Value->Destroy();
	}
	PieceActors.Empty();

	// Spawn new
	for (const auto& Pair : GameModel->BoardState->Pieces)
	{
		// Find Coord
		for (int32 i = 0; i < 64; ++i)
		{
			if (GameModel->BoardState->Squares[i] == Pair.Value.PieceId)
			{
				SpawnPieceActor(Pair.Value.PieceId, Pair.Value.Type, Pair.Value.Color, FBoardCoord::FromIndex(i));
				break;
			}
		}
	}
}

void AChessBoardActor::SpawnPieceActor(int32 PieceId, EPieceType Type, EPieceColor Color, FBoardCoord Coord)
{
	if (!StyleSet) return;
	
	TSubclassOf<AChessPieceActor> Class = StyleSet->GetPieceClass(Color, Type);
	if (Class)
	{
		FVector Loc = CoordToWorld(Coord);
		FActorSpawnParameters Params;
		// Keep relative rotation? Or enforce forward?
		// Usually pieces face opponent.
		FRotator Rot = FRotator::ZeroRotator;
		if (Color == EPieceColor::Black) Rot.Yaw = 180.0f;

		AChessPieceActor* NewPiece = GetWorld()->SpawnActor<AChessPieceActor>(Class, Loc, Rot, Params);
		if (NewPiece)
		{
			NewPiece->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			NewPiece->Init(PieceId, Type, Color);
			PieceActors.Add(PieceId, NewPiece);
		}
	}
}

void AChessBoardActor::OnMoveApplied(const FChessMove& Move)
{
	if (AChessPieceActor** ActorPtr = PieceActors.Find(Move.MovingPieceId))
	{
		AChessPieceActor* Actor = *ActorPtr;
		if (Actor)
		{
			// Animate? For now teleport or use Lerp in handling
			// We can broadcast event to Actor to handle its own movement visualization (e.g. slide)
			Actor->SetActorLocation(CoordToWorld(Move.To));
			Actor->OnMoved(Move.From, Move.To, Move.SpecialType);
			
			// Handle Promotion Visuals
			if (Move.SpecialType == ESpecialMoveType::Promotion)
			{
				// Replace actor or change mesh?
				// Easiest is to destroy and respawn since class changes (Pawn -> Queen)
				PieceActors.Remove(Move.MovingPieceId);
				Actor->Destroy();
				
				// Respawn
				// Note: Piece type in BoardState is already updated by now? 
				// Yes, OnMoveApplied is broadcast after state update.
				// But we are passing Move struct which has PromotionType.
				SpawnPieceActor(Move.MovingPieceId, Move.PromotionType, Actor->Color, Move.To);
			}

			// Handle Castling (Rook Move)
			if (Move.SpecialType == ESpecialMoveType::Castling)
			{
				// Identify Rook
				FBoardCoord RookFrom, RookTo;
				if (Move.To.File == 6) { RookFrom = FBoardCoord(7, Move.From.Rank); RookTo = FBoardCoord(5, Move.From.Rank); }
				else { RookFrom = FBoardCoord(0, Move.From.Rank); RookTo = FBoardCoord(3, Move.From.Rank); }
				
				// We need Rook ID. Scan actors? Or query model.
				int32 RookId = GameModel->BoardState->GetPieceIdAt(RookTo); // It's already moved on board state
				if (AChessPieceActor** RookActorPtr = PieceActors.Find(RookId))
				{
					(*RookActorPtr)->SetActorLocation(CoordToWorld(RookTo));
					(*RookActorPtr)->OnMoved(RookFrom, RookTo, ESpecialMoveType::Normal); // Treat rook move as normal
				}
			}
		}
	}
}

void AChessBoardActor::OnPieceCaptured(int32 PieceId)
{
	if (AChessPieceActor** ActorPtr = PieceActors.Find(PieceId))
	{
		AChessPieceActor* Actor = *ActorPtr;
		if (Actor)
		{
			Actor->OnCaptured();
			Actor->Destroy();
		}
		PieceActors.Remove(PieceId);
	}
}

void AChessBoardActor::OnGameEnded(bool bIsDraw, EPieceColor Winner)
{
	// Log or Show UI
	// UE_LOG(LogTemp, Warning, TEXT("Game Over. Draw: %d, Winner: %d"), bIsDraw, (int32)Winner);
}

void AChessBoardActor::Server_TryMove_Implementation(FChessMove Move)
{
	if (GameModel && GameModel->TryApplyMove(Move))
	{
		// Update Replicated State for late joiners
		ReplicatedState = GameModel->BoardState->ToStruct();
		
		// Broadcast to all
		Multicast_BroadcastMove(Move);
	}
}

void AChessBoardActor::Multicast_BroadcastMove_Implementation(FChessMove Move)
{
	// On Server, GameModel is already updated by TryApplyMove in Server_TryMove
	// On Client, we need to apply it
	if (!HasAuthority())
	{
		GameModel->TryApplyMove(Move);
	}
	
	// Visuals are updated via OnMoveApplied event which GameModel broadcasts
	// And we bound that in BeginPlay. 
	// So just ensure local model is consistent.
}

void AChessBoardActor::OnRep_ReplicatedState()
{
	// Hard sync for late joiners or lag compensation
	if (GameModel && GameModel->BoardState)
	{
		GameModel->BoardState->FromStruct(ReplicatedState);
		SyncVisuals();
	}
}
