/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "controller_state.h"
#include "mouse_state.h"
#include "keyboard_state.h"
#include "core/system.h"
#include <vector>

namespace Input
{
	class InputSystem : public Core::ISystem
	{
	public:
		InputSystem();
		virtual ~InputSystem();
		virtual bool PreInit(Core::ISystemEnumerator& systemEnumerator);
		virtual bool Initialise();
		virtual bool Tick();

		inline uint32_t ControllerCount() const { return (uint32_t)m_controllers.size(); }
		const ControllerRawState ControllerState(uint32_t padIndex) const;
		const MouseRawState& GetMouseState() const { return m_mouseState; }
		const KeyboardState& GetKeyboardState() const { return m_keysState;	}

	private:
		void UpdateControllerState();
		void EnumerateControllers();
		float ApplyDeadZone(float input, float deadZone) const;
		void UpdateMouseState();
		void OnSystemEvent(void*);

		struct ControllerDesc
		{
			void* m_sdlController;
			ControllerRawState m_padState;
		};
		std::vector<ControllerDesc> m_controllers;
		float m_controllerAxisDeadZone;

		int32_t m_currentMouseScroll = 0;
		MouseRawState m_mouseState;

		KeyboardState m_keysState;
	};
}