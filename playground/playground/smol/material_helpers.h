#pragma once
#include <map>
#include <string>

namespace Render
{
	class Device;
	class ShaderProgram;
	class Material;
}

namespace smol
{
	class TextureManager;
	struct TextureHandle;
	using DefaultTextures = std::map<std::string, TextureHandle>;
	void ApplyMaterial(Render::Device& d, Render::ShaderProgram& shader, const Render::Material& m, TextureManager& tm, const DefaultTextures& defaults);
}
