/*
SDLEngine
Matt Hoyle
*/
#include "job_queue.h"
#include "core/scoped_mutex.h"

namespace SDE
{
	JobQueue::JobQueue()
	{
	}

	JobQueue::~JobQueue()
	{
	}

	void JobQueue::PushJob(Job&& j)
	{
		Core::ScopedMutex lock(m_lock);
		m_currentJobs.push(std::move(j));
	}

	bool JobQueue::PopJob(Job &j)
	{
		Core::ScopedMutex lock(m_lock);
		if (!m_currentJobs.empty())
		{
			j = std::move(m_currentJobs.front());
			m_currentJobs.pop();
			return true;
		}

		return false;
	}

	void JobQueue::RemoveAll()
	{
		Core::ScopedMutex lock(m_lock);
		while (!m_currentJobs.empty())
		{
			m_currentJobs.pop();
		}
	}
}