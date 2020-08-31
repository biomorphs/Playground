/*
SDLEngine
Matt Hoyle
*/
#pragma once
#include "math/glm_headers.h"
#include "camera_controller.h"

namespace Input
{
	struct ControllerRawState;
	struct MouseRawState;
	struct KeyboardState;
}

namespace SDE
{
	class DebugCameraController : public CameraController
	{
	public:
		DebugCameraController();
		virtual ~DebugCameraController();
		virtual void ApplyToCamera(Render::Camera& target);
		void Update(const Input::ControllerRawState& controllerState, double timeDelta);
		void Update(const Input::MouseRawState& mouseState, double timeDelta);
		void Update(const Input::KeyboardState& kbState, double timeDelta);
		inline void SetPosition(const glm::vec3& pos) { m_position = pos; }
		inline void SetYaw(float y) { m_yaw = y; }
		inline void SetPitch(float p) { m_pitch = p; }
		inline glm::vec3 GetPosition() const { return m_position; }

	private:
		glm::vec3 m_position;
		glm::vec3 m_lookDirection;
		glm::vec3 m_right;
		float m_pitch;
		float m_yaw;
	};
}