#include "UghPasswords.h"

#include "Dom/JsonObject.h"

bool FUghPasswords::Load(const FJsonObject& LevelsFile, const FString& Path, FString& OutError)
{
	OnePlayer.Reset();
	Team.Reset();
	if (!LevelsFile.TryGetStringArrayField(TEXT("passwordsOnePlayer"), OnePlayer) ||
		!LevelsFile.TryGetStringArrayField(TEXT("passwordsTeam"), Team) || OnePlayer.IsEmpty() || Team.IsEmpty())
	{
		OutError = FString::Printf(TEXT("%s: no passwords"), *Path);
		return false;
	}
	return true;
}

int32 FUghPasswords::Find(int32 Players, const FString& Password) const
{
	return Of(Players).IndexOfByPredicate(
		[&](const FString& Known) { return Known.Equals(Password, ESearchCase::IgnoreCase); });
}
