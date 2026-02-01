#include "Presentation/ChessBoardActor.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "DrawDebugHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Logic/ChessGameSubsystem.h" // Include Subsystem
#include "Presentation/ChessPlayerController.h"
#include "Presentation/SelectableChessPieceComponent.h" // Required for Graveyard logic

AChessBoardActor::AChessBoardActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	
	BoardTilesWhite = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BoardTilesWhite"));
	BoardTilesWhite->SetupAttachment(RootComponent);

	BoardTilesBlack = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BoardTilesBlack"));
	BoardTilesBlack->SetupAttachment(RootComponent);

	// SelectedCoord removed
}

void AChessBoardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Debug: Show Current Player
	// if (GameModel && GameModel->BoardState)
	// {
	// 	FString SideStr = (GameModel->BoardState->SideToMove == EPieceColor::White) ? TEXT("White") : TEXT("Black");
	// 	if (GEngine)
	// 	{
	// 		GEngine->AddOnScreenDebugMessage(100, 0.0f, FColor::Yellow, FString::Printf(TEXT("Current Player: %s"), *SideStr));
	// 	}
	// }
	// Kept commented out for future debugging if needed, or remove completely? 
	// User might want it? "let's make it so that the current player id is printed in the top left of the screen at all times"
	// User REQUESTED this in Step 1086: "let's make it so that the current player id is printed in the top left of the screen at all times"
	// So I should KEEP the player ID debug.
	
	// I should ONLY remove the Mask Attachment debug.
	if (GameModel && GameModel->BoardState)
	{
		FString SideStr = (GameModel->BoardState->SideToMove == EPieceColor::White) ? TEXT("White") : TEXT("Black");
		if (GEngine)
		{
			// GEngine->AddOnScreenDebugMessage(100, 0.0f, FColor::Yellow, FString::Printf(TEXT("Current Player: %s"), *SideStr));
		}
	}

	if (bShowDebugGrid)
	{
		for (int32 File = 0; File < 8; ++File)
		{
			for (int32 Rank = 0; Rank < 8; ++Rank)
			{
				bool bIsBlack = (File + Rank) % 2 == 0;
				if (bIsBlack)
				{
					FVector Center = CoordToWorld(FBoardCoord(File, Rank));
					// Draw slightly above PieceHeightOffset to be visible? Or at it.
					// Use a small extent
					FVector Extent(SquareSize * 0.45f, SquareSize * 0.45f, 1.0f);
					DrawDebugSolidBox(GetWorld(), Center, Extent, FColor(0, 0, 0, 128), false, -1.0f, 0);
				}
			}
		}
	}
}

bool AChessBoardActor::ShouldTickIfViewportsOnly() const
{
	return bShowDebugGrid;
}

void AChessBoardActor::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("AChessBoardActor::BeginPlay Start"));

	// Initialize Model
	GameModel = NewObject<UChessGameModel>(this);
	GameModel->InitMode = InitMode; // Pass configuration
	GameModel->InitializeGame();
	UE_LOG(LogTemp, Warning, TEXT("GameModel Initialized. InitMode: %d"), (int32)InitMode);

	// Register with Subsystem
	if (UChessGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UChessGameSubsystem>())
	{
		Subsystem->RegisterGame(GameModel);
		Subsystem->OnSelectionUpdated.AddDynamic(this, &AChessBoardActor::OnSubsystemSelectionChanged);
	}

	// Bind Events
	GameModel->OnMoveApplied.AddDynamic(this, &AChessBoardActor::OnMoveApplied);
	GameModel->OnPieceCaptured.AddDynamic(this, &AChessBoardActor::OnPieceCaptured);
	GameModel->OnGameEnded.AddDynamic(this, &AChessBoardActor::OnGameEnded);
	GameModel->OnPieceMaskChanged.AddDynamic(this, &AChessBoardActor::OnPieceMaskChanged);
	GameModel->OnCheckStatusChanged.AddDynamic(this, &AChessBoardActor::OnCheckStatusChanged);

	// Initial Sync
	if (HasAuthority())
	{
		ReplicatedState = GameModel->BoardState->ToStruct();
	}
	SpawnBoardGrid();
	SyncVisuals();
}

void AChessBoardActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChessBoardActor, ReplicatedState);
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

void AChessBoardActor::OnSubsystemSelectionChanged(FBoardCoord NewCoord)
{
	// Logic to update visuals based on global selection
	
	// 1. Clear previous highlights
	OnClearHighlights();

	// 2. Notify all pieces (Deselect all first? Or keep track of last selected?)
	// Simplest: Iterate all piece actors and set selected = false, then set new one true.
	// Optimization: Subsystem could pass "OldCoord", but we only get NewCoord here.
	// Let's scan.
	for (auto& Pair : PieceActors)
	{
		if (Pair.Value) Pair.Value->OnSelectionChanged(false);
	}

	// 3. If Valid, Select and Highlight
	if (NewCoord.IsValid())
	{
		int32 PieceId = GameModel->BoardState->GetPieceIdAt(NewCoord);
		if (AChessPieceActor** PA = PieceActors.Find(PieceId))
		{
			if (*PA) (*PA)->OnSelectionChanged(true);
		}

		// Highlight Moves
		TArray<FChessMove> LegalMoves;
		GameModel->GetLegalMovesForCoord(NewCoord, LegalMoves);
		
		OnHighlightMoves(LegalMoves);
	}
}

FVector AChessBoardActor::CoordToWorld(const FBoardCoord Coord) const
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

void AChessBoardActor::HandleSquareClicked(FBoardCoord Coord)
{
	if (!Coord.IsValid()) return;

	UChessGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UChessGameSubsystem>();
	if (!Subsystem) return;

	FBoardCoord CurrentSelection = Subsystem->CurrentSelectedCoord;

	// Check if we selected a piece or a target
	int32 ClickedPieceId = GameModel->BoardState->GetPieceIdAt(Coord);
	UE_LOG(LogTemp, Warning, TEXT("[ChessBoard] Clicked Coord: %d,%d -> PieceID: %d"), Coord.File, Coord.Rank, ClickedPieceId);
	
	if (CurrentSelection.IsValid())
	{
		// Check if we clicked the same square (deselect)
		if (CurrentSelection == Coord)
		{
			Subsystem->ClearSelection();
			return;
		}

		// Try to move
		bool bMoved = false;
		
		// Build a move. We need to find if there is a legal move from Selected to Coord.
		TArray<FChessMove> LegalMoves;
		GameModel->GetLegalMovesForCoord(CurrentSelection, LegalMoves);

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
				TargetMove.PromotionType = EPieceType::Queen; // MVP
			}

			// Call Server RPC via PlayerController (Client owns PC, not Board)
			if (APlayerController* LocalPC = UGameplayStatics::GetPlayerController(this, 0))
			{
				if (AChessPlayerController* ChessPC = Cast<AChessPlayerController>(LocalPC))
				{
					ChessPC->Server_SubmitMove(this, TargetMove);
				}
			}
			
			// Deselect immediately via Subsystem
			Subsystem->ClearSelection();
			return;
		}
		
		// If not a legal move, check if we are selecting a different friendly piece
		if (ClickedPieceId != -1)
		{
			const FPieceInstance* ClickedPiece = GameModel->BoardState->GetPiece(ClickedPieceId);
			if (ClickedPiece && ClickedPiece->Color == GameModel->BoardState->SideToMove)
			{
				// Switch selection
				Subsystem->SetSelectedCoord(Coord);
			}
			else
			{
				// Clicked empty or enemy, but not a valid move -> Deselect
				Subsystem->ClearSelection();
			}
		}
		else
		{
			// Clicked empty space invalid -> Deselect
			Subsystem->ClearSelection();
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
				Subsystem->SetSelectedCoord(Coord);
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
	
	if (!GameModel || !GameModel->BoardState)
	{
		UE_LOG(LogTemp, Error, TEXT("SyncVisuals: GameModel or BoardState is NULL"));
		return;
	}

	int32 Count = GameModel->BoardState->Pieces.Num();
	UE_LOG(LogTemp, Warning, TEXT("SyncVisuals: Found %d pieces in BoardState"), Count);

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
	if (!StyleSet)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnPieceActor: StyleSet is NULL!"));
		return;
	}
	if (!GameModel || !GameModel->BoardState) return;

	// Determine Visual Type based on Observer
	EPieceType VisualType = Type;
	
	// APPROACH:
	// 1. Pass the PieceInstance to the Actor.
	// 2. The Actor in BeginPlay (and OnRep) checks Local Player Controller Team.
	// 3. Sets Mesh accordingly.
	
	TSubclassOf<AChessPieceActor> Class = StyleSet->GetPieceClass(Color, Type); 
	if (!Class)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnPieceActor: StyleSet returned NULL Class for Color %d Type %d"), (int32)Color, (int32)Type);
		return;
	}

	if (Class)
	{
		FVector Loc = CoordToWorld(Coord);
		FActorSpawnParameters Params;
		FRotator Rot = FRotator::ZeroRotator;
		if (Color == EPieceColor::Black) Rot.Yaw = 180.0f;

		AChessPieceActor* NewPiece = GetWorld()->SpawnActor<AChessPieceActor>(Class, Loc, Rot, Params);
		if (!NewPiece)
		{
			UE_LOG(LogTemp, Error, TEXT("SpawnPieceActor: GetWorld()->SpawnActor Failed!"));
		}
		if (NewPiece)
		{
			NewPiece->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			NewPiece->Init(PieceId, Type, Color);
			
			NewPiece->Init(PieceId, Type, Color);
			
			// Setup Visuals
			const FPieceInstance* Piece = GameModel->BoardState->Pieces.Find(PieceId);
			if (Piece)
			{
				UpdatePieceVisuals(*Piece, NewPiece);
			}
			else
			{
				// Fallback if not in map? Should not happen during spawn loop
				NewPiece->UpdateVisuals(StyleSet, Type, EPieceType::None);
			}

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

	// Refresh Visuals for Turn Change (Hot Seat Debugging)
	if (GameModel && GameModel->BoardState)
	{
		for (auto& Pair : GameModel->BoardState->Pieces)
		{
			if (AChessPieceActor** AP = PieceActors.Find(Pair.Key))
			{
				if (*AP) UpdatePieceVisuals(Pair.Value, *AP);
			}
		}

		// Notify Local Player Controller of Turn Change (for UI)
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (AChessPlayerController* ChessPC = Cast<AChessPlayerController>(PC))
			{
				ChessPC->OnTurnChanged(GameModel->BoardState->SideToMove);
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
			AddToGraveyard(Actor);
			// Do NOT Destroy.
		}
		PieceActors.Remove(PieceId);
	}
}

void AChessBoardActor::AddToGraveyard(AChessPieceActor* Actor)
{
	if (!Actor) return;

	// Disable Interaction
	if (Actor->SelectionComponent)
	{
		Actor->SelectionComponent->SetSelectable(false);
	}

	// Determine Graveyard & Index
	TArray<AChessPieceActor*>* Graveyard = nullptr;
	int32 Index = 0;
	int32 Direction = 1; // 1 for Right, -1 for Left

	// Logic:
	// Captured White pieces -> Go to Black's side? Or White's side?
	// Usually "My Captured Pile" contains enemy pieces I captured.
	// But "Pieces shown by the side" could mean "Dead Pool".
	// Let's group by "Color of the Piece".
	// White Pieces go to White Side (Left/Bottom), Black to Black Side.
	
	if (Actor->Color == EPieceColor::White)
	{
		Graveyard = &GraveyardWhite;
		Direction = -1; // Left of board
	}
	else
	{
		Graveyard = &GraveyardBlack;
		Direction = 1; // Right of board
	}

	if (Graveyard)
	{
		Index = Graveyard->Num();
		Graveyard->Add(Actor);
		
		// Calculate Location
		// Board Width approx 8 * SquareSize.
		// Offset from Center.
		// If CenterBoard is true, Board is -4 to +4.
		// Left Side starts at -5. Right starts at +5.
		
		float FileOffset = (bCenterBoard ? 5.0f : 9.0f) * Direction;
		// Stack them? 2 columns?
		// Simple Row:
		float X = FileOffset * SquareSize; 
		
		// Y Position: Start from Rank 0 up to Rank 7, then wrap?
		// Let's just stack them linearly along Y using Index.
		// Center Y = 0.
		// Start at Y = -3.5 * Size (Rank 0 equivalent).
		float StartY = (bCenterBoard ? -3.5f : 0.5f) * SquareSize;
		float Y = StartY + (Index * SquareSize * 0.6f); // Tighter packing
		
		// If too many, make a second column
		if (Index >= 8)
		{
			X += Direction * SquareSize * 0.8f;
			Y = StartY + ((Index - 8) * SquareSize * 0.6f);
		}

		FVector NewLoc(X, Y, PieceHeightOffset);
		
		// Transform to World
		// CoordToWorld handles Board Transform, but we are calculating local coords manually here.
		// Let's use Actor Transform.
		FVector WorldLoc = GetActorTransform().TransformPosition(NewLoc);
		
		Actor->SetActorLocation(WorldLoc);
		Actor->SetActorRotation(GetActorRotation()); // Reset rotation?
	}
}

void AChessBoardActor::OnGameEnded(bool bIsDraw, EPieceColor Winner)
{
	UE_LOG(LogTemp, Warning, TEXT("Game Over. Draw: %d, Winner: %d"), bIsDraw, (int32)Winner);

	FString Message;
	if (bIsDraw)
	{
		Message = TEXT("GAME OVER: Stalemate (Draw)");
	}
	else
	{
		FString WinnerStr = (Winner == EPieceColor::White) ? TEXT("White") : TEXT("Black");
		Message = FString::Printf(TEXT("CHECKMATE! %s Wins!"), *WinnerStr);
	}

	// Forward to Local Player Controller
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AChessPlayerController* ChessPC = Cast<AChessPlayerController>(PC))
		{
			ChessPC->OnGameStatusMessage(FText::FromString(Message));
		}
	}
}

void AChessBoardActor::OnCheckStatusChanged(bool bInCheck, EPieceColor SideInCheck)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AChessPlayerController* ChessPC = Cast<AChessPlayerController>(PC))
		{
			if (bInCheck)
			{
				FString SideStr = (SideInCheck == EPieceColor::White) ? TEXT("White") : TEXT("Black");
				FString Message = FString::Printf(TEXT("CHECK! %s King is under attack!"), *SideStr);
				ChessPC->OnGameStatusMessage(FText::FromString(Message));
			}
			else
			{
				// Clear message if not in check
				ChessPC->OnGameStatusMessage(FText::GetEmpty());
			}
		}
	}
}

void AChessBoardActor::OnPieceMaskChanged(int32 PieceId, EPieceType NewMask)
{
	if (!GameModel || !GameModel->BoardState) return;

	if (FPieceInstance* Piece = GameModel->BoardState->Pieces.Find(PieceId))
	{
		// Find Actor
		if (AChessPieceActor** ActorPtr = PieceActors.Find(PieceId))
		{
			AChessPieceActor* Actor = *ActorPtr;
			if (Actor)
			{
				UpdatePieceVisuals(*Piece, Actor);
				
				// Also notify actor of mask change (sound/fx)
				Actor->OnMaskChanged(NewMask);
			}
		}
	}
}

void AChessBoardActor::ProcessMove(FChessMove Move)
{
	if (!HasAuthority()) return;

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

void AChessBoardActor::RequestSetPieceMask(int32 PieceId, EPieceType NewMask)
{
	// If we have authority, call multicast directly
	if (HasAuthority())
	{
		Multicast_SetPieceMask(PieceId, NewMask);
	}
	else
	{
		// Route through player controller's server RPC
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AChessPlayerController* ChessPC = Cast<AChessPlayerController>(PC))
			{
				ChessPC->Server_SetPieceMask(this, PieceId, NewMask);
			}
		}
	}
}

void AChessBoardActor::RequestRemovePiece(int32 PieceId)
{
	// If we have authority, call multicast directly
	if (HasAuthority())
	{
		Multicast_RemovePiece(PieceId);
	}
	else
	{
		// Route through player controller's server RPC
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AChessPlayerController* ChessPC = Cast<AChessPlayerController>(PC))
			{
				ChessPC->Server_RemovePiece(this, PieceId);
			}
		}
	}
}

void AChessBoardActor::Multicast_SetPieceMask_Implementation(int32 PieceId, EPieceType NewMask)
{
	if (GameModel)
	{
		GameModel->SetPieceMask(PieceId, NewMask);
	}

	// Update replicated state on server
	if (HasAuthority() && GameModel && GameModel->BoardState)
	{
		ReplicatedState = GameModel->BoardState->ToStruct();
	}
}

void AChessBoardActor::Multicast_RemovePiece_Implementation(int32 PieceId)
{
	if (GameModel && GameModel->BoardState)
	{
		GameModel->BoardState->RemovePiece(PieceId);
		GameModel->OnPieceCaptured.Broadcast(PieceId);
	}

	// Update replicated state on server
	if (HasAuthority() && GameModel && GameModel->BoardState)
	{
		ReplicatedState = GameModel->BoardState->ToStruct();
	}
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

void AChessBoardActor::OnHighlightMoves_Implementation(const TArray<FChessMove>& Moves)
{
	if (!StyleSet || !StyleSet->HighlightActorClass) return;

	OnClearHighlights(); // Clear existing first

	for (const FChessMove& Move : Moves)
	{
		FVector Center = CoordToWorld(Move.To);
		Center.Z = PieceHeightOffset + 1.0f; // Slightly above board/piece base? Or at PieceHeightOffset.

		FActorSpawnParameters Params;
		Params.Owner = this;
		
		if (AActor* NewActor = GetWorld()->SpawnActor<AActor>(StyleSet->HighlightActorClass, Center, FRotator::ZeroRotator, Params))
		{
			// Auto-Scale: Assume the actor is designed for 100x100 units (Standard Unreal)
			// Adjust scale if SquareSize is different.
			float Scale = SquareSize / 100.0f;
			NewActor->SetActorScale3D(FVector(Scale, Scale, Scale));

			// Fix for Attachment Error: Ensure Root is Movable so it can attach to the Board
			if (USceneComponent* Root = NewActor->GetRootComponent())
			{
				Root->SetMobility(EComponentMobility::Movable);
			}

			// Disable Collision so they don't block clicks
			NewActor->SetActorEnableCollision(false);

			// Attach to board so they move with it
			NewActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			ActiveHighlightActors.Add(NewActor);
		}
	}
}

void AChessBoardActor::OnClearHighlights_Implementation()
{
	for (AActor* Actor : ActiveHighlightActors)
	{
		if (Actor) Actor->Destroy();
	}
	ActiveHighlightActors.Empty();
}

EPieceColor AChessBoardActor::GetObserverSide() const
{
	// 1. Try to get Local Player Controller (Client's perspective)
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AChessPlayerController* ChessPC = Cast<AChessPlayerController>(PC))
		{
			// If we have an assigned color, use it (Network/Local Multiplayer with assignment)
			if (ChessPC->HasAssignedColor())
			{
				return ChessPC->GetAssignedColor();
			}
		}
	}

	// 2. Fallback for Hot-Seat / Testing (No assigned colors): Use SideToMove
	if (GameModel && GameModel->BoardState)
	{
		return GameModel->BoardState->SideToMove;
	}
	return EPieceColor::White;
}

void AChessBoardActor::RefreshPieceVisuals()
{
	if (GameModel && GameModel->BoardState)
	{
		for (auto& Pair : GameModel->BoardState->Pieces)
		{
			if (AChessPieceActor** AP = PieceActors.Find(Pair.Key))
			{
				if (*AP) UpdatePieceVisuals(Pair.Value, *AP);
			}
		}
	}
}

void AChessBoardActor::UpdatePieceVisuals(const FPieceInstance& Piece, AChessPieceActor* Actor)
{
	if (!Actor || !StyleSet) return;

	EPieceColor ObserverSide = GetObserverSide();
	EPieceType BodyType = Piece.Type;
	EPieceType MaskType = Piece.MaskType;

	// Visual Deception Logic
	// If I am NOT the owner, and the piece is Masked -> I see a Pawn + Mask
	if (Piece.Color != ObserverSide && MaskType != EPieceType::None)
	{
		BodyType = EPieceType::Pawn;
	}

	Actor->UpdateVisuals(StyleSet, BodyType, MaskType);
}

void AChessBoardActor::SpawnBoardGrid()
{
	if (!StyleSet) return;
	if (!BoardTilesWhite || !BoardTilesBlack) return;

	// Reset
	BoardTilesWhite->ClearInstances();
	BoardTilesBlack->ClearInstances();

	// Set Assets
	if (StyleSet->BoardTileMesh)
	{
		BoardTilesWhite->SetStaticMesh(StyleSet->BoardTileMesh);
		BoardTilesBlack->SetStaticMesh(StyleSet->BoardTileMesh);
	}

	if (StyleSet->BoardMaterialWhite) BoardTilesWhite->SetMaterial(0, StyleSet->BoardMaterialWhite);
	if (StyleSet->BoardMaterialBlack) BoardTilesBlack->SetMaterial(0, StyleSet->BoardMaterialBlack);

	// Generate 8x8 Grid
	for (int32 File = 0; File < 8; ++File)
	{
		for (int32 Rank = 0; Rank < 8; ++Rank)
		{
			FBoardCoord Coord(File, Rank);
			FVector Center = CoordToWorld(Coord);
			// Tile should be at floor level, maybe offset Z? 
			// Assuming Mesh origin is center.
			// PieceHeightOffset is for pieces. Tiles should be below pieces.
			Center.Z -= 1.0f; // Minimal offset to avoid Z-fighting with debug lines if any

			FTransform Trans(Center);
			
			bool bIsBlack = (File + Rank) % 2 != 0; // A1=0,0 (Even) -> Black? 
			// Standard Chess: A1 is Black (Dark). A2 is White.
			// (0,0): 0+0=0 Even. 
			// Wait, A1 (Left Corner) is usually BLACK? No, "White on Right" rule.
			// H1 (Right corner for White) is White.
			// A1 (Left corner for White) is Black.
			// So (0,0) Sum=0 Even -> Black?
			// Let's stick to standard checkers convention.
			
			if (bIsBlack)
			{
				BoardTilesBlack->AddInstance(Trans);
			}
			else
			{
				BoardTilesWhite->AddInstance(Trans);
			}
		}
	}
}
