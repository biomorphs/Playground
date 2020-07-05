#pragma once
#include "core/system.h"
#include "core/timer.h"
#include "math/glm_headers.h"
#include "smol/mesh_instance.h"
#include <vector>
#include <string>
#include <memory>

namespace DebugGui
{
	class DebugGuiSystem;
}

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

namespace Render
{
	class RenderPass;
	class Mesh;
}

namespace smol 
{
	class TextureManager;
	struct TextureHandle;
	class MeshManager;
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
	void DrawQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 colour, const struct smol::TextureHandle& th);
	void DrawCube(glm::vec3 pos, glm::vec3 size, glm::vec4 colour, const struct smol::TextureHandle& th);
private:
	void GenerateCubeMesh();

	struct RenderMesh;
	std::vector<RenderMesh> CreateRenderMeshesFromModel(const class Model& m);
	std::vector<Graphics::RenderMesh> m_testMesh;

	struct Quad;
	class RenderPass2D;
	class RenderPass3D;
	std::unique_ptr<SDE::DebugCameraController> m_debugCameraController;
	std::vector<Quad> m_quads;
	std::vector<smol::MeshInstance> m_instances;
	std::unique_ptr<RenderPass2D> m_render2d;
	std::unique_ptr<RenderPass3D> m_render3d;
	std::unique_ptr<smol::TextureManager> m_textures;
	std::unique_ptr<smol::MeshManager> m_meshes;
	DebugGui::DebugGuiSystem* m_debugGui = nullptr;
	SDE::ScriptSystem* m_scriptSystem = nullptr;
	SDE::RenderSystem* m_renderSystem = nullptr;
	Input::InputSystem* m_inputSystem = nullptr;
};