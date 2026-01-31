// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Logic/ChessData.h"
#include "ChessGameState.generated.h"

class AProjectChairsPlayerState;

/**
 * GameState for Chess multiplayer that replicates player assignments (White/Black).
 */
UCLASS()
class PROJECTCHAIRS_API AChessGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AChessGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Player Assignments
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess")
	APlayerState* WhitePlayer;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess")
	APlayerState* BlackPlayer;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess")
	bool bGameStarted = false;

	/** Get the color assigned to a specific player state */
	UFUNCTION(BlueprintCallable, Category = "Chess")
	EPieceColor GetPlayerColor(APlayerState* PlayerState) const;

	/** Check if a player has been assigned a color */
	UFUNCTION(BlueprintCallable, Category = "Chess")
	bool HasPlayerColor(APlayerState* PlayerState) const;

	/** Check if both players are ready (both White and Black assigned) */
	UFUNCTION(BlueprintCallable, Category = "Chess")
	bool AreAllPlayersReady() const;

	/** Get the player state for a given color */
	UFUNCTION(BlueprintCallable, Category = "Chess")
	APlayerState* GetPlayerForColor(EPieceColor Color) const;
};
