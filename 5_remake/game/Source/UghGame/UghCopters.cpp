#include "UghCopters.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UghAssets.h"
#include "UghBetween.h"
#include "UghCopterModel.h"
#include "UghFigureLook.h"
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

	/** The shadows under the copters (AUghCopterShadows) do not darken them: none of the parts of `Copters` takes a decal. */
	void KeepOffDecals(AActor* Copters)
	{
		for (UPrimitiveComponent* Part : TInlineComponentArray<UPrimitiveComponent*>(Copters))
		{
			Part->SetReceivesDecals(false);
		}
	}
}

AUghCopters::AUghCopters()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghCopters::BeginPlay()
{
	Super::BeginPlay();
	if (!LoadModels())
	{
		for (const FLinearColor& Color : ClayColors)
		{
			ClayBodies.Add(UghShapes::AddShapes(this, EShape::Cube, UghShapes::Clay(this, Color)));
		}
		ClayRotors = UghShapes::AddShapes(this, EShape::Cube, UghShapes::Clay(this, ClayRotorColor));
	}
	KeepOffDecals(this);
}

bool AUghCopters::LoadModels()
{
	using namespace UghCopterModel;
	UStaticMesh* CrankMesh = UghAssets::Mesh(UghAssets::Copter, Crank);
	UStaticMesh* ShaftMesh = UghAssets::Mesh(UghAssets::Copter, Shaft);
	UStaticMesh* DriveMesh = UghAssets::Mesh(UghAssets::Copter, Drive);
	UStaticMesh* LinkMesh = UghAssets::Mesh(UghAssets::Copter, ChainLink);
	UStaticMesh* SlingMesh = UghAssets::Mesh(UghAssets::Copter, Sling);
	UStaticMesh* StoneMesh = UghAssets::Stone();
	TArray<UStaticMesh*> BodyMeshes, RotorMeshes;
	for (int32 Player = 0; Player < UE_ARRAY_COUNT(Bodies); ++Player)
	{
		BodyMeshes.Add(UghAssets::Mesh(UghAssets::Copter, Bodies[Player]));
		RotorMeshes.Add(UghAssets::Mesh(UghAssets::Copter, Rotors[Player]));
	}
	if (!Caveman.Load() || !CrankMesh || !ShaftMesh || !DriveMesh || !LinkMesh || !SlingMesh || !StoneMesh ||
		BodyMeshes.Contains(nullptr) ||
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
		Parts.Shaft = AddPart(this, ShaftMesh, Parts.Rotor, FVector::ZeroVector);
		Parts.Crank = AddPart(this, CrankMesh, Parts.Body, CrankAxle);
		Parts.Drive = AddPart(this, DriveMesh, Parts.Body, Sprocket);
		Parts.Chain = NewObject<UInstancedStaticMeshComponent>(this);
		Parts.Chain->SetStaticMesh(LinkMesh);
		Parts.Chain->SetMobility(EComponentMobility::Movable);
		Parts.Chain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Parts.Chain->SetupAttachment(Parts.Body);
		Parts.Chain->SetVisibility(false);
		Parts.Chain->RegisterComponent();
		AddInstanceComponent(Parts.Chain);
		Chain.Place(0, ChainLinks);
		Parts.Chain->AddInstances(ChainLinks, false);
		Parts.Sling = AddPart(this, SlingMesh, Parts.Body, FVector::ZeroVector);
		Parts.Stone = AddPart(this, StoneMesh, Parts.Sling, Hanging);
		Parts.SeatedStone = AddPart(this, StoneMesh, Parts.Body, PassengerSeat);
		Parts.SeatedStone->SetRelativeScale3D(FVector(SeatedStone));
		UghFigureLook::Mark(Parts.Stone);
		UghFigureLook::Mark(Parts.SeatedStone, false);
		Parts.Pilot = Caveman.Add(this, FUghCaveman::PilotLook);
		Parts.Pilot->AttachToComponent(Parts.Body, FAttachmentTransformRules::KeepRelativeTransform);
		Parts.Pilot->SetRelativeLocationAndRotation(PilotSeat, FRotator(0, PilotYaw, 0));
		UghFigureLook::Mark(Parts.Pilot, false);
	}
	return true;
}

void AUghCopters::Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
	TArray<FTransform>& OutClayRiders, const FUghDunks* Dunks)
{
	const bool bPlay = Current.phase == UGH_LOGIC_PHASE_PLAY && Current.level_id >= 0;
	const ugh_logic_view& From = UghBetween::From(Previous, Current);
	if (Models.IsEmpty())
	{
		ShowClay(From, Current, Alpha, OutClayRiders, Dunks);
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
		ShowModel(Models[Player], Player < From.copter_count ? From.copters[Player] : To, To, Alpha, Seconds,
			Dunks ? Dunks->Of(Player) : FUghCopterBob());
	}
}

void AUghCopters::ShowModel(FUghCopterParts& Parts, const ugh_logic_copter& From, const ugh_logic_copter& To,
	double Alpha, double Seconds, const FUghCopterBob& Bob)
{
	const FVector2D At = UghBetween::Position(From.x, From.y, To.x, To.y, Alpha);
	// afloat it bobs and rocks (FUghDunks) about the middle of its waterline
	const FVector Pivot = UghShapes::ToWorld(At.X + CopterMiddle, At.Y + FUghDunks::Waterline - Bob.Lift, 0);
	const FQuat Rock(FVector::YAxisVector, Bob.Roll);
	const FVector Bottom = UghShapes::ToWorld(At.X + CopterMiddle, At.Y + UghShapes::CopterBodyHeight - Bob.Lift, 0);
	Parts.Body->SetWorldLocationAndRotation(Pivot + Rock.RotateVector(Bottom - Pivot), Rock);
	Parts.Spin.Update(To.rotor_sprite, Seconds);
	Parts.Rotor->SetRelativeRotation(FRotator(0, 360 * Parts.Spin.RotorTurn(), 0));
	// the crank, the chain and the sprocket on the layshaft (Ratio times the crank's turns: the rotor's)
	const double Crank = UghCopterModel::CrankDirection * UE_TWO_PI * Parts.Spin.PedalTurn();
	const double Layshaft = UghCopterModel::CrankDirection * UE_TWO_PI * Parts.Spin.RotorTurn();
	Parts.Crank->SetRelativeRotation(UghCopterModel::PilotTurn * FQuat(FVector::XAxisVector, Crank));
	Parts.Drive->SetRelativeRotation(UghCopterModel::PilotTurn * FQuat(FVector::XAxisVector, Layshaft));
	Chain.Place(Parts.Spin.PedalTurn(), ChainLinks);
	Parts.Chain->BatchUpdateInstancesTransforms(0, ChainLinks, false, true);
	Caveman.Hold(Parts.Pilot, EUghCaveAction::Pedal, Parts.Spin.PedalTurn());
	for (USceneComponent* Part : TArray<USceneComponent*>{ Parts.Body, Parts.Rotor, Parts.Shaft, Parts.Crank,
		Parts.Drive, Parts.Chain, Parts.Pilot })
	{
		Part->SetVisibility(true, Part == Parts.Pilot);
	}
	const double Velocity = UghBetween::Moved(From.x, From.y, To.x, To.y)
		? double(To.x - From.x) / UghShapes::Subpixels : 0;
	ShowCargo(Parts, To, Seconds, Velocity);
}

/**
 * The passenger: sitting behind the pilot as a person of his look (the stone passenger smaller on the seat), or the
 * stone passenger hanging in the sling.
 */
void AUghCopters::ShowCargo(FUghCopterParts& Parts, const ugh_logic_copter& Copter, double Seconds, double Velocity)
{
	const bool bHangs = Copter.cargo_look != 0 && Copter.destination < 0;
	const bool bSits = !bHangs && FUghCaveman::IsPassenger(Copter.cargo_look);
	Parts.Sling->SetVisibility(bHangs);
	Parts.Stone->SetVisibility(bHangs);
	Parts.SeatedStone->SetVisibility(!bHangs && Copter.cargo_look != 0 && !bSits);
	const double Wanted = bHangs ? FMath::Clamp(Velocity * SwayPerPixel, -MaxSway, MaxSway) : 0;
	Parts.Sway = FMath::Lerp(Wanted, Parts.Sway, FMath::Exp(-Seconds / SwaySeconds));
	Parts.Sling->SetRelativeRotation(FQuat(FVector::YAxisVector, Parts.Sway));
	if (bSits && Parts.RiderLook != Copter.cargo_look)
	{
		if (Parts.Rider)
		{
			Caveman.Release(Parts.Rider);
		}
		Parts.Rider = Caveman.Add(this, Copter.cargo_look);
		Parts.Rider->AttachToComponent(Parts.Body, FAttachmentTransformRules::KeepRelativeTransform);
		Parts.Rider->SetRelativeLocationAndRotation(UghCopterModel::PassengerSeat,
			FRotator(0, UghCopterModel::PassengerYaw, 0));
		UghFigureLook::Mark(Parts.Rider, false);
		Parts.RiderLook = Copter.cargo_look;
	}
	if (Parts.Rider)
	{
		Parts.Rider->SetVisibility(bSits, true);
	}
	if (bSits)
	{
		Caveman.Play(Parts.Rider, EUghCaveAction::Sit);
	}
}

void AUghCopters::HideModel(FUghCopterParts& Parts)
{
	const TArray<USceneComponent*> All = { Parts.Body, Parts.Rotor, Parts.Shaft, Parts.Crank, Parts.Drive,
		Parts.Chain, Parts.Sling, Parts.Stone, Parts.SeatedStone, Parts.Pilot, Parts.Rider };
	for (USceneComponent* Part : All)
	{
		if (Part)
		{
			Part->SetVisibility(false, true);
		}
	}
}

void AUghCopters::ShowClay(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
	TArray<FTransform>& OutRiders, const FUghDunks* Dunks)
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
			const FVector2D At = UghBetween::Position(P.x, P.y, C.x, C.y, Alpha) -
				FVector2D(0, Dunks ? Dunks->Of(Player).Lift : 0.0);
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

void AUghCopters::Stock()
{
	Caveman.Stock(this, RiderSpares);
	KeepOffDecals(this);
}
