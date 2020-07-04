#pragma once
#include "core/system.h"
#include "core/timer.h"
#include "math/glm_headers.h"
#include <vector>
#include <string>
#include <memory>

namespace DebugGui
{
	class DebugGuiSystem;
}

namespace SDE
{
	class ScriptSystem;
	class RenderSystem;
}

namespace Render
{
	class RenderPass;
	class Mesh;
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
	void DrawQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 colour, const struct TextureHandle& th);
	void DrawCube(glm::vec3 pos, glm::vec3 size, glm::vec4 colour, const struct TextureHandle& th);
private:
	void GenerateCubeMesh();

	struct Quad;
	struct MeshInstance;
	class RenderPass2D;
	class RenderPass3D;
	class TextureArray;
	class MeshArray;
	std::vector<Quad> m_quads;
	std::vector<MeshInstance> m_instances;
	std::unique_ptr<RenderPass2D> m_render2d;
	std::unique_ptr<RenderPass3D> m_render3d;
	std::unique_ptr<TextureArray> m_textures;
	std::unique_ptr<MeshArray> m_meshes;
	DebugGui::DebugGuiSystem* m_debugGui = nullptr;
	SDE::ScriptSystem* m_scriptSystem = nullptr;
	SDE::RenderSystem* m_renderSystem = nullptr;
};