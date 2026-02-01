// Copyright Epic Games, Inc. All Rights Reserved.

#include "CardTargetHighlightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

UCardTargetHighlightComponent::UCardTargetHighlightComponent()
	: bUseCustomDepthOutline(true)
	, HighlightStencilValue(1)
	, bIsHighlighted(false)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCardTargetHighlightComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheMeshComponents();
}

void UCardTargetHighlightComponent::CacheMeshComponents()
{
	CachedMeshComponents.Empty();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Get all primitive components (meshes) that can render to custom depth
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* Comp : PrimitiveComponents)
	{
		// Only cache skeletal and static mesh components
		if (Comp->IsA<USkeletalMeshComponent>() || Comp->IsA<UStaticMeshComponent>())
		{
			CachedMeshComponents.Add(Comp);
		}
	}
}

void UCardTargetHighlightComponent::SetHighlighted(bool bNewHighlighted)
{
	if (bIsHighlighted != bNewHighlighted)
	{
		bIsHighlighted = bNewHighlighted;

		// Handle custom depth outline
		if (bUseCustomDepthOutline)
		{
			SetCustomDepthEnabled(bIsHighlighted);
		}

		HandleHighlightChanged(bIsHighlighted);
		OnHighlightChanged(bIsHighlighted);
	}
}

void UCardTargetHighlightComponent::SetCustomDepthEnabled(bool bEnabled)
{
	for (UPrimitiveComponent* Comp : CachedMeshComponents)
	{
		if (Comp)
		{
			Comp->SetRenderCustomDepth(bEnabled);
			if (bEnabled)
			{
				Comp->SetCustomDepthStencilValue(HighlightStencilValue);
			}
		}
	}
}

void UCardTargetHighlightComponent::HandleHighlightChanged_Implementation(bool bHighlighted)
{
	// Default implementation uses custom depth (already handled in SetHighlighted)
	// Override in Blueprint or C++ subclass to add additional visual effects
}

UCardTargetHighlightComponent* UCardTargetHighlightComponent::FindHighlightComponent(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	return Actor->FindComponentByClass<UCardTargetHighlightComponent>();
}
