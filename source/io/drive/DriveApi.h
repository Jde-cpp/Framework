#pragma once

namespace Jde::IO
{
	struct IDrive;
	JDE_NATIVE_VISIBILITY sp<IDrive> LoadNativeDrive();/*Implemented in NativeLib.a*/
}
extern "C"
{
	Jde::IO::IDrive* LoadDrive(); 
}

