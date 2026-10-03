// The golden replays as automation tests of the editor (Ugh.Replays.<replay>): each one played on the logic and
// compared field by field by 6_verification's ReplayCheck, the same check as 6_verification\build.ps1.
#if UGH_REPLAY_TESTS

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#include <string>
#include <vector>

#include "check/ReplayCheck.hpp"
#include "check/ReplayReport.hpp"
#include "data/ugd/DataFileReader.hpp"
#include "keyboard/KeyFile.hpp"

namespace
{
	/** The repository: the project is 5_remake/game. */
	FString RepositoryDir() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../..")); }
	FString DataPath() { return RepositoryDir() / TEXT("assets/logic/ugh-data.ugd"); }
	FString ReplayDir() { return RepositoryDir() / TEXT("4_test_data/verify/build/replays"); }

	const TCHAR* NoReplays = TEXT("NoReplays");
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FUghReplayTest, "Ugh.Replays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FUghReplayTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(ReplayDir() / TEXT("*.ugr")), true, false);
	Files.Sort();
	for (const FString& File : Files)
	{
		OutBeautifiedNames.Add(FPaths::GetBaseFilename(File));
		OutTestCommands.Add(ReplayDir() / File);
	}
	if (Files.IsEmpty())
	{
		OutBeautifiedNames.Add(NoReplays);   // a test that fails, so a missing folder is not a pass
		OutTestCommands.Add(NoReplays);
	}
}

bool FUghReplayTest::RunTest(const FString& Replay)
{
	if (Replay == NoReplays)
	{
		AddError(FString::Printf(TEXT("no golden replays in %s (.\\gradlew.bat :verify:replays)"), *ReplayDir()));
		return false;
	}
	const std::string Data = TCHAR_TO_UTF8(*DataPath());
	std::string Error;
	const auto GameData = ugh::data::ugd::DataFileReader::read(Data, Error);
	std::vector<ugh::keyboard::KeyBinding> Keys;
	if (!GameData || !ugh::keyboard::KeyFile::read(Data, Keys, Error))
	{
		AddError(UTF8_TO_TCHAR(Error.c_str()));
		return false;
	}
	const ugh::check::ReplayCheck::Options Options;
	ugh::check::ReplayReport Report;
	const std::string Path = TCHAR_TO_UTF8(*Replay);
	ugh::check::ReplayCheck(*GameData, Keys, Options, Report).run(Path);
	const FString Text = UTF8_TO_TCHAR(Report.text(Path).c_str());
	if (Report.passed())
	{
		AddInfo(Text);
	}
	else
	{
		AddError(Text);
	}
	return Report.passed();
}

#endif
