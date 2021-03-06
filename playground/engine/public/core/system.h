/*
SDLEngine
Matt Hoyle
*/
#pragma once
#include "core/profiler.h"

namespace Core
{
	class ISystemEnumerator;

	class ISystem
	{
	public:
		virtual ~ISystem() {}
		virtual bool PreInit(ISystemEnumerator& systemEnumerator) { SDE_PROF_EVENT(); return true; }
		virtual bool Initialise() { SDE_PROF_EVENT(); return true; }
		virtual bool PostInit() { SDE_PROF_EVENT(); return true; }

		virtual bool Tick() { SDE_PROF_EVENT(); return true; }

		virtual void PreShutdown() { SDE_PROF_EVENT(); }
		virtual void Shutdown() { SDE_PROF_EVENT(); }
		virtual void PostShutdown() { SDE_PROF_EVENT(); }
	};
}