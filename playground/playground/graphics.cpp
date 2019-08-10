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
#include "stb_image.h"
#include <sol.hpp>

struct TextureHandle
{
	uint64_t m_index;
	static TextureHandle Invalid() { return { (uint64_t)-1 }; };
};

struct Graphics::Quad
{
	glm::vec2 m_position;
	glm::vec2 m_size;
	glm::vec4 m_colour;
	TextureHandle m_texture;
};

const uint64_t c_maxQuads = 1024 * 128;

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
	void BuildInstanceBuffers();
	std::vector<Quad>& m_quads;
	std::unique_ptr<Render::Mesh> m_quadMesh;
	std::unique_ptr<Render::Material> m_quadMaterial;
	std::unique_ptr<Render::ShaderProgram> m_quadShaders;
	SDE::RenderSystem* m_renderSystem;
	Graphics::TextureArray* m_textures;
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

void Graphics::DrawQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 colour, const TextureHandle& th)
{
	m_quads.push_back({ pos, size, colour, th });
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

	m_renderPass = std::make_unique<RenderPass2D>(m_renderSystem, m_textures.get(), m_quads);
	m_renderSystem->AddPass(*m_renderPass);

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
	graphics["LoadTexture"] = [this](const char* path) -> TextureHandle {
		return m_textures->LoadTexture(path);
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
	m_renderPass = nullptr;
	m_textures = nullptr;
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

void Graphics::RenderPass2D::BuildInstanceBuffers()
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

	BuildInstanceBuffers();

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