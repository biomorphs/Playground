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
#include "render/window.h"
#include "math/glm_headers.h"
#include "model_asset.h"
#include <sol.hpp>
#include "smol/texture_manager.h"
#include "smol/mesh_manager.h"
#include "smol/renderer.h"
#include "smol/renderer_2d.h"

struct Graphics::RenderMesh
{
	smol::MeshHandle m_mesh;
	smol::TextureHandle m_texture;
	glm::mat4 m_transform;
};

Graphics::Graphics()
{
}

Graphics::~Graphics()
{
}

void Graphics::DrawCube(glm::vec3 pos, glm::vec3 size, glm::vec4 colour, const struct smol::TextureHandle& th)
{
	// cube is always mesh 0
	glm::mat4 cubeTransform = glm::translate(glm::identity<glm::mat4>(), pos);
	cubeTransform = glm::scale(cubeTransform, size);
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
	// Create managers
	m_textures = std::make_unique<smol::TextureManager>();
	m_meshes = std::make_unique<smol::MeshManager>();

	// load white texture in slot 0
	m_textures->LoadTexture("white.bmp");

	// load cube mesh into slot 0
	GenerateCubeMesh();
	
	auto loadedModel = Model::Loader::Load("container.fbx");
	if (loadedModel != nullptr)
	{
		m_testMesh = CreateRenderMeshesFromModel(*loadedModel);
	}

	auto otherModel = Model::Loader::Load("cottage_blender.fbx");
	if (otherModel != nullptr)
	{
		m_testMesh2 = CreateRenderMeshesFromModel(*otherModel);
	}

	const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
	auto windowSize = glm::vec2(windowProps.m_sizeX, windowProps.m_sizeY);

	// Init render passes
	m_render2d = std::make_unique<smol::Renderer2D>(m_textures.get(), windowSize);
	m_renderSystem->AddPass(*m_render2d);
	
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
		m_render2d->SubmitQuad({ px,py }, { sx,sy }, { r,g,b,a }, { 0 });
	};
	graphics["DrawTexturedQuad"] = [this](float px, float py, float sx, float sy, float r, float g, float b, float a, smol::TextureHandle h) {
		m_render2d->SubmitQuad({ px,py }, { sx,sy }, { r,g,b,a }, h);
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

	for (const auto& mh : m_testMesh2)
	{
		glm::mat4 modelMat = mh.m_transform;
		m_render3d->SubmitInstance(modelMat, glm::vec4(1.0f), mh.m_mesh, mh.m_texture);
	}

	const float tm_distance = 4.0f;
	const int maxMeshes = 8;
	glm::vec3 translate(-((maxMeshes * tm_distance) / 2.0f),0.0f,20.0f);
	for (int i = 0; i < 8; ++i)
	{
		for (const auto& mh : m_testMesh)
		{
			glm::mat4 modelMat = mh.m_transform;
			auto newTransform = glm::translate(glm::identity<glm::mat4>(), translate);
			m_render3d->SubmitInstance(newTransform * modelMat, glm::vec4(1.0f), mh.m_mesh, mh.m_texture);
		}
		translate.x += tm_distance;
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

		char meshName[1024] = "";
		sprintf_s(meshName, "%s_%d", m.GetPath().c_str(), meshIndex++);
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