#include "graphics.h"
#include "kernel/log.h"
#include "core/system_enumerator.h"
#include "debug_gui/debug_gui_system.h"
#include "render/render_pass.h"
#include "sde/script_system.h"
#include "sde/render_system.h"
#include "sde/debug_camera_controller.h"
#include "input/input_system.h"
#include "render/mesh_builder.h"
#include "render/mesh.h"
#include "render/material.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "render/window.h"
#include "render/mesh_instance_render_pass.h"
#include "math/glm_headers.h"
#include "model_asset.h"
#include <sol.hpp>
#include "smol/texture_manager.h"
#include "smol/mesh_manager.h"
#include "smol/renderer.h"

struct Graphics::Quad
{
	glm::vec2 m_position;
	glm::vec2 m_size;
	glm::vec4 m_colour;
	smol::TextureHandle m_texture;
};

struct Graphics::RenderMesh
{
	smol::MeshHandle m_mesh;
	smol::TextureHandle m_texture;
	glm::mat4 m_transform;
};

const uint64_t c_maxQuads = 1024 * 128;

class Graphics::RenderPass2D : public Render::RenderPass
{
public:
	RenderPass2D(SDE::RenderSystem* rs, smol::TextureManager* ta, std::vector<Quad>& quads)
		: m_renderSystem(rs)
		, m_textures(ta)
		, m_quads(quads)
	{ 
	}
	virtual ~RenderPass2D() = default;

	void Reset() { m_quads.clear(); }
	void RenderAll(Render::Device&);
private:
	void BuildQuadMesh();
	void SetupQuadMaterial();
	void PopulateInstanceBuffers();
	std::vector<Quad>& m_quads;
	std::unique_ptr<Render::Mesh> m_quadMesh;
	std::unique_ptr<Render::Material> m_quadMaterial;
	std::unique_ptr<Render::ShaderProgram> m_quadShaders;
	SDE::RenderSystem* m_renderSystem;
	smol::TextureManager* m_textures;
	Render::RenderBuffer m_quadInstanceTransforms;
	Render::RenderBuffer m_quadInstanceColours;
};

Graphics::Graphics()
{
	m_quads.reserve(c_maxQuads);
}

Graphics::~Graphics()
{
}

void Graphics::DrawQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 colour, const smol::TextureHandle& th)
{
	m_quads.push_back({ pos, size, colour, th });
}

void Graphics::DrawCube(glm::vec3 pos, glm::vec3 size, glm::vec4 colour, const struct smol::TextureHandle& th)
{
	// cube is always mesh 0
	glm::mat4 cubeTransform;
	glm::translate(cubeTransform, pos);
	glm::scale(cubeTransform, size);
	m_render3d->SubmitInstance(cubeTransform, colour, { 0 }, th);
}

bool Graphics::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_scriptSystem = (SDE::ScriptSystem*)systemEnumerator.GetSystem("Script");
	m_renderSystem = (SDE::RenderSystem*)systemEnumerator.GetSystem("Render");
	m_inputSystem = (Input::InputSystem*)systemEnumerator.GetSystem("Input");

	return true;
}

bool Graphics::PostInit()
{
	// load white texture in slot 0
	m_textures = std::make_unique<smol::TextureManager>();
	m_textures->LoadTexture("white.bmp");

	// make mesh array
	m_meshes = std::make_unique<smol::MeshManager>();
	GenerateCubeMesh();
	
	auto loadedModel = Model::Loader::Load("container.FBX");
	if (loadedModel != nullptr)
	{
		m_testMesh = CreateRenderMeshesFromModel(*loadedModel);
	}

	m_render2d = std::make_unique<RenderPass2D>(m_renderSystem, m_textures.get(), m_quads);
	m_renderSystem->AddPass(*m_render2d);

	const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
	auto windowSize = glm::vec2(windowProps.m_sizeX, windowProps.m_sizeY);
	m_render3d = std::make_unique<smol::Renderer>(m_textures.get(), m_meshes.get(), windowSize);
	m_renderSystem->AddPass(*m_render3d);

	// expose TextureHandle to lua
	auto texHandleScriptType = m_scriptSystem->Globals().new_usertype<smol::TextureHandle>("TextureHandle",
		sol::constructors<smol::TextureHandle()>()
		);

	// expose Graphics namespace functions
	auto graphics = m_scriptSystem->Globals()["Graphics"].get_or_create<sol::table>();
	graphics["SetClearColour"] = [this](float r, float g, float b) {
		m_renderSystem->SetClearColour(glm::vec4(r, g, b, 1.0f));
	};
	graphics["DrawQuad"] = [this](float px, float py, float sx, float sy, float r, float g, float b, float a) {
		DrawQuad({ px,py }, { sx,sy }, { r,g,b,a }, { 0 });
	};
	graphics["DrawTexturedQuad"] = [this](float px, float py, float sx, float sy, float r, float g, float b, float a, smol::TextureHandle h) {
		DrawQuad({ px,py }, { sx,sy }, { r,g,b,a }, h);
	};
	graphics["DrawCube"] = [this](float px, float py, float pz, float sx, float sy, float sz, float r, float g, float b, float a) {
		DrawCube({ px,py,pz }, { sx,sy,sz }, { r,g,b,a }, { 0 });
	};
	graphics["DrawTexturedCube"] = [this](float px, float py, float pz, float sx, float sy, float sz, float r, float g, float b, float a, smol::TextureHandle h) {
		DrawCube({ px,py,pz }, { sx,sy,sz }, { r,g,b,a }, h);
	};
	graphics["LoadTexture"] = [this](const char* path) -> smol::TextureHandle {
		return m_textures->LoadTexture(path);
	};
	graphics["LookAt"] = [this](float px, float py, float pz, float tx, float ty, float tz) {
		//m_render3d->SetCamera({ px,py,pz }, { tx,ty,tz });
	};

	m_debugCameraController = std::make_unique<SDE::DebugCameraController>();
	m_debugCameraController->SetPosition({0.0f,0.0f,0.0f});

	return true;
}

bool Graphics::Tick()
{
	Render::Camera c;
	m_debugCameraController->Update(m_inputSystem->ControllerState(0), 0.033f);
	m_debugCameraController->ApplyToCamera(c);
	m_render3d->SetCamera(c);

	// draw the test meshes
	for (const auto& mh : m_testMesh)
	{
		m_render3d->SubmitInstance(mh.m_transform, glm::vec4(1.0f), mh.m_mesh, mh.m_texture);
	}

	return true;
}

void Graphics::Shutdown()
{
	m_scriptSystem->Globals()["Graphics"] = nullptr;
	m_render2d = nullptr;
	m_render3d = nullptr;
	m_textures = nullptr;
	m_meshes = nullptr;
}

std::vector<Graphics::RenderMesh> Graphics::CreateRenderMeshesFromModel(const class Model& m)
{
	std::vector<Graphics::RenderMesh> meshHandles;

	int meshIndex = 0;
	meshHandles.reserve(m.Meshes().size());
	for (const auto& mesh : m.Meshes())
	{
		Render::MeshBuilder builder;
		builder.AddVertexStream(3, mesh.Indices().size());		// position
		builder.AddVertexStream(2, mesh.Indices().size());		// uv
		builder.BeginChunk();
		const auto& vertices = mesh.Vertices();
		const auto& indices = mesh.Indices();
		for (uint32_t index = 0; index < indices.size(); index += 3)
		{
			const auto& v0 = vertices[indices[index]];
			const auto& v1 = vertices[indices[index + 1]];
			const auto& v2 = vertices[indices[index + 2]];
			builder.BeginTriangle();
			builder.SetStreamData(0, v0.m_position, v1.m_position, v2.m_position);
			builder.SetStreamData(1, v0.m_texCoord0, v1.m_texCoord0, v2.m_texCoord0);
			builder.EndTriangle();
		}
		builder.EndChunk();

		auto newMesh = new Render::Mesh();
		builder.CreateMesh(*newMesh);

		char meshName[256];
		sprintf_s(meshName, "Test_%d", meshIndex++);
		auto newMeshHandle = m_meshes->AddMesh(meshName, newMesh);
		std::string diffuseTexturePath = mesh.Material().DiffuseMaps().size() > 0 ? mesh.Material().DiffuseMaps()[0] : "white.bmp";
		auto newTextureHandle = m_textures->LoadTexture(diffuseTexturePath.c_str());

		meshHandles.push_back({ newMeshHandle, newTextureHandle, mesh.Transform() });
	}	

	return meshHandles;
}

void Graphics::GenerateCubeMesh()
{
	Render::MeshBuilder builder;
	auto triangle = [&builder](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2)
	{
		builder.BeginTriangle();
		builder.SetStreamData(0, p0, p1, p2);
		builder.SetStreamData(1, uv0, uv1, uv2);
		builder.EndTriangle();
	};

	const float c_l = 0.5f;
	auto* mesh = new Render::Mesh();

	builder.AddVertexStream(3);		// position
	builder.AddVertexStream(2);		// uv
	builder.BeginChunk();

	//front (+z)
	triangle({ -c_l,-c_l, c_l }, { c_l, -c_l, c_l }, { -c_l,c_l,c_l }, { 0.0,0.0 }, { 1.0,0.0 }, { 0.0,1.0 });	// ccw by default
	triangle({ -c_l, c_l , c_l }, { c_l, -c_l, c_l }, { c_l , c_l , c_l }, {0.0, 1.0}, { 1.0,0.0 }, { 1.0,1.0 });

	//back (-z)
	triangle({ c_l, -c_l, -c_l }, { -c_l,c_l, -c_l }, { c_l , c_l , -c_l }, { 0.0,0.0 }, { 1.0,1.0 }, { 0.0,1.0 });
	triangle({ -c_l,-c_l,-c_l }, { -c_l,c_l ,-c_l }, { c_l ,-c_l ,-c_l }, { 1.0,0.0 }, { 1.0,1.0 }, { 0.0,0.0 });

	// left
	triangle({ -c_l,-c_l ,c_l }, { -c_l ,c_l ,c_l }, { -c_l ,-c_l ,-c_l }, { 1.0,0.0 }, { 1.0,1.0 }, { 0.0,0.0 });
	triangle({ -c_l ,-c_l ,-c_l }, { -c_l ,c_l ,c_l }, { -c_l ,c_l ,-c_l }, { 0.0, 0.0}, { 1.0,1.0 }, { 0.0,1.0 });

	// right 
	triangle({ c_l ,-c_l ,c_l }, { c_l,-c_l ,-c_l }, { c_l ,c_l ,c_l }, { 0.0,0.0 }, { 1.0,0.0 }, { 0.0,1.0 });
	triangle({ c_l ,-c_l ,-c_l }, { c_l ,c_l ,-c_l }, { c_l ,c_l ,c_l }, { 1.0,0.0 }, { 1.0,1.0 }, { 0.0,1.0 });

	// top 
	triangle({ -c_l ,c_l ,c_l }, { c_l ,c_l ,c_l }, { -c_l ,c_l ,-c_l }, { 0.0,0.0 }, { 1.0,0.0 }, { 0.0,1.0 });
	triangle({ c_l ,c_l ,c_l }, { c_l ,c_l ,-c_l }, { -c_l ,c_l ,-c_l }, { 1.0,0.0 }, { 1.0,1.0 }, { 0.0,1.0 });

	// bottom
	triangle({ -c_l ,-c_l ,c_l }, { -c_l ,-c_l ,-c_l }, { c_l ,-c_l ,c_l }, { 1.0,0.0 }, { 1.0,1.0 }, { 0.0,0.0 });
	triangle({ -c_l ,-c_l ,-c_l }, { c_l ,-c_l ,-c_l }, { c_l ,-c_l ,c_l }, { 1.0,1.0 }, { 0.0,1.0 }, { 0.0,0.0 });

	builder.EndChunk();
	builder.CreateMesh(*mesh);

	m_meshes->AddMesh("Cube", mesh);
}

void Graphics::RenderPass2D::BuildQuadMesh()
{
	m_quadMesh = std::make_unique<Render::Mesh>();
	Render::MeshBuilder builder;
	builder.AddVertexStream(2);		// position only
	builder.BeginChunk();
	builder.BeginTriangle();
	builder.SetStreamData(0, { 0,0 }, { 1,0 }, { 0,1 });	// ccw by default
	builder.EndTriangle();
	builder.BeginTriangle();
	builder.SetStreamData(0, { 0,1 }, { 1,0 }, { 1,1 });
	builder.EndTriangle();
	builder.EndChunk();
	builder.CreateMesh(*m_quadMesh);

	// set up instance buffers
	m_quadInstanceTransforms.Create(c_maxQuads * sizeof(glm::mat4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic);
	m_quadInstanceColours.Create(c_maxQuads * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic);
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

	m_quadMaterial->SetShaderProgram(m_quadShaders.get());
	m_quadMesh->SetMaterial(m_quadMaterial.get());
}

void Graphics::RenderPass2D::PopulateInstanceBuffers()
{
	//static to avoid constant allocations
	static std::vector<glm::mat4> instanceTransforms;
	instanceTransforms.reserve(c_maxQuads);
	instanceTransforms.clear();
	static std::vector<glm::vec4> instanceColours;
	instanceColours.reserve(c_maxQuads);
	instanceColours.clear();

	for (const auto& q : m_quads)
	{
		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, glm::vec3(q.m_position, 0.0f));
		modelMat = glm::scale(modelMat, glm::vec3(q.m_size, 0.0f));
		instanceTransforms.push_back(modelMat);
		instanceColours.push_back(q.m_colour);
	}

	// copy the instance buffers to gpu
	m_quadInstanceTransforms.SetData(0, instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data());
	m_quadInstanceColours.SetData(0, instanceColours.size() * sizeof(glm::vec4), instanceColours.data());
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

	if (m_quads.size() == 0)
	{
		return;
	}

	// Sort the quads by texture to separate draw calls
	std::sort(m_quads.begin(), m_quads.end(), [](const Quad& q1, const Quad& q2) -> bool {
		return q1.m_texture.m_index < q2.m_texture.m_index;
	});

	PopulateInstanceBuffers();

	// global uniforms
	const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
	auto windowSize = glm::vec2(windowProps.m_sizeX, windowProps.m_sizeY);
	auto projectionMat = glm::ortho(0.0f, windowSize.x, 0.0f, windowSize.y);
	Render::UniformBuffer ub;
	ub.SetValue("ProjectionMat", projectionMat);

	// render state
	d.SetDepthState(true, false);		// enable z-test, disable write
	d.SetBackfaceCulling(true, true);	// backface culling, ccw order
	d.SetBlending(true);				// enable blending (we might want to do it manually instead)
	d.SetScissorEnabled(false);			// (don't) scissor me timbers

	d.BindShaderProgram(*m_quadShaders);

	// bind vertex array
	d.BindVertexArray(m_quadMesh->GetVertexArray());

	// set instance buffers. matrices must be set over multiple stream as part of vao
	d.BindInstanceBuffer(m_quadMesh->GetVertexArray(), m_quadInstanceTransforms, 1, 4, 0, 4);
	d.BindInstanceBuffer(m_quadMesh->GetVertexArray(), m_quadInstanceTransforms, 2, 4, sizeof(float) * 4, 4);
	d.BindInstanceBuffer(m_quadMesh->GetVertexArray(), m_quadInstanceTransforms, 3, 4, sizeof(float) * 8, 4);
	d.BindInstanceBuffer(m_quadMesh->GetVertexArray(), m_quadInstanceTransforms, 4, 4, sizeof(float) * 12, 4);
	d.BindInstanceBuffer(m_quadMesh->GetVertexArray(), m_quadInstanceColours, 5, 4, 0);
	
	// find the first and last quad with the same texture
	auto firstQuad = m_quads.begin();
	while (firstQuad != m_quads.end())
	{
		uint64_t texID = firstQuad->m_texture.m_index;
		auto lastQuad = std::find_if(firstQuad, m_quads.end(), [texID](const Quad& q) -> bool {
			return q.m_texture.m_index != texID;
		});

		// use the texture or our in built white texture
		smol::TextureHandle texture = firstQuad->m_texture.m_index != (uint64_t)-1 ? firstQuad->m_texture : smol::TextureHandle{0};
		ub.SetSampler("MyTexture", m_textures->GetTexture(texture)->GetHandle());
		d.SetUniforms(*m_quadShaders, ub);

		// somehow only draw firstQuad-lastQuad instances
		const auto& meshChunk = m_quadMesh->GetChunks()[0];
		auto instanceCount = (uint32_t)(lastQuad - firstQuad);
		uint32_t firstIndex = (uint32_t)(firstQuad - m_quads.begin());
		d.DrawPrimitivesInstanced(meshChunk.m_primitiveType, meshChunk.m_firstVertex, meshChunk.m_vertexCount, instanceCount, firstIndex);

		firstQuad = lastQuad;
	}
}