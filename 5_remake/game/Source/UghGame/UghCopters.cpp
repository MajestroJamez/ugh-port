#include "UghCopters.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UghAssets.h"
#include "UghBetween.h"
#include "UghCopterModel.h"
#include "UghShapes.h"

namespace
{
	using UghShapes::EShape;

	/** The clay copters (the players' colours) and their rotors. */
	const FLinearColor ClayColors[] = { FLinearColor(0.9f, 0.35f, 0.03f), FLinearColor(0.03f, 0.5f, 0.6f) };
	const FLinearColor ClayRotorColor(0.35f, 0.22f, 0.1f);

	/** The middle of the body across, in pixels from the copter's corner. */
	constexpr double CopterMiddle = (UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0;
	/** Clay: the rotor above the body, the passenger in the cabin or hanging below (pixels). */
	constexpr double RotorWidth = 28, RotorHeight = 1.5;
	constexpr double RiderSize = 8;
	constexpr double HangingTop = UghShapes::CopterBodyHeight + 1;
	constexpr double FigureDepth = 0, FigureThickness = UghShapes::PlaneThickness;

	/** The sling sways against the copter's way: radians per pixel a step, at most, how slowly (seconds). */
	constexpr double SwayPerPixel = 0.12, MaxSway = 0.25, SwaySeconds = 0.4;

	UStaticMeshComponent* AddPart(AActor* Owner, UStaticMesh* Mesh, USceneComponent* Parent, const FVector& At)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
		Part->SetStaticMesh(Mesh);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetupAttachment(Parent);
		Part->SetRelativeLocation(At);
		Part->SetVisibility(false);
		Part->RegisterComponent();
		Owner->AddInstanceComponent(Part);
		return Part;
	}
}

AUghCopters::AUghCopters()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghCopters::BeginPlay()
{
	Super::BeginPlay();
	if (LoadModels())
	{
		return;
	}
	for (const FLinearColor& Color : ClayColors)
	{
		ClayBodies.Add(UghShapes::AddShapes(this, EShape::Cube, UghShapes::Clay(this, Color)));
	}
	ClayRotors = UghShapes::AddShapes(this, EShape::Cube, UghShapes::Clay(this, ClayRotorColor));
}

bool AUghCopters::LoadModels()
{
	using namespace UghCopterModel;
	UStaticMesh* CrankMesh = UghAssets::Mesh(UghAssets::Copter, Crank);
	UStaticMesh* SlingMesh = UghAssets::Mesh(UghAssets::Copter, Sling);
	UStaticMesh* StoneMesh = UghAssets::Mesh(UghAssets::StonePassenger, StonePassenger);
	TArray<UStaticMesh*> BodyMeshes, RotorMeshes;
	for (int32 Player = 0; Player < UE_ARRAY_COUNT(Bodies); ++Player)
	{
		BodyMeshes.Add(UghAssets::Mesh(UghAssets::Copter, Bodies[Player]));
		RotorMeshes.Add(UghAssets::Mesh(UghAssets::Copter, Rotors[Player]));
	}
	if (!Caveman.Load() || !CrankMesh || !SlingMesh || !StoneMesh || BodyMeshes.Contains(nullptr) ||
		RotorMeshes.Contains(nullptr))
	{
		UE_LOG(LogTemp, Display, TEXT("UGH the copters are clay (their models are not imported)"));
		return false;
	}
	for (int32 Player = 0; Player < BodyMeshes.Num(); ++Player)
	{
		FUghCopterParts& Parts = Models.AddDefaulted_GetRef();
		Parts.Body = AddPart(this, BodyMeshes[Player], RootComponent, FVector::ZeroVector);
		Parts.Rotor = AddPart(this, RotorMeshes[Player], Parts.Body, RotorHub);
		Parts.Crank = AddPart(this, CrankMesh, Parts.Body, CrankAxle);
		Parts.Sling = AddPart(this, SlingMesh, Parts.Body, FVector::ZeroVector);
		Parts.Stone = AddPart(this, StoneMesh, Parts.Sling, Hanging);
		Parts.Pilot = Caveman.Add(this);
		Parts.Pilot->AttachToComponent(Parts.Body, FAttachmentTransformRules::KeepRelativeTransform);
		Parts.Pilot->SetRelativeLocation(PilotSeat);
		FUghCaveman::Dress(Parts.Pilot, FUghCaveman::Pilot);
		Parts.Rider = Caveman.Add(this);
		Parts.Rider->AttachToComponent(Parts.Body, FAttachmentTransformRules::KeepRelativeTransform);
		Parts.Rider->SetRelativeLocation(PassengerSeat);
	}
	return true;
}

void AUghCopters::Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
	TArray<FTransform>& OutClayRiders)
{
	const bool bPlay = Current.phase == UGH_LOGIC_PHASE_PLAY && Current.level_id >= 0;
	const ugh_logic_view& From = UghBetween::From(Previous, Current);
	if (Models.IsEmpty())
	{
		ShowClay(From, Current, Alpha, OutClayRiders);
		return;
	}
	for (int32 Player = 0; Player < Models.Num(); ++Player)
	{
		if (!bPlay || Player >= Current.copter_count)
		{
			HideModel(Models[Player]);
			continue;
		}
		const ugh_logic_copter& To = Current.copters[Player];
		ShowModel(Models[Player], Player < From.copter_count ? From.copters[Player] : To, To, Alpha, Seconds);
	}
}

void AUghCopters::ShowModel(FUghCopterParts& Parts, const ugh_logic_copter& From, const ugh_logic_copter& To,
	double Alpha, double Seconds)
{
	const FVector2D At = UghBetween::Position(From.x, From.y, To.x, To.y, Alpha);
	Parts.Body->SetWorldLocation(UghShapes::ToWorld(At.X + CopterMiddle, At.Y + UghShapes::CopterBodyHeight, 0));
	Parts.Spin.Update(To.rotor_sprite, Seconds);
	Parts.Rotor->SetRelativeRotation(FRotator(0, 360 * Parts.Spin.RotorTurn(), 0));
	const double Crank = UghCopterModel::CrankDirection * UE_TWO_PI * Parts.Spin.PedalTurn();
	Parts.Crank->SetRelativeRotation(FQuat(FVector::XAxisVector, Crank));
	Caveman.Hold(Parts.Pilot, EUghCaveAction::Pedal, Parts.Spin.PedalTurn());
	for (USceneComponent* Part : TArray<USceneComponent*>{ Parts.Body, Parts.Rotor, Parts.Crank, Parts.Pilot })
	{
		Part->SetVisibility(true);
	}
	const double Velocity = UghBetween::Moved(From.x, From.y, To.x, To.y)
		? double(To.x - From.x) / UghShapes::Subpixels : 0;
	ShowCargo(Parts, To, Seconds, Velocity);
}

/** The passenger: sitting behind the pilot as a caveman of his look, or the stone passenger hanging in the sling. */
void AUghCopters::ShowCargo(FUghCopterParts& Parts, const ugh_logic_copter& Copter, double Seconds, double Velocity)
{
	const bool bHangs = Copter.cargo_look != 0 && Copter.destination < 0;
	const FUghCaveLook* Look = bHangs ? nullptr : FUghCaveman::Passenger(Copter.cargo_look);
	Parts.Sling->SetVisibility(bHangs);
	Parts.Stone->SetVisibility(bHangs);
	Parts.Rider->SetVisibility(Look != nullptr);
	const double Wanted = bHangs ? FMath::Clamp(Velocity * SwayPerPixel, -MaxSway, MaxSway) : 0;
	Parts.Sway = FMath::Lerp(Wanted, Parts.Sway, FMath::Exp(-Seconds / SwaySeconds));
	Parts.Sling->SetRelativeRotation(FQuat(FVector::YAxisVector, Parts.Sway));
	if (Look && Parts.RiderLook != Copter.cargo_look)
	{
		FUghCaveman::Dress(Parts.Rider, *Look);
		Parts.RiderLook = Copter.cargo_look;
	}
	if (Look)
	{
		Caveman.Play(Parts.Rider, EUghCaveAction::Sit);
	}
}

void AUghCopters::HideModel(FUghCopterParts& Parts)
{
	const TArray<USceneComponent*> All = { Parts.Body, Parts.Rotor, Parts.Crank, Parts.Sling, Parts.Stone, Parts.Pilot,
		Parts.Rider };
	for (USceneComponent* Part : All)
	{
		Part->SetVisibility(false);
	}
}

void AUghCopters::ShowClay(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
	TArray<FTransform>& OutRiders)
{
	const bool bPlay = Current.phase == UGH_LOGIC_PHASE_PLAY && Current.level_id >= 0;
	TArray<FTransform> RotorBoxes;
	for (int32 Player = 0; Player < ClayBodies.Num(); ++Player)
	{
		TArray<FTransform> Body;
		if (bPlay && Player < Current.copter_count)
		{
			const ugh_logic_copter& C = Current.copters[Player];
			const ugh_logic_copter& P = Player < Previous.copter_count ? Previous.copters[Player] : C;
			const FVector2D At = UghBetween::Position(P.x, P.y, C.x, C.y, Alpha);
			Body.Add(UghShapes::Box(At.X + UghShapes::CopterBodyLeft, At.Y + RotorHeight,
				UghShapes::CopterBodyRight - UghShapes::CopterBodyLeft + 1, UghShapes::CopterBodyHeight - RotorHeight,
				FigureDepth, FigureThickness));
			// the rotor's sprites turn it: a blade that gets shorter and longer
			const double Blade = RotorWidth * (1 + C.rotor_sprite % 3) / 3;
			RotorBoxes.Add(UghShapes::Box(At.X + CopterMiddle - Blade / 2, At.Y, Blade, RotorHeight, FigureDepth,
				FigureThickness / 2));
			if (C.cargo_look != 0)
			{
				const double Top = C.destination < 0 ? HangingTop : (UghShapes::CopterBodyHeight - RiderSize) / 2;
				OutRiders.Add(UghShapes::Box(At.X + CopterMiddle - RiderSize / 2, At.Y + Top, RiderSize, RiderSize,
					FigureDepth - FigureThickness / 2, FigureThickness / 2));
			}
		}
		UghShapes::SetShapes(ClayBodies[Player], Body);
	}
	UghShapes::SetShapes(ClayRotors, RotorBoxes);
}
