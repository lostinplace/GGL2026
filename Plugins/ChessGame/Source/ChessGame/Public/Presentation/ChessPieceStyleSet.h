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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<EPieceType, UStaticMesh*> PieceMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UMaterialInterface* Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<EPieceType, TSubclassOf<AActor>> MaskActors;
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

	UFUNCTION(BlueprintCallable)
	const UStaticMesh* GetPieceMesh(EPieceColor Color, EPieceType Type) const
	{
		const FChessPieceStyle& Style = (Color == EPieceColor::White) ? WhitePieces : BlackPieces;
		if (const UStaticMesh* const* Found = Style.PieceMeshes.Find(Type)) // const reference to pointer
		{
			return *Found;
		}
		return nullptr;
	}

	UFUNCTION(BlueprintCallable)
	UMaterialInterface* GetSideMaterial(EPieceColor Color) const
	{
		const FChessPieceStyle& Style = (Color == EPieceColor::White) ? WhitePieces : BlackPieces;
		return Style.Material;
	}

	UFUNCTION(BlueprintCallable)
	TSubclassOf<AActor> GetMaskActorClass(EPieceType Type) const
	{
		// Mask logic might not be color specific, or it is.
		// "The masks will be their own actors".
		// Assuming generic masks for piece types?
		// Actually, user said: "separate set of properties... for the masks".
		// Let's assume Mask Actors are defined per piece type.
		// e.g. "Pawn Mask", "Rook Mask".
		// Or maybe the user meant "The mask used to Disguise AS A PAWN".
		// If I am disguised as a Pawn, I need a "Pawn Mask".
		
		// Let's check WhitePieces/BlackPieces first.
		if (const TSubclassOf<AActor>* Found = WhitePieces.MaskActors.Find(Type)) return *Found;
		
		// Fallback or symmetrical?
		return nullptr;
	}
};
