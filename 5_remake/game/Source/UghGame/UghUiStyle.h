// The look of the menu and the HUD.
#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class UTexture2D;
class SWidget;

/**
 * The look of the screen (UghUi): its fonts - Lilita One for titles and numbers, Alegreya Sans for the text (free,
 * OFL: Assets.json, fetched to assets/3d/googlefonts; else the engine's Roboto) -, its colours (warm bone and amber
 * of firelight on dark glass), its panels and the pictures carved by UghStoneArt. Sizes are the slate units of a
 * screen 1080 pixels high (the engine scales them by the screen's DPI curve).
 */
namespace UghUiStyle
{
	extern const FLinearColor Bone, Muted, Amber, Ember, Leaf, Ink, Shadow, Glass, Edge;

	/** Looks for the fonts under the game's data folder `AssetsDir` (else the engine's, logged). */
	void FindFonts(const FString& AssetsDir);
	/** Lilita One: titles and numbers; `Outline` pixels of a dark outline. */
	FSlateFontInfo Display(float Size, float Outline = 0);
	/** Alegreya Sans: the text. */
	FSlateFontInfo Text(float Size, bool bBold = false);

	/** A text block's style: the font, its colour, a soft dark shadow below it. */
	FTextBlockStyle TextStyle(const FSlateFontInfo& Font, const FLinearColor& Color);

	/** Dark glass with a thin warm edge: the panels. */
	const FSlateBrush* Panel();
	/** The chosen row of the menu: amber glass. */
	const FSlateBrush* Chosen();
	/** A small outlined key cap of the keys' hints. */
	const FSlateBrush* KeyCap();
	/** A field to type into (dark, an amber edge when chosen). */
	const FSlateBrush* Field(bool bChosen);
	/** A button filled amber (chosen) or only outlined. */
	const FSlateBrush* Button(bool bChosen);
	/** A thin line between parts of a panel. */
	const FSlateBrush* Line();
	/** A white box with rounded corners to tint (gauges, dots). */
	const FSlateBrush* Rounded();
	/** A dark veil over the whole screen. */
	const FSlateBrush* Dim();

	/** The pictures of UghStoneArt as brushes (their textures are kept by the HUD). */
	struct FPictures
	{
		FSlateBrush Logo, Tablet, Copter, Bone, Fade;
	};
	/** Makes the pictures' textures (into `Textures`, owned by `Outer`) and their brushes (tens of milliseconds). */
	TSharedRef<FPictures> MakePictures(UObject* Outer, TArray<TObjectPtr<UTexture2D>>& Textures);

	/** A key cap with `Key` and what it does beside it. */
	TSharedRef<SWidget> KeyHint(const FString& Key, const TAttribute<FText>& What);
}
