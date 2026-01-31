// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ChessLobbyWidget.h"
#include "ChessGameState.h"
#include "ChessLobbyGameMode.h"
#include "ProjectChairsPlayerState.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

void UChessLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

TSharedRef<SWidget> UChessLobbyWidget::RebuildWidget()
{
	// Build the entire UI using Slate
	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(RootBox, SVerticalBox)

			// Title
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 0, 0, 20)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Chess Lobby")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 36))
			]

			// Host Button
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 10)
			[
				SNew(SBox)
				.MinDesiredWidth(200)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.OnClicked(FOnClicked::CreateUObject(this, &UChessLobbyWidget::OnHostClicked))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Host Game")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
					]
				]
			]

			// Join Section (IP + Button)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 10)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 10, 0)
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.MinDesiredWidth(200)
					[
						SAssignNew(IPTextBox, SEditableTextBox)
						.Text(FText::FromString(TEXT("127.0.0.1")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.OnClicked(FOnClicked::CreateUObject(this, &UChessLobbyWidget::OnJoinClicked))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Join")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
					]
				]
			]

			// Spacer
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 30, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Players:")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				.Justification(ETextJustify::Center)
			]

			// White Player
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 10, 0, 5)
			[
				SAssignNew(WhitePlayerText, STextBlock)
				.Text(FText::FromString(TEXT("White: (Waiting...)")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
			]

			// Black Player
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 5)
			[
				SAssignNew(BlackPlayerText, STextBlock)
				.Text(FText::FromString(TEXT("Black: (Waiting...)")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
			]

			// Start Game Button (host only)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 20, 0, 10)
			[
				SAssignNew(StartGameButton, SBox)
				.Visibility(EVisibility::Collapsed)
				[
					SNew(SButton)
					.OnClicked(FOnClicked::CreateUObject(this, &UChessLobbyWidget::OnStartGameClicked))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Start Game")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
					]
				]
			]

			// Status Text
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 20, 0, 0)
			[
				SAssignNew(StatusText, STextBlock)
				.Text(FText::FromString(TEXT("Enter an IP to join or host a game.")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f)))
			]
		];
}

void UChessLobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdatePlayerList();
}

UChessLobbyWidget* UChessLobbyWidget::ShowLobby(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	// Create the widget
	UChessLobbyWidget* LobbyWidget = CreateWidget<UChessLobbyWidget>(PC, UChessLobbyWidget::StaticClass());
	if (LobbyWidget)
	{
		LobbyWidget->AddToViewport();

		// Set input mode to Game and UI so we can still interact
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	return LobbyWidget;
}

FReply UChessLobbyWidget::OnHostClicked()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[ChessLobby] No world!"));
		return FReply::Handled();
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[ChessLobby] No player controller!"));
		return FReply::Handled();
	}

	// Validate map exists
	FString MapPackageName = ChessMapName;
	if (!FPackageName::DoesPackageExist(MapPackageName))
	{
		UE_LOG(LogTemp, Error, TEXT("[ChessLobby] Map not found: %s"), *ChessMapName);
		if (StatusText.IsValid())
		{
			StatusText->SetText(FText::FromString(FString::Printf(TEXT("ERROR: Map not found: %s"), *ChessMapName)));
		}
		return FReply::Handled();
	}

	// Build the travel URL with listen option
	FString TravelURL = ChessMapName + TEXT("?listen");

	UE_LOG(LogTemp, Warning, TEXT("[ChessLobby] Hosting game: %s"), *TravelURL);

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(TEXT("Hosting game...")));
	}

	bIsHost = true;

	// Use console command for more reliable travel
	FString Command = FString::Printf(TEXT("open %s"), *TravelURL);
	PC->ConsoleCommand(Command);

	return FReply::Handled();
}

FReply UChessLobbyWidget::OnJoinClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[ChessLobby] No player controller!"));
		return FReply::Handled();
	}

	// Get IP from text box
	FString IPAddress = TEXT("127.0.0.1");
	if (IPTextBox.IsValid())
	{
		IPAddress = IPTextBox->GetText().ToString();
	}

	// Add port if not specified
	if (!IPAddress.Contains(TEXT(":")))
	{
		IPAddress = FString::Printf(TEXT("%s:%d"), *IPAddress, DefaultPort);
	}

	UE_LOG(LogTemp, Warning, TEXT("[ChessLobby] Joining game at: %s"), *IPAddress);

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("Connecting to %s..."), *IPAddress)));
	}

	bIsHost = false;

	// Use console command for more reliable travel
	FString Command = FString::Printf(TEXT("open %s"), *IPAddress);
	PC->ConsoleCommand(Command);

	return FReply::Handled();
}

FReply UChessLobbyWidget::OnStartGameClicked()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return FReply::Handled();
	}

	// Only the host can start the game
	AChessLobbyGameMode* GameMode = Cast<AChessLobbyGameMode>(UGameplayStatics::GetGameMode(World));
	if (GameMode && GameMode->CanStartGame())
	{
		GameMode->StartChessGame();

		if (StatusText.IsValid())
		{
			StatusText->SetText(FText::FromString(TEXT("Game started!")));
		}

		// Hide start button
		if (StartGameButton.IsValid())
		{
			StartGameButton->SetVisibility(EVisibility::Collapsed);
		}
	}

	return FReply::Handled();
}

void UChessLobbyWidget::UpdatePlayerList()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AChessGameState* ChessState = Cast<AChessGameState>(UGameplayStatics::GetGameState(World));
	if (!ChessState)
	{
		return;
	}

	// Update White player text
	if (WhitePlayerText.IsValid())
	{
		if (ChessState->WhitePlayer)
		{
			WhitePlayerText->SetText(FText::FromString(
				FString::Printf(TEXT("White: %s"), *ChessState->WhitePlayer->GetPlayerName())));
		}
		else
		{
			WhitePlayerText->SetText(FText::FromString(TEXT("White: (Waiting...)")));
		}
	}

	// Update Black player text
	if (BlackPlayerText.IsValid())
	{
		if (ChessState->BlackPlayer)
		{
			BlackPlayerText->SetText(FText::FromString(
				FString::Printf(TEXT("Black: %s"), *ChessState->BlackPlayer->GetPlayerName())));
		}
		else
		{
			BlackPlayerText->SetText(FText::FromString(TEXT("Black: (Waiting...)")));
		}
	}

	// Show/hide start button (only for host when both players ready)
	if (StartGameButton.IsValid())
	{
		bool bCanStart = ChessState->AreAllPlayersReady() && !ChessState->bGameStarted;

		// Only show on server/host
		APlayerController* PC = GetOwningPlayer();
		bool bIsServer = PC && PC->HasAuthority();

		if (bCanStart && bIsServer)
		{
			StartGameButton->SetVisibility(EVisibility::Visible);
		}
		else
		{
			StartGameButton->SetVisibility(EVisibility::Collapsed);
		}
	}

	// Update status
	if (StatusText.IsValid() && ChessState->bGameStarted)
	{
		StatusText->SetText(FText::FromString(TEXT("Game in progress!")));
	}
}
