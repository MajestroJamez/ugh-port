// What the copy of the Electric Dreams sample needs to cook.
#pragma once

#include "CoreMinimal.h"

/**
 * Repairs of the copy of the Electric Dreams sample (UUghCopyElectricDreamsCommandlet, after each copy). Its scanned
 * assets' master materials convert material attributes to Substrate with a legacy conversion node fed by a break of the
 * attributes, its Anisotropy input too, although they set none: the material's cached connections say no anisotropy,
 * its translation says some, and the cook stops on the shader map it then expects (FAnisotropyPS missing). Such an
 * Anisotropy input is cut (the anisotropy was nothing), the material saved again.
 */
namespace UghElectricDreamsRepair
{
	/** Repairs the materials under `Root` (a package path); false when one could not be saved. */
	bool Materials(const FString& Root);
}
