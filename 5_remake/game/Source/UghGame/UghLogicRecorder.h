// The inputs of the logic written down, for the tests.
#pragma once

#include "CoreMinimal.h"
#include "UghControls.h"
#include "ugh_logic.h"

/** Writes down the inputs FUghControls gives the logic ("key 0 up on", "menu other"), for the tests to compare. */
class FUghLogicRecorder : public IUghLogicInput
{
public:
	virtual void Key(int32 Player, int32 LogicKey, bool bPressed) override
	{
		static const TCHAR* const Keys[] = { TEXT("up"), TEXT("down"), TEXT("left"), TEXT("right"), TEXT("fire") };
		Inputs.Add(FString::Printf(TEXT("key %d %s %s"), Player, Keys[LogicKey], bPressed ? TEXT("on") : TEXT("off")));
	}

	virtual void MenuKey(int32 LogicMenuKey) override
	{
		Inputs.Add(LogicMenuKey == UGH_LOGIC_MENU_ESCAPE ? TEXT("menu escape")
			: LogicMenuKey == UGH_LOGIC_MENU_PAUSE ? TEXT("menu pause") : TEXT("menu other"));
	}

	/** What was written down since the last time, separated by commas. */
	FString Take()
	{
		const FString Taken = FString::Join(Inputs, TEXT(", "));
		Inputs.Reset();
		return Taken;
	}

private:
	TArray<FString> Inputs;
};
