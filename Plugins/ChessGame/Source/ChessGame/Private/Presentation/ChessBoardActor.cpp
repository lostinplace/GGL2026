#include "Presentation/ChessBoardActor.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Logic/ChessGameSubsystem.h" // Include Subsystem

AChessBoardActor::AChessBoardActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	// SelectedCoord removed
}

void AChessBoardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	// Initialize Model
	GameModel = NewObject<UChessGameModel>(this);
	GameModel->InitMode = InitMode; // Pass configuration
	GameModel->InitializeGame();

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

		UE_LOG(LogTemp, Warning, TEXT("[ChessBoard] Selection Changed to %d,%d. Found %d Legal Moves."), NewCoord.File, NewCoord.Rank, LegalMoves.Num());
		
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

			// Call Server RPC
			Server_TryMove(TargetMove);
			
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
	if (!GameModel || !GameModel->BoardState) return;

	// Determine Visual Type based on Observer
	EPieceType VisualType = Type;
	
	// Get Observer (For Local Player). 
	// In a real network game, we need to know "Who Am I".
	// For MVP/Local testing, we can assume we are "White" or "Black" based on...
	// Actually, this logic runs on ALL clients.
	// We need GetLocalPlayer -> Subsystem -> Side?
	// Or simply, we spawn based on "Who is viewing".
	// BUT, if we spawn an Actor, everyone sees the same Actor Mesh unless we use 'OnlyOwnerSee' which is complex for board games.
	// Better approach: The Piece Actor itself handles the visual deception based on Local Player Controller?
	// OR: The Server replicates the "Visual Type" separately?
	
	// User Requirement: "to the owning player, the pice will look like it's real type... to the opposing player, a masked piece should always look like a rook"
	// This means we need DYNAMIC visuals per client.
	// Actors are replicated. 
	
	// APPROACH:
	// 1. Pass the PieceInstance to the Actor.
	// 2. The Actor in BeginPlay (and OnRep) checks Local Player Controller Team.
	// 3. Sets Mesh accordingly.
	
	// For NOW, in SpawnPieceActor, we just pass the PieceID/Info. We interact with the Actor below.
	
	TSubclassOf<AChessPieceActor> Class = StyleSet->GetPieceClass(Color, Type); 
	// PROBLEM: If we spawn the "Real" class, and the visual is "Pawn", but real is "Rook", 
	// and if classes allow different logic/visuals, we might need a generic class.
	// Assuming StyleSet returns same class (BP_ChessPiece) for all, just different meshes?
	// If different classes, we have a problem swapping them locally.
	
	// Let's assume generic BP_ChessPiece_C is used, or they share logic.
	// If the user uses unique BPs for Rook vs Pawn, we cannot easily swap "Visuals" without destroying actor.
	
	// MVP Fix: Spawn the "REAL" actor, but tell it to "Disguise" itself.
	
	if (Class)
	{
		FVector Loc = CoordToWorld(Coord);
		FActorSpawnParameters Params;
		FRotator Rot = FRotator::ZeroRotator;
		if (Color == EPieceColor::Black) Rot.Yaw = 180.0f;

		AChessPieceActor* NewPiece = GetWorld()->SpawnActor<AChessPieceActor>(Class, Loc, Rot, Params);
		if (NewPiece)
		{
			NewPiece->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			NewPiece->Init(PieceId, Type, Color);
			
			// Setup Masking
			const FPieceInstance* Piece = GameModel->BoardState->Pieces.Find(PieceId);
			if (Piece && Piece->MaskType != EPieceType::None)
			{
				// Determine Observer Side
				EPieceColor ObserverSide = EPieceColor::White; // Default
				
				// In a real network setting, check UChessGameSubsystem or PC->PlayerState->Team
				// For now, MVP assumes Standalone/Listening Server is White context or "God"
				// But to TEST masking, we need to pretend we are the OPPONENT?
				// If I am White, and I spawn a Black Piece with Mask, I (White) am the Observer.
				// So ObserverSide = My Side.
				
				APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
				// MVP: Assume Player 0 is White.
				
				EPieceType VisType = Piece->GetVisualType(ObserverSide);
				if (VisType != Type)
				{
					NewPiece->UpdateVisuals(StyleSet, VisType);
				}
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
				// Determine Observer Side (MVP: White)
				EPieceColor ObserverSide = EPieceColor::White;
				APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0); 
				
				// Calculate Visual Type
				EPieceType VisType = Piece->GetVisualType(ObserverSide);
				
				// Update Actor
				Actor->UpdateVisuals(StyleSet, VisType);
				
				// Also notify actor of mask change (sound/fx)
				Actor->OnMaskChanged(NewMask);
			}
		}
	}
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

void AChessBoardActor::OnHighlightMoves_Implementation(const TArray<FChessMove>& Moves)
{
	UE_LOG(LogTemp, Warning, TEXT("[ChessBoard] Drawing Highlights for %d Moves"), Moves.Num());

	// Default Debug Implementation
	for (const FChessMove& Move : Moves)
	{
		FVector Center = CoordToWorld(Move.To);
		// Draw a solid box (semi-transparent green), Persistent = true
		DrawDebugSolidBox(GetWorld(), Center, FVector(SquareSize*0.4f, SquareSize*0.4f, 2.0f), FColor(0, 255, 0, 100), true);
	}
}

void AChessBoardActor::OnClearHighlights_Implementation()
{
	// DrawDebugSolidBox doesn't have a specific "Clear" unless we use persistent lines and flush.
	// But since we use Lifetime=-1 (one frame? no, -1 is default? Wait.)
	// Default is -1.0f which means "Lifetime of line" ?? No.
	// If bPersistent is false and Lifetime is -1, it lasts for one frame? 
	// Actually DrawDebugSolidBox usually persists if bPersistent is true.
	// If false, it's one frame.
	
	// Wait, if we draw on "Click", it won't persist if it's one frame.
	// We need it to persist until cleared.
	// So we should use bPersistent = true? And FlushDebugStrings? No, FlushPersistentDebugLines.
	
	FlushPersistentDebugLines(GetWorld());
}
