#include "Presentation/ChessCameraPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AChessCameraPawn::AChessCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootScene;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 800.0f;
	SpringArm->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f));
	SpringArm->bDoCollisionTest = false; // Don't clip into board

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
}

void AChessCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// Initialize camera orientation
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
}

void AChessCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AChessCameraPawn::Move);
		}

		if (ZoomAction)
		{
			EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AChessCameraPawn::OnCameraZoom);
		}

		if (CameraDragAction)
		{
			EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Started, this, &AChessCameraPawn::OnCameraDragStart);
			EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Completed, this, &AChessCameraPawn::OnCameraDragEnd);
		}

		if (CameraLookAction)
		{
			EnhancedInputComponent->BindAction(CameraLookAction, ETriggerEvent::Triggered, this, &AChessCameraPawn::OnCameraLook);
		}
	}
}

void AChessCameraPawn::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	// Use spring arm world yaw for camera-relative movement
	const FRotator YawRotation(0, SpringArm->GetComponentRotation().Yaw, 0);

	// Forward/Backward
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, MovementVector.Y);

	// Right/Left
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void AChessCameraPawn::OnCameraZoom(const FInputActionValue& Value)
{
	float ZoomValue = Value.Get<float>();
	float NewArmLength = SpringArm->TargetArmLength - (ZoomValue * ZoomSpeed);
	SpringArm->TargetArmLength = FMath::Clamp(NewArmLength, MinZoomDistance, MaxZoomDistance);
}

void AChessCameraPawn::OnCameraDragStart(const FInputActionValue& Value)
{
	bIsDragging = true;
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void AChessCameraPawn::OnCameraDragEnd(const FInputActionValue& Value)
{
	bIsDragging = false;
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->SetShowMouseCursor(true);
		PC->SetInputMode(FInputModeGameAndUI());
	}
}

void AChessCameraPawn::OnCameraLook(const FInputActionValue& Value)
{
	if (!bIsDragging) return;

	FVector2D LookValue = Value.Get<FVector2D>();
	CameraYaw += LookValue.X * LookSensitivity;
	CameraPitch = FMath::Clamp(CameraPitch + LookValue.Y * LookSensitivity, -89.0f, 89.0f);
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
}
