/*
SDLEngine
Matt Hoyle
*/
#include "event_system.h"
#include <algorithm>
#include <SDL.h>

namespace SDE
{
	EventSystem::EventSystem()
	{

	}

	EventSystem::~EventSystem()
	{

	}

	void EventSystem::RegisterEventHandler(EventHandler h)
	{
		m_handlers.push_back(h);
	}

	bool EventSystem::Tick()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			for (auto& it : m_handlers)
			{
				it(&event);
			}
			if (event.type == SDL_QUIT)
			{
				return false;
			}
		}
		return true;
	}
}