#include "UghKeyBindings.h"

#include "Algo/Find.h"
#include "Dom/JsonObject.h"
#include "ugh_logic.h"

namespace
{
	const TCHAR* const ActionNames[FUghKeyBindings::Actions] = {
		TEXT("Up"), TEXT("Down"), TEXT("Left"), TEXT("Right"), TEXT("Fire") };
	static_assert(UGH_LOGIC_KEY_UP == 0 && UGH_LOGIC_KEY_FIRE == FUghKeyBindings::Actions - 1);

	FString PilotField(int32 Pilot)
	{
		return FString::Printf(TEXT("pilot%d"), Pilot + 1);
	}
}

FUghKeyBindings FUghKeyBindings::Defaults()
{
	FUghKeyBindings Bindings;
	auto Set = [&](int32 Pilot, int32 Action, const FKey& First, const FKey& Second = FKey())
	{
		Bindings.Keys[Pilot][Action][0] = First;
		Bindings.Keys[Pilot][Action][1] = Second;
	};
	Set(0, UGH_LOGIC_KEY_UP, EKeys::Up);
	Set(0, UGH_LOGIC_KEY_DOWN, EKeys::Down);
	Set(0, UGH_LOGIC_KEY_LEFT, EKeys::Left);
	Set(0, UGH_LOGIC_KEY_RIGHT, EKeys::Right);
	Set(0, UGH_LOGIC_KEY_FIRE, EKeys::RightControl, EKeys::SpaceBar);
	Set(1, UGH_LOGIC_KEY_UP, EKeys::W);
	Set(1, UGH_LOGIC_KEY_DOWN, EKeys::S);
	Set(1, UGH_LOGIC_KEY_LEFT, EKeys::A);
	Set(1, UGH_LOGIC_KEY_RIGHT, EKeys::D);
	Set(1, UGH_LOGIC_KEY_FIRE, EKeys::LeftControl);
	return Bindings;
}

FKey FUghKeyBindings::KeyOf(int32 Pilot, int32 Action) const
{
	const FKey (&Both)[Slots] = Keys[Pilot][Action];
	return Both[0].IsValid() ? Both[0] : Both[1];
}

TOptional<FUghKeyBindings::FPlace> FUghKeyBindings::Find(const FKey& Key) const
{
	for (int32 Pilot = 0; Pilot < Pilots && Key.IsValid(); ++Pilot)
	{
		for (int32 Action = 0; Action < Actions; ++Action)
		{
			for (int32 Slot = 0; Slot < Slots; ++Slot)
			{
				if (Keys[Pilot][Action][Slot] == Key)
				{
					return FPlace{ Pilot, Action, Slot };
				}
			}
		}
	}
	return {};
}

FUghKeyBindings::EBound FUghKeyBindings::Bind(const FPlace& Place, const FKey& Key, FPlace* OutOther)
{
	if (IsReserved(Key))
	{
		return EBound::Reserved;
	}
	FKey& Slot = Keys[Place.Pilot][Place.Action][Place.Slot];
	const TOptional<FPlace> Other = Find(Key);
	const bool bSwapped = Other && *Other != Place;
	if (bSwapped)
	{
		Keys[Other->Pilot][Other->Action][Other->Slot] = Slot;
		if (OutOther)
		{
			*OutOther = *Other;
		}
	}
	Slot = Key;
	return bSwapped ? EBound::Swapped : EBound::Bound;
}

bool FUghKeyBindings::IsReserved(const FKey& Key)
{
	static const FKey Game[] = { EKeys::Escape, EKeys::P, EKeys::F1, EKeys::F5, EKeys::U, EKeys::G, EKeys::PageUp,
		EKeys::PageDown };
	return !Key.IsValid() || Key.IsMouseButton() || Key.IsGamepadKey() || Key.IsTouch() || Key.IsAxis1D() ||
		Key.IsAxis2D() || Key.IsAxis3D() || Algo::Find(Game, Key) != nullptr;
}

FString FUghKeyBindings::NameOf(const FKey& Key)
{
	static const TMap<FKey, const TCHAR*> Short = {
		{ EKeys::Up, TEXT("↑") }, { EKeys::Down, TEXT("↓") }, { EKeys::Left, TEXT("←") }, { EKeys::Right, TEXT("→") },
		{ EKeys::SpaceBar, TEXT("Space") }, { EKeys::LeftControl, TEXT("Left Ctrl") },
		{ EKeys::RightControl, TEXT("Right Ctrl") }, { EKeys::LeftShift, TEXT("Left Shift") },
		{ EKeys::RightShift, TEXT("Right Shift") }, { EKeys::LeftAlt, TEXT("Left Alt") },
		{ EKeys::RightAlt, TEXT("Right Alt") }, { EKeys::Enter, TEXT("Enter") }, { EKeys::BackSpace, TEXT("Backspace") },
	};
	if (!Key.IsValid())
	{
		return TEXT("-");
	}
	const TCHAR* const* Found = Short.Find(Key);
	return Found ? FString(*Found) : Key.GetDisplayName(false).ToString();
}

const TCHAR* FUghKeyBindings::ActionName(int32 Action)
{
	return ActionNames[FMath::Clamp(Action, 0, Actions - 1)];
}

TSharedRef<FJsonObject> FUghKeyBindings::ToJson() const
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	for (int32 Pilot = 0; Pilot < Pilots; ++Pilot)
	{
		TSharedRef<FJsonObject> Fields = MakeShared<FJsonObject>();
		for (int32 Action = 0; Action < Actions; ++Action)
		{
			TArray<TSharedPtr<FJsonValue>> Names;
			for (const FKey& Key : Keys[Pilot][Action])
			{
				Names.Add(MakeShared<FJsonValueString>(Key.IsValid() ? Key.GetFName().ToString() : FString()));
			}
			Fields->SetArrayField(FString(ActionNames[Action]).ToLower(), Names);
		}
		Json->SetObjectField(PilotField(Pilot), Fields);
	}
	return Json;
}

FUghKeyBindings FUghKeyBindings::FromJson(const FJsonObject& Json)
{
	FUghKeyBindings Bindings = Defaults();
	for (int32 Pilot = 0; Pilot < Pilots; ++Pilot)
	{
		const TSharedPtr<FJsonObject>* Fields = nullptr;
		if (!Json.TryGetObjectField(PilotField(Pilot), Fields))
		{
			continue;
		}
		for (int32 Action = 0; Action < Actions; ++Action)
		{
			const TArray<TSharedPtr<FJsonValue>>* Names = nullptr;
			if (!(*Fields)->TryGetArrayField(FString(ActionNames[Action]).ToLower(), Names))
			{
				continue;
			}
			for (int32 Slot = 0; Slot < FMath::Min(Slots, Names->Num()); ++Slot)
			{
				const FString Name = (*Names)[Slot]->AsString();
				const FKey Key{ FName(*Name) };
				if (Name.IsEmpty() || (Key.IsValid() && !IsReserved(Key)))
				{
					Bindings.Keys[Pilot][Action][Slot] = Name.IsEmpty() ? FKey() : Key;
				}
			}
		}
	}
	return Bindings;
}

bool FUghKeyBindings::operator==(const FUghKeyBindings& Other) const
{
	for (int32 Pilot = 0; Pilot < Pilots; ++Pilot)
	{
		for (int32 Action = 0; Action < Actions; ++Action)
		{
			for (int32 Slot = 0; Slot < Slots; ++Slot)
			{
				if (Keys[Pilot][Action][Slot] != Other.Keys[Pilot][Action][Slot])
				{
					return false;
				}
			}
		}
	}
	return true;
}
