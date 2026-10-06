// Copies the assets of the Electric Dreams sample the game uses.
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UghCopyElectricDreamsCommandlet.generated.h"

/**
 * Copies the assets of Epic's Electric Dreams sample the game asks for (UghElectricDreams::All) and every package
 * they need (their hard dependencies in the sample's content, followed through the sample's asset registry data)
 * from the sample's project to Content/External/ElectricDreams, file by file (.uasset, .uexp, .ubulk ...), so the
 * sample's /Game/<path> is UghElectricDreams::Root/<path> here; the [CoreRedirects] of DefaultEngine.ini send the
 * copied packages' references there (checked: a top folder without its redirect fails). The sample is only read:
 * its content is mounted at /UghSample/ for the registry, nothing is written there. electric-dreams.ps1 runs it:
 * UnrealEditor-Cmd UghGame.uproject -run=UghCopyElectricDreams -Source=<the sample's project folder>. Idempotent
 * (a file of the same size and time is left alone; what the game no longer needs is deleted); then it repairs what
 * the cook cannot take (UghElectricDreamsRepair: a material saved again is copied anew next time) and loads every
 * asset the game asks for, so the meshes' Nanite data is built once here, not in the game. Fails when
 * the sample or one of the assets is missing, or a copied asset does not load.
 */
UCLASS()
class UUghCopyElectricDreamsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUghCopyElectricDreamsCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	/**
	 * The packages the game's assets need in the sample's content `Content` (paths below its /Game, the assets
	 * themselves too); false when one is missing there.
	 */
	static bool Gather(const FString& Content, TSet<FString>& Packages);
	/**
	 * Copies the files of `Packages` from `Content` (adding the bytes copied now to `Copied`, those of the whole copy to
	 * `Total`), deletes the other files of the copy; false when a file could not be copied.
	 */
	static bool Copy(const FString& Content, const TSet<FString>& Packages, int64& Copied, int64& Total);
	/** Every top folder of `Packages` is redirected to the copy (DefaultEngine.ini); false (and a log) when not. */
	static bool Redirected(const TSet<FString>& Packages);
	/** Loads every asset the game asks for (the meshes' Nanite data is built); false when one does not load. */
	static bool LoadAll();
};
