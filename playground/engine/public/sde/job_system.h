/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "job_queue.h"
#include "core/system.h"
#include "core/thread_pool.h"
#include "kernel/semaphore.h"
#include "kernel/atomics.h"

namespace SDE
{
	class JobSystem : public Core::ISystem
	{
	public:
		JobSystem();
		virtual ~JobSystem();

		bool PreInit(Core::ISystemEnumerator& systemEnumerator);
		bool PostInit();
		void Shutdown();

		void PushJob(Job::JobThreadFunction threadFn);

	private:
		class RenderSystem* m_renderSystem;
		Core::ThreadPool m_threadPool;
		JobQueue m_pendingJobs;
		Kernel::Semaphore m_jobThreadTrigger;
		Kernel::AtomicInt32 m_jobThreadStopRequested;
		int32_t m_threadCount;
	};
}