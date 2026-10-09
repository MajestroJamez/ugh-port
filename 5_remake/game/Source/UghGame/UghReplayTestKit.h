// What the tests of the replays and the ghost share (Ugh.Record.*, Ugh.Ghost): the data, a folder of their own, a replay
// written by hand as docs/replay-format.md says, a pilot flying at random.
#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/Crc.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UghControls.h"
#include "UghReplays.h"
#include "UghSimulation.h"

namespace UghReplayTestKit
{
	inline const FString Assets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }
	inline const FString DataFile() { return Assets() / TEXT("logic/ugh-data.ugd"); }

	/** A folder of its own for a test's replays (deleted at its end). */
	inline FString TestFolder()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::AutomationTransientDir() / TEXT("Replays-") +
			FGuid::NewGuid().ToString());
	}

	/** The CRC-32 of zip (as the format's checksum). */
	inline uint32 Crc32(const TArray<uint8>& Bytes)
	{
		uint32 Crc = 0xFFFFFFFFu;
		for (const uint8 Byte : Bytes)
		{
			Crc ^= Byte;
			for (int32 Bit = 0; Bit < 8; ++Bit)
			{
				Crc = Crc & 1 ? 0xEDB88320u ^ (Crc >> 1) : Crc >> 1;
			}
		}
		return ~Crc;
	}

	/** A replay written by hand as docs/replay-format.md says. */
	struct FCraft
	{
		int32 Players = 1, Difficulty = 1, Level = 2;
		uint32 Points = 1000;
		int32 Steps = 3000, PlaySteps = 2000;
		bool bDone = true;
		uint32 DataHash = 0;
		uint64 Date = 1791000000;
		FString Name = TEXT("Jan");

		TArray<uint8> Bytes() const
		{
			TArray<uint8> B;
			auto Number = [&B](uint64 Value)
			{
				for (; Value >= 0x80; Value >>= 7)
				{
					B.Add(uint8(Value | 0x80));
				}
				B.Add(uint8(Value));
			};
			auto Word = [&B](uint32 Value, int32 Bytes) { for (int32 I = 0; I < Bytes; ++I) B.Add(uint8(Value >> (8 * I))); };
			auto Text = [&B, &Number](const FString& Value)
			{
				const FTCHARToUTF8 Utf8(*Value);
				Number(Utf8.Length());
				B.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
			};
			B.Append({ 'U', 'G', 'H', 'R', 1 });
			Number(UGH_LOGIC_VERSION);
			Word(DataHash, 4);
			B.Add(uint8(Players));
			B.Add(uint8(Difficulty));
			Number(Level);
			Number(3);        // lives
			Number(5000);     // the points before
			Number(1);        // multiplier
			for (const uint16 Random : { 0x0003, 0x8134, 0x48bc, 0x2347 })
			{
				Word(Random, 2);
			}
			B.Add(180);       // the rain's row
			for (int32 Player = 0; Player < Players; ++Player)
			{
				Number(0);    // effort 0 (zigzag)
			}
			B.Add(2);         // the last menu key: Other
			Number(Steps);
			Number(PlaySteps);
			Number(1);        // attempts
			Number(Points);
			B.Add(bDone ? 1 : 0);
			Number(Date);
			Text(TEXT("PASSWORD"));
			B.Add(1);
			Text(Name);
			Number(2);        // inputs: Up pressed with Other after it after step 10, released after step 30
			Number((10ull << 6) | (0 * 10 + 0 * 2 + 1 + 20));
			Number((20ull << 6) | (0 * 10 + 0 * 2 + 0));
			Word(Crc32(B), 4);
			return B;
		}
		TSharedPtr<FUghReplay> Replay() const
		{
			FString Error;
			return FUghReplay::Read(Bytes(), Error);
		}
	};

	/** A pilot holding random keys a random while (as the frontend gives them: each key event to the game loop too). */
	struct FRandomPilot
	{
		uint32 State;
		bool Held[2][5] = {};
		void Fly(IUghLogicInput& Logic, int32 Players)
		{
			for (int32 Player = 0; Player < Players; ++Player)
			{
				for (int32 Key = UGH_LOGIC_KEY_UP; Key <= UGH_LOGIC_KEY_FIRE; ++Key)
				{
					State = State * 1664525u + 1013904223u;
					if ((State >> 8) % (Key == UGH_LOGIC_KEY_UP ? 12 : 30) == 0)
					{
						Held[Player][Key] = !Held[Player][Key];
						Logic.Key(Player, Key, Held[Player][Key]);
						Logic.MenuKey(UGH_LOGIC_MENU_OTHER);
					}
				}
			}
		}
	};

	inline uint32 HashOf(const ugh_logic_view& View) { return FCrc::MemCrc32(&View, sizeof View); }

	/** Real time of one step of the logic (and a little more: one step a call, never two). */
	constexpr double StepSeconds = 1.0 / FUghSimulation::TickRate + 1e-9;
}


#endif
