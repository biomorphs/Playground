#include "scoped_mutex.h"
#include "core/profiler.h"

namespace Core
{
	ScopedMutex::ScopedMutex(Kernel::Mutex& target)
		: m_mutex(target)
	{
		SDE_PROF_STALL("ScopedMutex:Wait");
		m_mutex.Lock();
	}

	ScopedMutex::~ScopedMutex()
	{
		m_mutex.Unlock();
	}
}