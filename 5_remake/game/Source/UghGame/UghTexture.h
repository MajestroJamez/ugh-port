// Pixels as a texture.
#pragma once

#include "CoreMinimal.h"

class UTexture2D;

namespace UghTexture
{
	/** A texture of `Width` x `Height` pixels (row by row, sRGB), filtered or crisp; nullptr when the sizes differ. */
	UTexture2D* Create(UObject* Outer, int32 Width, int32 Height, const TArray<FColor>& Pixels, bool bCrisp);
}
