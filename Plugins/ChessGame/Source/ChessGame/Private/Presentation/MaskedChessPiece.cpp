#include "Presentation/MaskedChessPiece.h" // Wait, need to ensure correct include path
#include "Presentation/ChessPieceStyleSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

AMaskedChessPiece::AMaskedChessPiece()
{
	// Setup Components
	
	// Assuming RootComponent from Base Class
	
	UnmaskedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnmaskedMesh"));
	UnmaskedMesh->SetupAttachment(RootComponent);
	UnmaskedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Visual only

	MaskedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MaskedMesh"));
	MaskedMesh->SetupAttachment(RootComponent);
	MaskedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MaskedMesh->SetVisibility(false); // Hidden by default

	UnmaskedMaskAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("UnmaskedMaskAttachPoint"));
	UnmaskedMaskAttachPoint->SetupAttachment(RootComponent);

	MaskedMaskAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("MaskedMaskAttachPoint"));
	MaskedMaskAttachPoint->SetupAttachment(RootComponent);
}

void AMaskedChessPiece::Init(int32 InPieceId, EPieceType InType, EPieceColor InColor)
{
	Super::Init(InPieceId, InType, InColor);

	// Material application will be handled in UpdateVisuals or helper, 
	// but user said "receives a material at initialization".
	// The base Init doesn't take Material.
	// We can query the StyleSet here if we had access, or wait for UpdateVisuals.
}

void AMaskedChessPiece::UpdateVisuals_Implementation(const UChessPieceStyleSet* StyleSet, EPieceType VisualType)
{
	if (!StyleSet) return;

	// 1. Determine Identity
	bool bIsMasked = (VisualType != Type);
	
	// 2. Set Meshes
	// Unmasked Mesh = My Real Type Mesh
	if (const UStaticMesh* RealMeshAsset = StyleSet->GetPieceMesh(Color, Type))
	{
		UnmaskedMesh->SetStaticMesh(const_cast<UStaticMesh*>(RealMeshAsset));
	}

	// Masked Mesh = The Visual Type Mesh (e.g. Pawn)
	if (const UStaticMesh* DisguiseMeshAsset = StyleSet->GetPieceMesh(Color, VisualType))
	{
		MaskedMesh->SetStaticMesh(const_cast<UStaticMesh*>(DisguiseMeshAsset));
	}

	// 3. Apply Materials (User Requirement: Assign material 0)
	if (UMaterialInterface* SideMat = StyleSet->GetSideMaterial(Color))
	{
		UnmaskedMesh->SetMaterial(0, SideMat);
		MaskedMesh->SetMaterial(0, SideMat);
	}

	// 4. Toggle Visibility
	if (bIsMasked)
	{
		UnmaskedMesh->SetVisibility(false);
		MaskedMesh->SetVisibility(true);
	}
	else
	{
		UnmaskedMesh->SetVisibility(true);
		MaskedMesh->SetVisibility(false);
	}

	// 5. Handle Mask Actor (The Prop)
	// We need the Mask Actor Class. Where is it?
	// User said "separate set of properties in our data asset for the masks".
	// Let's assume StyleSet has GetMaskActorClass(VisualType) or something.
	
	TSubclassOf<AActor> MaskClass = StyleSet->GetMaskActorClass(VisualType); // We need to add this
	
	if (MaskClass)
	{
		// Spawn if needed
		if (!MaskActorInstance || MaskActorInstance->GetClass() != MaskClass)
		{
			if (MaskActorInstance) MaskActorInstance->Destroy();
			
			FActorSpawnParameters Params;
			Params.Owner = this;
			MaskActorInstance = GetWorld()->SpawnActor<AActor>(MaskClass, GetActorTransform(), Params);
		}

		// Attach to correct point
		if (MaskActorInstance)
		{
			if (bIsMasked)
			{
				MaskActorInstance->AttachToComponent(MaskedMaskAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}
			else
			{
				MaskActorInstance->AttachToComponent(UnmaskedMaskAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}
		}
	}
	else
	{
		// No mask defined for this type (e.g. maybe only Pawns have masks?)
		if (MaskActorInstance)
		{
			MaskActorInstance->Destroy();
			MaskActorInstance = nullptr;
		}
	}
}

void AMaskedChessPiece::BeginDestroy()
{
	Super::BeginDestroy();
}

void AMaskedChessPiece::Destroyed()
{
	if (MaskActorInstance)
	{
		MaskActorInstance->Destroy();
	}
	Super::Destroyed();
}
