#pragma once
#include "math/glm_headers.h"
#include "math/box3.h"
#include "texture_manager.h"
#include <memory>

namespace Render
{
	class Mesh;
}

namespace Assets
{
	class Model;
}

namespace smol
{
	class Renderer;
	class Model
	{
	public:
		Model() = default;
		~Model() = default;

		static std::unique_ptr<Model> CreateFromAsset(const Assets::Model& src, TextureManager& tm);

		struct Part
		{
			std::unique_ptr<Render::Mesh> m_mesh;
			glm::mat4 m_transform;
			Math::Box3 m_bounds;
		};
		const std::vector<Part>& Parts() const { return m_parts; }
		std::vector<Part>& Parts() { return m_parts; }
	private:
		std::vector<Part> m_parts;
	};
}