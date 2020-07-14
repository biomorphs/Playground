#pragma once
#include "core/system.h"
#include "core/timer.h"
#include "math/glm_headers.h"
#include <vector>
#include <string>
#include <memory>

namespace Input
{
	class InputSystem;
}

namespace SDE
{
	class ScriptSystem;
	class RenderSystem;
	class DebugCameraController;
	class JobSystem;
}

namespace smol 
{
	class TextureManager;
	class ModelManager;
	class ShaderManager;
	class Renderer;
	class Renderer2D;
	class DebugRender;
}

namespace DebugGui
{
	class DebugGuiSystem;
}

class Graphics : public Core::ISystem
{
public:
	Graphics();
	virtual ~Graphics();
	virtual bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	virtual bool PostInit();
	virtual bool Tick();
	virtual void Shutdown();
private:
	std::unique_ptr<smol::DebugRender> m_debugRender;
	std::unique_ptr<SDE::DebugCameraController> m_debugCameraController;
	std::unique_ptr<smol::Renderer2D> m_render2d;
	std::unique_ptr<smol::Renderer> m_render3d;
	std::unique_ptr<smol::TextureManager> m_textures;
	std::unique_ptr<smol::ModelManager> m_models;
	std::unique_ptr<smol::ShaderManager> m_shaders;
	DebugGui::DebugGuiSystem* m_debugGui = nullptr;
	SDE::ScriptSystem* m_scriptSystem = nullptr;
	SDE::RenderSystem* m_renderSystem = nullptr;
	SDE::JobSystem* m_jobSystem = nullptr;
	Input::InputSystem* m_inputSystem = nullptr;
};