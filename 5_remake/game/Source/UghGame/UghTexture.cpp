#include "UghTexture.h"

#include "Engine/Texture2D.h"
#include "TextureResource.h"

UTexture2D* UghTexture::Create(UObject* Outer, int32 Width, int32 Height, const TArray<FColor>& Pixels, bool bCrisp)
{
	if (Width <= 0 || Height <= 0 || Pixels.Num() != Width * Height)
	{
		return nullptr;
	}
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	Texture->Rename(nullptr, Outer);
	Texture->SRGB = true;
	Texture->Filter = bCrisp ? TF_Nearest : TF_Bilinear;
	Texture->AddressX = Texture->AddressY = TA_Clamp;
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	FMemory::Memcpy(Mip.BulkData.Lock(LOCK_READ_WRITE), Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}
