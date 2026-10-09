// The menu's screen of the replays.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class FUghReplay;
class FUghReplays;
class IUghClipboard;

/**
 * The menu's screen Replays (FUghReplays: the best of each level and mode, the ones saved after a level, the imported
 * ones): Up and Down choose one, Enter watches it, C copies it to the clipboard as a line of text to share, V imports
 * one from the clipboard (a friend's line of text), Delete twice deletes its file, O opens the folder (a .ughr file put
 * there is listed), Esc goes back. What a key did shows as a notice. The menu does not draw itself (UghUi does).
 */
class FUghReplaysMenu
{
public:
	/** What a key did for the menu: nothing more, back to the title, watch the chosen replay, open the folder. */
	enum class EResult : uint8 { None, Back, Watch, OpenFolder };

	/** The screen opens: the cursor on the first replay, no notice. */
	void Open();
	/** A key of the menu (FUghControls::MenuKeyOf; a gamepad's B as FUghControls::BackKey). */
	EResult HandleKey(const FKey& Key, FUghReplays& Replays, IUghClipboard& Clipboard);

	/** The entry of FUghReplays::GetEntries chosen. */
	int32 GetCursor() const { return Cursor; }
	/** The chosen replay (none: no entries, or a refused one). */
	TSharedPtr<FUghReplay> GetChosen(const FUghReplays& Replays) const;
	/** What the last key did; how many notices there were (a new one shows anew). */
	const FString& GetNotice() const { return Notice; }
	int32 GetNoticeCount() const { return NoticeCount; }
	/** Delete was pressed once: the next Delete deletes. */
	bool IsDeleting() const { return bDeleting; }

private:
	void Say(const FString& Text);

	int32 Cursor = 0;
	FString Notice;
	int32 NoticeCount = 0;
	bool bDeleting = false;
};
