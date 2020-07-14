/*
SDLEngine
	Matt Hoyle
*/
#pragma once

namespace Kernel
{
	namespace Platform
	{
		enum class InitResult
		{
			InitOK,
			InitFailed,
		};

		enum class ShutdownResult
		{
			ShutdownOK
		};

		int CPUCount();
		InitResult Initialise(int argc, char* argv[]);
		ShutdownResult Shutdown();
	}
}
