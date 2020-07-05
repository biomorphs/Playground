#pragma once
#include <string>
#include <vector>

namespace Render
{
	class Mesh;
}

namespace smol
{
	struct MeshHandle
	{
		uint64_t m_index;
		static MeshHandle Invalid() { return { (uint64_t)-1 }; };
	};

	class MeshManager
	{
	public:
		MeshManager() = default;
		~MeshManager();

		MeshHandle AddMesh(const char* name, Render::Mesh* m);
		MeshHandle LoadMesh(const char* name);
		Render::Mesh* GetMesh(const smol::MeshHandle& h);
	private:
		struct MeshDesc {
			Render::Mesh* m_mesh;
			std::string m_name;
		};
		std::vector<MeshDesc> m_meshes;
	};
}