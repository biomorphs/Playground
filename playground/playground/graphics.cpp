#include "graphics.h"
#include "kernel/log.h"
#include "core/system_enumerator.h"
#include "debug_gui/debug_gui_system.h"
#include "render/render_pass.h"
#include "sde/script_system.h"
#include "sde/render_system.h"
#include "render/mesh_builder.h"
#include "render/mesh.h"
#include "render/material.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "render/window.h"
#include "render/mesh_instance_render_pass.h"
#include <sol.hpp>

struct Graphics::Quad
{
	glm::vec2 m_position;
	glm::vec2 m_size;
	glm::vec4 m_colour;
};

class Graphics::RenderPass2D : public Render::RenderPass
{
public:
	RenderPass2D(SDE::RenderSystem* rs) : m_renderSystem(rs) { m_quads.reserve(1024); }
	virtual ~RenderPass2D() = default;
	void SetQuads(const std::vector<Graphics::Quad>& q) { m_quads = q; }

	void Reset() { m_quads.clear(); m_meshInstancePass.Reset(); }
	void RenderAll(Render::Device&);
private:
	void BuildQuadMesh();
	void SetupQuadMaterial();
	std::vector<Quad> m_quads;
	std::unique_ptr<Render::Mesh> m_quadMesh;
	std::unique_ptr<Render::Material> m_quadMaterial;
	std::unique_ptr<Render::ShaderProgram> m_quadShaders;
	SDE::RenderSystem* m_renderSystem;
	Render::MeshInstanceRenderPass m_meshInstancePass;
};

Graphics::Graphics()
{
	m_quads.reserve(1024);
}

Graphics::~Graphics()
{
}

void Graphics::DrawQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 colour)
{
	m_quads.push_back({ pos, size, colour });
}

bool Graphics::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_debugGui = (DebugGui::DebugGuiSystem*)systemEnumerator.GetSystem("DebugGui");
	m_scriptSystem = (SDE::ScriptSystem*)systemEnumerator.GetSystem("Script");
	m_renderSystem = (SDE::RenderSystem*)systemEnumerator.GetSystem("Render");

	return true;
}

bool Graphics::Initialise()
{
	m_scriptSystem->Globals()["Graphics"].get_or_create<sol::table>();
	m_scriptSystem->Globals()["Graphics"]["SetClearColour"] = [this](float r, float g, float b) {
		m_renderSystem->SetClearColour(glm::vec4(r, g, b, 1.0f));
	};
	m_scriptSystem->Globals()["Graphics"]["DrawQuad"] = [this](float px, float py, float sx, float sy, float r, float g, float b, float a) {
		DrawQuad({ px,py }, { sx,sy }, { r,g,b,a });
	};

	return true;
}

bool Graphics::PostInit()
{
	m_renderPass = std::make_unique<RenderPass2D>(m_renderSystem);
	m_renderSystem->AddPass(*m_renderPass);

	return true;
}

bool Graphics::Tick()
{
	m_renderPass->SetQuads(m_quads);
	m_quads.clear();
	return true;
}

void Graphics::Shutdown()
{
	m_renderPass = nullptr;
}

void Graphics::RenderPass2D::BuildQuadMesh()
{
	m_quadMesh = std::make_unique<Render::Mesh>();
	Render::MeshBuilder builder;
	builder.AddVertexStream(2);		// position
	builder.BeginChunk();
	builder.BeginTriangle();
	builder.SetStreamData(0, { 0,0 }, { 1,0 }, { 0,1 });	// ccw by default
	builder.EndTriangle();
	builder.BeginTriangle();
	builder.SetStreamData(0, { 0,1 }, { 1,0 }, { 1,1 });
	builder.EndTriangle();
	builder.EndChunk();
	builder.CreateMesh(*m_quadMesh);
}

void Graphics::RenderPass2D::SetupQuadMaterial()
{
	m_quadMaterial = std::make_unique<Render::Material>();
	m_quadShaders = std::make_unique<Render::ShaderProgram>();

	auto vertexShader = std::make_unique<Render::ShaderBinary>();
	std::string errorText;
	if (!vertexShader->CompileFromFile(Render::ShaderType::VertexShader, "quad.vs", errorText))
	{
		SDE_LOG("Vertex shader compilation failed - %s", errorText.c_str());
	}
	auto fragmentShader = std::make_unique<Render::ShaderBinary>();
	if (!fragmentShader->CompileFromFile(Render::ShaderType::FragmentShader, "quad.fs", errorText))
	{
		SDE_LOG("Fragment shader compilation failed - %s", errorText.c_str());
	}

	if (!m_quadShaders->Create(*vertexShader, *fragmentShader, errorText))
	{
		SDE_LOG("Shader linkage failed - %s", errorText.c_str());
	}
	m_quadShaders->AddUniform("ProjectionMat");
	m_quadShaders->AddUniform("ModelMat");

	m_quadMaterial->SetShaderProgram(m_quadShaders.get());
	m_quadMesh->SetMaterial(m_quadMaterial.get());
}

void Graphics::RenderPass2D::RenderAll(Render::Device& d)
{
	static bool s_firstFrame = true;
	if (s_firstFrame)
	{
		BuildQuadMesh();
		SetupQuadMaterial();
		s_firstFrame = false;
	}

	const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
	auto windowSize = glm::vec2(windowProps.m_sizeX, windowProps.m_sizeY);
	auto projectionMat = glm::ortho(0.0f, windowSize.x, 0.0f, windowSize.y);
	Render::UniformBuffer ub;
	for (const auto& q : m_quads)
	{
		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, glm::vec3(q.m_position, 0.0f));
		modelMat = glm::scale(modelMat, glm::vec3(q.m_size, 0.0f));
		ub.SetValue("ModelMat", modelMat);
		ub.SetValue("ProjectionMat", projectionMat);
		m_meshInstancePass.AddInstance(m_quadMesh.get(), std::move(ub));
	}

	m_meshInstancePass.RenderAll(d);
}