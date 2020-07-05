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
}

namespace smol 
{
	class TextureManager;
	struct TextureHandle;
	class MeshManager;
	class Renderer;
	class Renderer2D;
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
	void DrawCube(glm::vec3 pos, glm::vec3 size, glm::vec4 colour, const struct smol::TextureHandle& th);
private:
	void GenerateCubeMesh();

	struct RenderMesh;
	std::vector<RenderMesh> CreateRenderMeshesFromModel(const class Model& m);
	std::vector<Graphics::RenderMesh> m_testMesh;
	std::vector<Graphics::RenderMesh> m_testMesh2;

	std::unique_ptr<SDE::DebugCameraController> m_debugCameraController;
	std::unique_ptr<smol::Renderer2D> m_render2d;
	std::unique_ptr<smol::Renderer> m_render3d;
	std::unique_ptr<smol::TextureManager> m_textures;
	std::unique_ptr<smol::MeshManager> m_meshes;
	SDE::ScriptSystem* m_scriptSystem = nullptr;
	SDE::RenderSystem* m_renderSystem = nullptr;
	Input::InputSystem* m_inputSystem = nullptr;
};