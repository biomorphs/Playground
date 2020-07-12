/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include <functional>
#include <string>

namespace SDE
{
	class JobSystem;

	class Job
	{
	public:
		typedef std::function<void()> JobThreadFunction;	// Code to be ran on the job thread

		Job();
		Job(JobSystem* parent, JobThreadFunction threadFn);
		~Job() = default;
		Job(Job&&) = default;
		Job& operator=(Job&&) = default;
		Job(const Job&) = delete;
		
		void Run();

	private:
		JobThreadFunction m_threadFn;
		JobSystem* m_parent;
		uint64_t m_padding[8];
	};
}