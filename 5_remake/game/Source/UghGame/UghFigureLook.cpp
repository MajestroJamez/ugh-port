#include "UghFigureLook.h"

#include "Components/PrimitiveComponent.h"

void UghFigureLook::Mark(USceneComponent* Root, bool bHalo)
{
	static_assert(Channel == 1, "the figures and the figure light (AUghStage) are in channel 1");
	TArray<USceneComponent*> Parts;
	Root->GetChildrenComponents(true, Parts);
	Parts.Add(Root);
	for (USceneComponent* Part : Parts)
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Part))
		{
			Primitive->SetLightingChannels(true, true, false);
			Primitive->SetRenderCustomDepth(bHalo);
		}
	}
}
