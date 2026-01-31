#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Logic/ChessGameModel.h"
#include "Presentation/ChessPieceActor.h"
#include "Presentation/ChessPieceStyleSet.h"
#include "ChessBoardActor.generated.h"

UCLASS()
class CHESSGAME_API AChessBoardActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AChessBoardActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

public:	
	// Config
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chess")
	UChessPieceStyleSet* StyleSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chess")
	float SquareSize = 100.0f;

	// If true, the board center (intersection of rank 4/5 and file D/E) is at (0,0,0)
	// If false, A1 (0,0) is at (0,0,0) with offset half-size
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chess")
	bool bCenterBoard = true;

	// Height offset for spawning pieces (e.g. if board has thickness)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chess")
	float PieceHeightOffset = 0.0f;

	// Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugGrid = false;

	// State
	UPROPERTY(BlueprintReadOnly, Category = "Chess")
	UChessGameModel* GameModel;

	UPROPERTY()
	TMap<int32, AChessPieceActor*> PieceActors;

	// Selection
	UPROPERTY(BlueprintReadOnly)
	FBoardCoord SelectedCoord;

	// Helpers
	UFUNCTION(BlueprintCallable)
	FVector CoordToWorld(FBoardCoord Coord) const;

	UFUNCTION(BlueprintCallable)
	FBoardCoord WorldToCoord(FVector WorldLoc) const;

	UFUNCTION(BlueprintCallable)
	void HandleSquareClicked(FBoardCoord Coord);

	// Events from Model
	UFUNCTION()
	void OnMoveApplied(const FChessMove& Move);

	UFUNCTION()
	void OnPieceCaptured(int32 PieceId);

	UFUNCTION()
	void OnGameEnded(bool bIsDraw, EPieceColor Winner);

	// Visuals
	void SyncVisuals();
	void SpawnPieceActor(int32 PieceId, EPieceType Type, EPieceColor Color, FBoardCoord Coord);

	// Network
	UFUNCTION(Server, Reliable)
	void Server_TryMove(FChessMove Move);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastMove(FChessMove Move);

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedState)
	FChessBoardStateData ReplicatedState;

	UFUNCTION()
	void OnRep_ReplicatedState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Blueprint Events
	UFUNCTION(BlueprintNativeEvent)
	void OnHighlightMoves(const TArray<FChessMove>& Moves);

	UFUNCTION(BlueprintNativeEvent)
	void OnClearHighlights();

};
