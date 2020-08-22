/*
SDLEngine
Matt Hoyle
*/
#include "job_system.h"
#include "core/profiler.h"
#include "kernel/platform.h"
#include "core/system_enumerator.h"
#include "render_system.h"
#include "render/device.h"

namespace SDE
{
	JobSystem::JobSystem()
		: m_threadCount(8)
		, m_jobThreadTrigger(0)
		, m_jobThreadStopRequested(0)
	{
		int cpuCount = Kernel::Platform::CPUCount();
		if (cpuCount > 1)
		{
			m_threadCount = cpuCount - 1;
		}
	}

	JobSystem::~JobSystem()
	{
	}

	bool JobSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
	{
		m_renderSystem = (RenderSystem*)systemEnumerator.GetSystem("Render");
		return true;
	}

	bool JobSystem::PostInit()
	{
		// Create shared GL contexts for each job thread on the main thread
		// This allows us to call *some* gl functions from workers
		auto renderDevice = m_renderSystem->GetDevice();
		std::vector<void*> workerContexts;
		for (int w = 0; w < m_threadCount; ++w)
		{
			workerContexts.push_back(renderDevice->CreateSharedGLContext());
		}
		// Creating a context sets it by default, so make sure we reset the main thread context
		renderDevice->SetGLContext(renderDevice->GetGLContext());
		auto jobInit = [this, workerContexts](uint32_t threadIndex)
		{
			m_renderSystem->GetDevice()->SetGLContext(workerContexts[threadIndex]);
		};

		auto jobThread = [this](uint32_t threadIndex)
		{
			if (m_jobThreadStopRequested.Get() == 0)	// This is to stop deadlock on the semaphore when shutting down
			{
				m_jobThreadTrigger.Wait();		// Wait for jobs

				Job currentJob;
				if (m_pendingJobs.PopJob(currentJob))
				{
					SDE_PROF_EVENT("RunJob");
					currentJob.Run();
				}
				else
				{
					// If no job was handled (due to scheduling), re-trigger threads
					m_jobThreadTrigger.Post();
				}
			}
		};
		m_threadPool.Start("SDEJobSystem", m_threadCount, jobThread, jobInit);
		return true;
	}

	void JobSystem::Shutdown()
	{
		SDE_PROF_EVENT();

		// Clear out pending jobs, we do not flush under any circumstances!
		m_pendingJobs.RemoveAll();

		// At this point, jobs may still be running, or the threads may be waiting
		// on the trigger. In order to ensure the jobs finish, we set the quitting flag, 
		// then post the semaphore for each thread, and stop the thread pool
		// This should ensure we don't deadlock on shutdown
		m_jobThreadStopRequested.Set(1);

		for (int32_t t = 0; t < m_threadCount; ++t)
		{
			m_jobThreadTrigger.Post();
		}

		// Stop the threadpool, no more jobs will be taken after this
		m_threadPool.Stop();
	}

	void JobSystem::PushJob(Job::JobThreadFunction threadFn)
	{
		SDE_PROF_EVENT();

		Job jobDesc(this, threadFn);
		m_pendingJobs.PushJob(std::move(jobDesc));
		m_jobThreadTrigger.Post();		// Trigger threads
	}
}