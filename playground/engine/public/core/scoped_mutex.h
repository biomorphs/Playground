#pragma once
#include "kernel/mutex.h"

namespace Core
{
	class ScopedMutex
	{
	public:
		ScopedMutex(Kernel::Mutex& target);
		~ScopedMutex();
	private:
		Kernel::Mutex& m_mutex;
	};
}