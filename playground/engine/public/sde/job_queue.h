/*
SDLEngine
Matt Hoyle
*/
#pragma once
#include "job.h"
#include "kernel/mutex.h"
#include <queue>

namespace SDE
{
	class JobQueue
	{
	public:
		JobQueue();
		~JobQueue();

		void PushJob(Job&& j);
		bool PopJob(Job &j);
		void RemoveAll();

	private:
		Kernel::Mutex m_lock;
		std::queue<Job> m_currentJobs;
	};
}