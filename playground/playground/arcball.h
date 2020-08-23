#pragma once
#include "math/glm_headers.h"

namespace Input
{
	struct MouseRawState;
}

class Arcball
{
public:
	Arcball(glm::ivec2 windowSize, glm::vec3 pos, glm::vec3 target, glm::vec3 up);
	void Update(const Input::MouseRawState&, float dt);
	glm::vec3 GetPosition() { return m_position; }
	glm::vec3 GetTarget() { return m_target; }
	glm::vec3 GetUp() { return m_upVector; }
private:
	bool m_enabled = false;
	glm::ivec2 m_anchorPoint;	// first 'click' point
	glm::ivec2 m_currentPoint;
	glm::ivec2 m_windowSize;
	glm::vec3 m_position;
	glm::vec3 m_target;
	glm::vec3 m_upVector;
};