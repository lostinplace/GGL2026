#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Presentation/ChessPieceActor.h"
#include "Logic/ChessData.h"
#include "ChessPieceStyleSet.generated.h"

USTRUCT(BlueprintType)
struct FChessPieceStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<EPieceType, TSubclassOf<AChessPieceActor>> Pieces;
};

/**
 * definitions for piece actors
 */
UCLASS(BlueprintType)
class CHESSGAME_API UChessPieceStyleSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FChessPieceStyle WhitePieces;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FChessPieceStyle BlackPieces;

	UFUNCTION(BlueprintCallable)
	TSubclassOf<AChessPieceActor> GetPieceClass(EPieceColor Color, EPieceType Type) const
	{
		const FChessPieceStyle& Style = (Color == EPieceColor::White) ? WhitePieces : BlackPieces;
		if (const TSubclassOf<AChessPieceActor>* Found = Style.Pieces.Find(Type))
		{
			return *Found;
		}
		return nullptr;
	}
};
