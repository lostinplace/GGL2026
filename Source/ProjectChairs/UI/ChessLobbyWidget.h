// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChessLobbyWidget.generated.h"

class SVerticalBox;
class SEditableTextBox;
class STextBlock;

/**
 * Minimal lobby widget for hosting/joining a Chess game over LAN.
 * Creates all UI programmatically using Slate - no Blueprint required.
 */
UCLASS()
class PROJECTCHAIRS_API UChessLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Create and show the lobby widget */
	UFUNCTION(BlueprintCallable, Category = "Chess Lobby", meta = (WorldContext = "WorldContextObject"))
	static UChessLobbyWidget* ShowLobby(UObject* WorldContextObject);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Slate widget references
	TSharedPtr<SVerticalBox> RootBox;
	TSharedPtr<SEditableTextBox> IPTextBox;
	TSharedPtr<STextBlock> WhitePlayerText;
	TSharedPtr<STextBlock> BlackPlayerText;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SWidget> StartGameButton;

	// Button Handlers
	FReply OnHostClicked();
	FReply OnJoinClicked();
	FReply OnStartGameClicked();

	// Update player list display
	void UpdatePlayerList();

	// The map to travel to when hosting (just the path, no asset name suffix)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Lobby")
	FString ChessMapName = TEXT("/Game/MaskChess/DevStuff/Lvl_DevBoard");

	// Default port for joining
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Lobby")
	int32 DefaultPort = 7777;

private:
	bool bIsHost = false;
};
