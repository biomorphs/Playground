#include "mesh_manager.h"
#include "render/mesh.h"

namespace smol
{
	MeshHandle MeshManager::AddMesh(const char* name, Render::Mesh* m)
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

	MeshHandle MeshManager::LoadMesh(const char* name)
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

	Render::Mesh* MeshManager::GetMesh(const MeshHandle& h)
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

	MeshManager::~MeshManager()
	{
		for (auto& it : m_meshes)
		{
			delete it.m_mesh;
		}
	}
}
