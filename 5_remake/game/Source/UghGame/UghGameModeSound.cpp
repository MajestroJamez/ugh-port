// The game's ambience: what is around the camera heard (FUghAmbience).
#include "UghFringe.h"
#include "UghGameMode.h"
#include "UghMood.h"
#include "UghShapes.h"
#include "UghSpeaker.h"

void AUghGameMode::Hear(const ugh_logic_view& View, double Surface, double Shown, bool bMenuView, bool bIsles,
	const FUghCameraPose& Pose)
{
	using EPlace = FUghAmbience::EPlace;
	using ESource = FUghAmbience::ESource;
	FUghAmbience& Ambience = Speaker->GetPlayer().GetAmbience();
	const EPlace Place = Intro.IsFlying() ? EPlace::Flight : bIsles ? EPlace::Isles : bMenuView ? EPlace::Menu
		: EPlace::Play;
	// heard as much as seen: silent in the black between the levels (the mood changes there), fading in and out with
	// the picture; nothing without a level
	const float Presence = View.level_id >= 0 ? float(Shown) : 0.f;
	Ambience.SetScene(UghMood::Of(View.level, View.wind).Kind, Place, Presence);
	Ambience.SetListener(Pose.Location, Pose.Rotation);
	// the campfires' and torches' flames, burning while the water is below them (as AUghCampfire, AUghTorches)
	TArray<FUghAmbience::FFire, TInlineAllocator<32>> Fires;
	for (const FUghDecoration& Fire : HeardFires)
	{
		const bool bTorch = Fire.Kind == FUghDecoration::EKind::Torch;
		Fires.Add({ UghShapes::ToWorld(Fire.X, bTorch ? Fire.Top() : Fire.Y - Fire.Height * 0.3, Fire.Depth),
			bTorch ? ESource::Torch : ESource::Campfire, (bTorch ? Fire.Top() : Fire.Y) < Surface });
	}
	Ambience.SetFires(Fires);
	FVector Rustling = FVector::ZeroVector;
	const float Rustle = Fringe->Rustle(Rustling);
	Ambience.SetRustle(Rustle, Rustling);
}

void AUghGameMode::HearFires(const TArray<FUghDecoration>& Decorations)
{
	HeardFires = Decorations.FilterByPredicate([](const FUghDecoration& Decoration)
	{
		return Decoration.Kind == FUghDecoration::EKind::Campfire || Decoration.Kind == FUghDecoration::EKind::Torch;
	});
}
