#include "UghUiStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "UghStoneArt.h"
#include "UghTexture.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

const FLinearColor UghUiStyle::Bone = FLinearColor::FromSRGBColor(FColor(240, 230, 208));
const FLinearColor UghUiStyle::Muted = FLinearColor::FromSRGBColor(FColor(190, 176, 152));
const FLinearColor UghUiStyle::Amber = FLinearColor::FromSRGBColor(FColor(246, 180, 64));
const FLinearColor UghUiStyle::Ember = FLinearColor::FromSRGBColor(FColor(232, 92, 52));
const FLinearColor UghUiStyle::Leaf = FLinearColor::FromSRGBColor(FColor(150, 204, 84));
const FLinearColor UghUiStyle::Ink = FLinearColor::FromSRGBColor(FColor(38, 30, 22));
const FLinearColor UghUiStyle::Shadow = FLinearColor(0, 0, 0, 0.6f);
const FLinearColor UghUiStyle::Glass = FLinearColor(0.012f, 0.009f, 0.006f, 0.72f);
const FLinearColor UghUiStyle::Edge = FLinearColor(1.f, 0.85f, 0.6f, 0.16f);

namespace
{
	/** The fonts: Lilita One, Alegreya Sans regular and bold. */
	TSharedPtr<const FCompositeFont> DisplayFont, TextFont, BoldFont;

	FSlateBrush MakeBrush(UTexture2D* Texture, const FVector2D& Size)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}
}

void UghUiStyle::FindFonts(const FString& AssetsDir)
{
	const FString Folder = AssetsDir / TEXT("3d/googlefonts");
	auto Find = [&](const TCHAR* File, const TCHAR* EngineFont) -> TSharedPtr<const FCompositeFont>
	{
		FString Path = Folder / File;
		if (!FPaths::FileExists(Path))
		{
			UE_LOG(LogTemp, Warning, TEXT("UGH no font %s (fetch-assets.ps1): the engine's %s instead"), *Path, EngineFont);
			Path = FPaths::EngineContentDir() / TEXT("Slate/Fonts") / EngineFont;
		}
		return MakeShared<FStandaloneCompositeFont>(NAME_None, Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
	};
	DisplayFont = Find(TEXT("lilitaone/LilitaOne-Regular.ttf"), TEXT("Roboto-Black.ttf"));
	TextFont = Find(TEXT("alegreyasans/AlegreyaSans-Regular.ttf"), TEXT("Roboto-Regular.ttf"));
	BoldFont = Find(TEXT("alegreyasans/AlegreyaSans-Bold.ttf"), TEXT("Roboto-Bold.ttf"));
}

FSlateFontInfo UghUiStyle::Display(float Size, float Outline)
{
	FSlateFontInfo Font(DisplayFont, Size);
	Font.OutlineSettings = FFontOutlineSettings(FMath::RoundToInt(Outline), FLinearColor(0.02f, 0.015f, 0.01f, 0.85f));
	return Font;
}

FSlateFontInfo UghUiStyle::Text(float Size, bool bBold)
{
	return FSlateFontInfo(bBold ? BoldFont : TextFont, Size);
}

FTextBlockStyle UghUiStyle::TextStyle(const FSlateFontInfo& Font, const FLinearColor& Color)
{
	return FTextBlockStyle().SetFont(Font).SetColorAndOpacity(Color).SetShadowOffset(FVector2D(0, 2))
		.SetShadowColorAndOpacity(Shadow);
}

const FSlateBrush* UghUiStyle::Panel()
{
	static const FSlateRoundedBoxBrush Brush(Glass, 18.f, Edge, 1.5f);
	return &Brush;
}

const FSlateBrush* UghUiStyle::Chosen()
{
	static const FSlateRoundedBoxBrush Brush(Amber.CopyWithNewOpacity(0.16f), 12.f, Amber.CopyWithNewOpacity(0.7f), 1.5f);
	return &Brush;
}

const FSlateBrush* UghUiStyle::KeyCap()
{
	static const FSlateRoundedBoxBrush Brush(FLinearColor(1, 0.9f, 0.75f, 0.08f), 6.f, Bone.CopyWithNewOpacity(0.45f), 1.2f);
	return &Brush;
}

const FSlateBrush* UghUiStyle::Field(bool bChosen)
{
	static const FSlateRoundedBoxBrush Plain(FLinearColor(0, 0, 0, 0.45f), 8.f, Edge, 1.2f);
	static const FSlateRoundedBoxBrush Lit(FLinearColor(0, 0, 0, 0.55f), 8.f, Amber, 1.5f);
	return bChosen ? &Lit : &Plain;
}

const FSlateBrush* UghUiStyle::Button(bool bChosen)
{
	static const FSlateRoundedBoxBrush Plain(FLinearColor(1, 0.9f, 0.75f, 0.05f), 12.f, Bone.CopyWithNewOpacity(0.35f), 1.5f);
	static const FSlateRoundedBoxBrush Lit(Amber, 12.f, FLinearColor::FromSRGBColor(FColor(255, 214, 130)), 1.5f);
	return bChosen ? &Lit : &Plain;
}

const FSlateBrush* UghUiStyle::Line()
{
	static const FSlateColorBrush Brush(Edge);
	return &Brush;
}

const FSlateBrush* UghUiStyle::Rounded()
{
	static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 5.f);
	return &Brush;
}

const FSlateBrush* UghUiStyle::Dim()
{
	static const FSlateColorBrush Brush(FLinearColor(0, 0, 0, 0.5f));
	return &Brush;
}

TSharedRef<UghUiStyle::FPictures> UghUiStyle::MakePictures(UObject* Outer, TArray<TObjectPtr<UTexture2D>>& Textures)
{
	const double Started = FPlatformTime::Seconds();
	auto Make = [&](int32 Width, int32 Height, const TArray<FColor>& Pixels, const FVector2D& Size)
	{
		UTexture2D* Texture = UghTexture::Create(Outer, Width, Height, Pixels, false);
		Textures.Add(Texture);
		return MakeBrush(Texture, Size);
	};
	TSharedRef<FPictures> Pictures = MakeShared<FPictures>();
	Pictures->Logo = Make(1100, 420, UghStoneArt::Logo(1100, 420), FVector2D(680, 260));
	Pictures->Tablet = Make(1000, 330, UghStoneArt::Tablet(1000, 330), FVector2D(760, 250));
	Pictures->Copter = Make(96, 72, UghStoneArt::Copter(96, 72), FVector2D(44, 33));
	Pictures->Bone = Make(640, 80, UghStoneArt::Bone(640, 80), FVector2D(336, 42));
	Pictures->Fade = Make(256, 1, UghStoneArt::Fade(256, 0.82f), FVector2D(256, 1));
	UE_LOG(LogTemp, Display, TEXT("UGH screen pictures in %.0f ms"), (FPlatformTime::Seconds() - Started) * 1000);
	return Pictures;
}

TSharedRef<SWidget> UghUiStyle::KeyHint(const FString& Key, const TAttribute<FText>& What)
{
	return LiveKeyHint(FText::FromString(Key), What);
}

TSharedRef<SWidget> UghUiStyle::LiveKeyHint(const TAttribute<FText>& Key, const TAttribute<FText>& What)
{
	static const FTextBlockStyle KeyStyle = TextStyle(Text(14, true), Bone);
	static const FTextBlockStyle WhatStyle = TextStyle(Text(15), Muted);
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBorder).BorderImage(KeyCap()).Padding(FMargin(8, 2))
			[
				SNew(STextBlock).TextStyle(&KeyStyle).Text(Key)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7, 0, 20, 0)
		[
			SNew(STextBlock).TextStyle(&WhatStyle).Text(What)
		];
}
