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
#include "render/texture.h"
#include "render/texture_source.h"
#include "render/mesh_instance_render_pass.h"
#include "math/glm_headers.h"
#include "stb_image.h"
#include <sol.hpp>

struct TextureHandle
{
	uint64_t m_index;
	static TextureHandle Invalid() { return { (uint64_t)-1 }; };
};

struct MeshHandle
{
	uint64_t m_index;
	static MeshHandle Invalid() { return { (uint64_t)-1 }; };
};

struct Graphics::Quad
{
	glm::vec2 m_position;
	glm::vec2 m_size;
	glm::vec4 m_colour;
	TextureHandle m_texture;
};

struct Graphics::MeshInstance
{
	uint64_t m_sortKey;		// top 32 bits = mesh id, bottom = texture id
	glm::vec3 m_position;
	glm::vec3 m_size;
	glm::vec4 m_colour;
	TextureHandle m_texture;
	MeshHandle m_mesh;
};

const uint64_t c_maxQuads = 1024 * 128;
const uint64_t c_maxInstances = 1024 * 128;

class Graphics::MeshArray
{
public:
	MeshArray() = default;
	~MeshArray();

	MeshHandle AddMesh(const char* name, Render::Mesh* m);
	MeshHandle LoadMesh(const char* name);
	Render::Mesh* GetMesh(const MeshHandle& h);
private:
	struct MeshDesc {
		Render::Mesh* m_mesh;
		std::string m_name;
	};
	std::vector<MeshDesc> m_meshes;
};

class Graphics::TextureArray
{
public:
	TextureArray() = default;
	~TextureArray();

	TextureHandle LoadTexture(const char* path);
	Render::Texture* GetTexture(const TextureHandle& h);
private:
	struct TextureDesc { 
		Render::Texture m_texture;
		std::string m_path;
	};
	std::vector<TextureDesc> m_textures;
};

class Graphics::RenderPass2D : public Render::RenderPass
{
public:
	RenderPass2D(SDE::RenderSystem* rs, Graphics::TextureArray* ta, std::vector<Quad>& quads)
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
	Graphics::TextureArray* m_textures;
	Render::RenderBuffer m_quadInstanceTransforms;
	Render::RenderBuffer m_quadInstanceColours;
};

class Graphics::RenderPass3D : public Render::RenderPass
{
public:
	RenderPass3D(SDE::RenderSystem* rs, Graphics::TextureArray* ta, Graphics::MeshArray* ma, std::vector<MeshInstance>& instances)
		: m_renderSystem(rs)
		, m_textures(ta)
		, m_meshes(ma)
		, m_instances(instances)
	{
	}
	virtual ~RenderPass3D() = default;

	void Reset() { m_instances.clear(); }
	void RenderAll(Render::Device&);
	void SetCamera(glm::vec3 pos, glm::vec3 target) { m_cameraPosition = pos; m_cameraTarget = target; }
private:
	glm::vec3 m_cameraPosition = { 0.0f,200.0f,500.0f };
	glm::vec3 m_cameraTarget = { 0.0f,0.0f,0.0f };
	void SetupShader();
	void PopulateInstanceBuffers();
	std::vector<MeshInstance>& m_instances;
	std::unique_ptr<Render::ShaderProgram> m_shaders;
	SDE::RenderSystem* m_renderSystem;
	Graphics::TextureArray* m_textures;
	Graphics::MeshArray* m_meshes;
	Render::RenderBuffer m_instanceTransforms;
	Render::RenderBuffer m_instanceColours;
	Render::RenderBuffer m_globalsUniformBuffer;
};

struct GlobalUniforms
{
	glm::mat4 m_projectionMat;
	glm::mat4 m_viewMat;
};

Graphics::Graphics()
{
	m_quads.reserve(c_maxQuads);
	m_instances.reserve(c_maxInstances);
}

Graphics::~Graphics()
{
}

void Graphics::DrawQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 colour, const TextureHandle& th)
{
	m_quads.push_back({ pos, size, colour, th });
}

void Graphics::DrawCube(glm::vec3 pos, glm::vec3 size, glm::vec4 colour, const struct TextureHandle& th)
{
	// cube is always mesh 0
	uint64_t meshIndex = 0;
	uint64_t sortKey = (th.m_index & 0x00000000ffffffff) | (meshIndex << 32);
	m_instances.push_back({ sortKey, pos, size, colour, th, {0} });
}

bool Graphics::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_debugGui = (DebugGui::DebugGuiSystem*)systemEnumerator.GetSystem("DebugGui");
	m_scriptSystem = (SDE::ScriptSystem*)systemEnumerator.GetSystem("Script");
	m_renderSystem = (SDE::RenderSystem*)systemEnumerator.GetSystem("Render");

	return true;
}

bool Graphics::PostInit()
{
	// load white texture in slot 0
	m_textures = std::make_unique<TextureArray>();
	m_textures->LoadTexture("white.bmp");

	// make mesh array
	m_meshes = std::make_unique<MeshArray>();
	GenerateCubeMesh();

	m_render2d = std::make_unique<RenderPass2D>(m_renderSystem, m_textures.get(), m_quads);
	m_renderSystem->AddPass(*m_render2d);

	m_render3d = std::make_unique<RenderPass3D>(m_renderSystem, m_textures.get(), m_meshes.get(), m_instances);
	m_renderSystem->AddPass(*m_render3d);

	// expose TextureHandle to lua
	auto texHandleScriptType = m_scriptSystem->Globals().new_usertype<TextureHandle>("TextureHandle",
		sol::constructors<TextureHandle()>()
		);

	// expose Graphics namespace functions
	auto graphics = m_scriptSystem->Globals()["Graphics"].get_or_create<sol::table>();
	graphics["SetClearColour"] = [this](float r, float g, float b) {
		m_renderSystem->SetClearColour(glm::vec4(r, g, b, 1.0f));
	};
	graphics["DrawQuad"] = [this](float px, float py, float sx, float sy, float r, float g, float b, float a) {
		DrawQuad({ px,py }, { sx,sy }, { r,g,b,a }, { 0 });
	};
	graphics["DrawTexturedQuad"] = [this](float px, float py, float sx, float sy, float r, float g, float b, float a, TextureHandle h) {
		DrawQuad({ px,py }, { sx,sy }, { r,g,b,a }, h);
	};
	graphics["DrawCube"] = [this](float px, float py, float pz, float sx, float sy, float sz, float r, float g, float b, float a) {
		DrawCube({ px,py,pz }, { sx,sy,sz }, { r,g,b,a }, { 0 });
	};
	graphics["DrawTexturedCube"] = [this](float px, float py, float pz, float sx, float sy, float sz, float r, float g, float b, float a, TextureHandle h) {
		DrawCube({ px,py,pz }, { sx,sy,sz }, { r,g,b,a }, h);
	};
	graphics["LoadTexture"] = [this](const char* path) -> TextureHandle {
		return m_textures->LoadTexture(path);
	};
	graphics["LookAt"] = [this](float px, float py, float pz, float tx, float ty, float tz) {
		m_render3d->SetCamera({ px,py,pz }, { tx,ty,tz });
	};

	return true;
}

bool Graphics::Tick()
{
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
		TextureHandle texture = firstQuad->m_texture.m_index != (uint64_t)-1 ? firstQuad->m_texture : TextureHandle{0};
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

TextureHandle Graphics::TextureArray::LoadTexture(const char* path)
{
	for (uint64_t i = 0; i < m_textures.size(); ++i)
	{
		if (m_textures[i].m_path == path)
		{
			return { i };
		}
	}

	auto newTex = Render::Texture();
	int w, h, components;
	stbi_set_flip_vertically_on_load(true);	
	unsigned char* loadedData = stbi_load(path, &w, &h, &components, 4);		// we always want rgba
	if (loadedData == nullptr)
	{
		stbi_image_free(loadedData);
		return TextureHandle::Invalid();
	}

	std::vector<uint8_t> rawDataBuffer;
	rawDataBuffer.resize(w * h * 4);
	memcpy(rawDataBuffer.data(), loadedData, w*h * 4);
	stbi_image_free(loadedData);

	std::vector<Render::TextureSource::MipDesc> mip;
	mip.push_back({ (uint32_t)w,(uint32_t)h,0,w*h * (size_t)4 });
	Render::TextureSource ts((uint32_t)w, (uint32_t)h, Render::TextureSource::Format::RGBA8, { mip }, rawDataBuffer);
	std::vector< Render::TextureSource> sources;
	sources.push_back(ts);
	if (!newTex.Create(sources))
	{
		return TextureHandle::Invalid();
	}

	m_textures.push_back({ std::move(newTex), path });
	return { m_textures.size() - 1 };
}

Render::Texture* Graphics::TextureArray::GetTexture(const TextureHandle& h)
{
	if (h.m_index != -1 && h.m_index < m_textures.size())
	{
		return &m_textures[h.m_index].m_texture;
	}
	else
	{
		return nullptr;
	}
}

Graphics::TextureArray::~TextureArray()
{
	for (auto& it : m_textures)
	{
		it.m_texture.Destroy();
	}
}

MeshHandle Graphics::MeshArray::AddMesh(const char* name, Render::Mesh* m)
{
	for (uint64_t i = 0; i < m_meshes.size(); ++i)
	{
		if (m_meshes[i].m_name == name)
		{
			return MeshHandle::Invalid();
		}
	}
	m_meshes.push_back({ m, name });
	return { m_meshes.size() - 1 };
}

MeshHandle Graphics::MeshArray::LoadMesh(const char* name)
{
	for (uint64_t i = 0; i < m_meshes.size(); ++i)
	{
		if (m_meshes[i].m_name == name)
		{
			return { i };
		}
	}

	return MeshHandle::Invalid();
}

Render::Mesh* Graphics::MeshArray::GetMesh(const MeshHandle& h)
{
	if (h.m_index != -1 && h.m_index < m_meshes.size())
	{
		return m_meshes[h.m_index].m_mesh;
	}
	else
	{
		return nullptr;
	}
}

Graphics::MeshArray::~MeshArray()
{
	for (auto& it : m_meshes)
	{
		delete it.m_mesh;
	}
}

void Graphics::RenderPass3D::SetupShader()
{
	m_shaders = std::make_unique<Render::ShaderProgram>();

	auto vertexShader = std::make_unique<Render::ShaderBinary>();
	std::string errorText;
	if (!vertexShader->CompileFromFile(Render::ShaderType::VertexShader, "cube.vs", errorText))
	{
		SDE_LOG("Vertex shader compilation failed - %s", errorText.c_str());
	}
	auto fragmentShader = std::make_unique<Render::ShaderBinary>();
	if (!fragmentShader->CompileFromFile(Render::ShaderType::FragmentShader, "cube.fs", errorText))
	{
		SDE_LOG("Fragment shader compilation failed - %s", errorText.c_str());
	}

	if (!m_shaders->Create(*vertexShader, *fragmentShader, errorText))
	{
		SDE_LOG("Shader linkage failed - %s", errorText.c_str());
	}
}

void Graphics::RenderPass3D::PopulateInstanceBuffers()
{
	//static to avoid constant allocations
	static std::vector<glm::mat4> instanceTransforms;
	instanceTransforms.reserve(c_maxInstances);
	instanceTransforms.clear();
	static std::vector<glm::vec4> instanceColours;
	instanceColours.reserve(c_maxInstances);
	instanceColours.clear();

	for (const auto& c : m_instances)
	{
		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, c.m_position);
		modelMat = glm::scale(modelMat, c.m_size);
		instanceTransforms.push_back(modelMat);
		instanceColours.push_back(c.m_colour);
	}

	// copy the instance buffers to gpu
	m_instanceTransforms.SetData(0, instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data());
	m_instanceColours.SetData(0, instanceColours.size() * sizeof(glm::vec4), instanceColours.data());
}

void Graphics::RenderPass3D::RenderAll(Render::Device& d)
{
	static bool s_firstFrame = true;
	if (s_firstFrame)
	{
		m_instanceTransforms.Create(c_maxInstances * sizeof(glm::mat4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic);
		m_instanceColours.Create(c_maxInstances * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic);
		m_globalsUniformBuffer.Create(sizeof(GlobalUniforms), Render::RenderBufferType::UniformData, Render::RenderBufferModification::Static);

		SetupShader();

		// Bind the globals UBO to index 0
		d.BindUniformBufferIndex(*m_shaders, "Globals", 0);

		s_firstFrame = false;
	}

	if (m_instances.size() == 0)
	{
		return;
	}

	// sort by generic sort key
	std::sort(m_instances.begin(), m_instances.end(), [](const MeshInstance& q1, const MeshInstance& q2) -> bool {
		return q1.m_sortKey < q2.m_sortKey;
	});

	PopulateInstanceBuffers();

	// global uniforms
	Render::UniformBuffer ub;
	const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
	auto windowSize = glm::vec2(windowProps.m_sizeX, windowProps.m_sizeY);

	// render state
	d.SetDepthState(true, true);		// enable z-test, enable write
	d.SetBackfaceCulling(true, true);	// backface culling, ccw order
	d.SetBlending(true);				// enable blending (we might want to do it manually instead)
	d.SetScissorEnabled(false);			// (don't) scissor me timbers
	d.BindShaderProgram(*m_shaders);

	GlobalUniforms globals;
	globals.m_projectionMat = glm::perspectiveFov(glm::radians(90.0f), windowSize.x, windowSize.y, 0.1f, 1000.0f);
	globals.m_viewMat = glm::lookAt(m_cameraPosition, m_cameraTarget, { 0.0f,1.0f,0.0f });
	m_globalsUniformBuffer.SetData(0, sizeof(globals), &globals);

	d.SetUniforms(*m_shaders, m_globalsUniformBuffer, 0);	// do this for every shader change

	auto firstInstance = m_instances.begin();
	while (firstInstance != m_instances.end())
	{
		const Render::Mesh* theMesh = m_meshes->GetMesh(firstInstance->m_mesh);

		// bind vertex array
		d.BindVertexArray(theMesh->GetVertexArray());

		// bind instancing streams immediately after mesh vertex streams
		int instancingSlotIndex = theMesh->GetVertexArray().GetStreamCount();
		// matrices must be set over multiple stream as part of vao
		d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, 0, 4);
		d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, sizeof(float) * 4, 4);
		d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, sizeof(float) * 8, 4);
		d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, sizeof(float) * 12, 4);
		d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceColours, instancingSlotIndex++, 4, 0);

		// find the last matching mesh
		uint64_t meshID = firstInstance->m_mesh.m_index;
		auto lastMeshInstance = std::find_if(firstInstance, m_instances.end(), [meshID](const MeshInstance& m) -> bool {
			return m.m_mesh.m_index != meshID;
		});

		// 1 draw call per texture
		auto firstTexInstance = firstInstance;
		while (firstTexInstance != lastMeshInstance)
		{
			uint64_t texID = firstTexInstance->m_texture.m_index;
			auto lastTexInstance = std::find_if(firstTexInstance, lastMeshInstance, [texID](const MeshInstance& m) -> bool {
				return m.m_texture.m_index != texID;
			});

			// firstTexInstance -> lastTexInstance instances to draw
			TextureHandle texture = texID != (uint64_t)-1 ? firstTexInstance->m_texture : TextureHandle{ 0 };
			ub.SetSampler("MyTexture", m_textures->GetTexture(texture)->GetHandle());
			d.SetUniforms(*m_shaders, ub);
			for (const auto& chunk : theMesh->GetChunks())
			{
				auto instanceCount = (uint32_t)(lastTexInstance - firstTexInstance);
				uint32_t firstIndex = (uint32_t)(firstTexInstance - m_instances.begin());
				d.DrawPrimitivesInstanced(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount, instanceCount, firstIndex);
			}		
			firstTexInstance = lastTexInstance;
		}
		firstInstance = lastMeshInstance;
	}
}