#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Presentation/ChessPieceActor.h"
#include "Logic/ChessData.h"
#include "ChessPieceStyleSet.generated.h"

UCLASS(BlueprintType)
class CHESSGAME_API UChessPieceStyleSet : public UDataAsset
{
	GENERATED_BODY()

public:
	// Master Class for all pieces
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TSubclassOf<AChessPieceActor> MasterPieceClass;

	// Meshes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TMap<EPieceType, USkeletalMesh*> PieceMeshes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TMap<EPieceType, UStaticMesh*> MaskMeshes;

	// Materials
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	UMaterialInterface* WhiteMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	UMaterialInterface* BlackMaterial;

	UFUNCTION(BlueprintCallable)
	TSubclassOf<AChessPieceActor> GetPieceClass(EPieceColor Color, EPieceType Type) const
	{
		return MasterPieceClass;
	}

	UFUNCTION(BlueprintCallable)
	const USkeletalMesh* GetPieceMesh(EPieceColor Color, EPieceType Type) const
	{
		if (const USkeletalMesh* const* Found = PieceMeshes.Find(Type))
		{
			return *Found;
		}
		return nullptr;
	}

	UFUNCTION(BlueprintCallable)
	const UStaticMesh* GetMaskMesh(EPieceColor Color, EPieceType Type) const
	{
		if (const UStaticMesh* const* Found = MaskMeshes.Find(Type))
		{
			return *Found;
		}
		return nullptr;
	}

	UFUNCTION(BlueprintCallable)
	UMaterialInterface* GetSideMaterial(EPieceColor Color) const
	{
		return (Color == EPieceColor::White) ? WhiteMaterial : BlackMaterial;
	}
};
