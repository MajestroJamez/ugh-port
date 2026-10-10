#include "UghCameraLog.h"

#include "ContentStreaming.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

void FUghCameraLog::Configure()
{
	FString File;
	if (FParse::Value(FCommandLine::Get(), TEXT("-UghCameraLog="), File))
	{
		Path = FPaths::ConvertRelativePathToFull(File);
		FParse::Value(FCommandLine::Get(), TEXT("-UghCameraLogFlight="), WantedFlight);
		Collected = FCoreUObjectDelegates::GetPostGarbageCollect().AddLambda([this] { Note(TEXT("gc")); });
	}
}

void FUghCameraLog::Record(double Seconds, const FUghCameraPose& Pose, int32 Phase, bool bFlying, double FlightTime,
	double Mist)
{
	if (!IsOn() || bWritten)
	{
		return;
	}
	if (!bRecording)
	{
		// (the flight wanted: -UghCameraLogFlight=<n>, the n-th one, else the first)
		Flights += bFlying && !bWasFlying ? 1 : 0;
		bWasFlying = bFlying;
		if (!bFlying || Flights != WantedFlight)
		{
			return;
		}
		bRecording = true;
		Lines.Add(TEXT("frame,clock,dt,wall,phase,flying,flight,x,y,z,pitch,yaw,roll,fov,exposure,blur,speed,flightspeed,streaming,mist,notes"));
	}
#if WITH_EDITOR
	// shaders still compiling (a hitch when the renderer waits for one)
	if (GShaderCompilingManager && GShaderCompilingManager->GetNumRemainingJobs() > 0)
	{
		Note(FString::Printf(TEXT("shaders %d"), GShaderCompilingManager->GetNumRemainingJobs()));
	}
#endif
	Clock += Seconds;
	const double Now = FPlatformTime::Seconds();
	const double Wall = LastWall > 0 ? Now - LastWall : Seconds;
	LastWall = Now;
	double Speed = 0, FlightSpeed = 0;
	if (Last)
	{
		const double Moved = FVector::Dist(Pose.Location, Last->Location);
		Speed = Seconds > 0 ? Moved / Seconds : 0;
		const double FlightStep = FlightTime - LastFlightTime;
		FlightSpeed = FlightStep > 0 ? Moved / FlightStep : 0;
	}
	Lines.Add(FString::Printf(TEXT("%d,%.4f,%.5f,%.5f,%d,%d,%.7f,%.3f,%.3f,%.3f,%.5f,%.5f,%.5f,%.5f,%.4f,%.3f,%.1f,%.1f,%d,%.3f,%s"),
		Frame++, Clock, Seconds, Wall, Phase, bFlying ? 1 : 0, FlightTime, Pose.Location.X, Pose.Location.Y,
		Pose.Location.Z, Pose.Rotation.Pitch, Pose.Rotation.Yaw, Pose.Rotation.Roll, Pose.FieldOfView, Pose.ExposureBias,
		Pose.MotionBlur, Speed, FlightSpeed, IStreamingManager::Get().GetNumWantingResources(), Mist, *Notes));
	Notes.Reset();
	Last = Pose;
	LastFlightTime = FlightTime;
	Since = bFlying ? 0 : Since + Seconds;
	if (Since > AfterSeconds)
	{
		Flush();
	}
}

void FUghCameraLog::Note(const FString& What)
{
	if (IsOn())
	{
		Notes += (Notes.IsEmpty() ? TEXT("") : TEXT(" ")) + What;
	}
}

void FUghCameraLog::Flush()
{
	if (!IsOn() || bWritten || Lines.IsEmpty())
	{
		return;
	}
	bWritten = true;
	FCoreUObjectDelegates::GetPostGarbageCollect().Remove(Collected);
	FFileHelper::SaveStringArrayToFile(Lines, *Path);
	UE_LOG(LogTemp, Display, TEXT("UGH camera log: %d frames in %s"), Frame, *Path);
}
