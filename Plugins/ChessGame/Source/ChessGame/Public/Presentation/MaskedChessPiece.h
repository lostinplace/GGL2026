#pragma once

#include "CoreMinimal.h"
#include "Presentation/ChessPieceActor.h"
#include "MaskedChessPiece.generated.h"

/**
 * Specialized Chess Piece that handles Masking via dual meshes and attachment points.
 */
UCLASS()
class CHESSGAME_API AMaskedChessPiece : public AChessPieceActor
{
	GENERATED_BODY()
	
public:
	AMaskedChessPiece();

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
	UStaticMeshComponent* UnmaskedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
	UStaticMeshComponent* MaskedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
	USceneComponent* UnmaskedMaskAttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visuals")
	USceneComponent* MaskedMaskAttachPoint;

	// The actual physical mask actor (the prop)
	UPROPERTY(BlueprintReadOnly, Category = "Visuals")
	AActor* MaskActorInstance;

protected:
	// Override to handle dual mesh and mask positioning
	virtual void UpdateVisuals_Implementation(const UChessPieceStyleSet* StyleSet, EPieceType VisualType) override;

	// Override to optionally set material
	virtual void Init(int32 InPieceId, EPieceType InType, EPieceColor InColor) override;

	// Hook to destroy mask actor
	virtual void BeginDestroy() override;
	virtual void Destroyed() override;
};
