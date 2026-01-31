// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectChairsGameMode.h"
#include "ChessLobbyGameMode.generated.h"

class AChessGameState;

/**
 * GameMode for Chess multiplayer lobby and game.
 * Handles player assignment to White/Black colors on join.
 */
UCLASS()
class PROJECTCHAIRS_API AChessLobbyGameMode : public AProjectChairsGameMode
{
	GENERATED_BODY()

public:
	AChessLobbyGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** Called when both players are ready - can be used to start the game */
	UFUNCTION(BlueprintCallable, Category = "Chess")
	void StartChessGame();

	/** Check if the game can start */
	UFUNCTION(BlueprintCallable, Category = "Chess")
	bool CanStartGame() const;

protected:
	/** Assign a color to a newly joined player */
	void AssignPlayerColor(APlayerController* PlayerController);

	/** Get the ChessGameState */
	AChessGameState* GetChessGameState() const;
};
