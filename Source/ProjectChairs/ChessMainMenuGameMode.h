// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ChessMainMenuGameMode.generated.h"

class UChessLobbyWidget;

/**
 * Simple GameMode for the main menu that automatically shows the chess lobby widget.
 * Use this as the GameMode for your MainMenu level.
 */
UCLASS()
class PROJECTCHAIRS_API AChessMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AChessMainMenuGameMode();

	virtual void BeginPlay() override;

protected:
	UPROPERTY()
	UChessLobbyWidget* LobbyWidget;
};
