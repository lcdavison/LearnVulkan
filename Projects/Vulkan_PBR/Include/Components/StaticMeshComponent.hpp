#pragma once

#include "Common.hpp"
#include "Utilities/Iterator.hpp"

namespace Scene
{
    struct SceneData;
}

namespace Components::StaticMesh::Types
{
    struct ComponentData
    {
        uint32 MeshHandle = {};
        uint32 MaterialHandle = {};
    };

    /* 
        This will be useful later, since we will be able to skip "free" components using some extra data to turn
        the component array into linked list.
    */
    class Iterator : public Utilities::IteratorBase
    {
    public:
        static Iterator Begin();
        static Iterator End();

        ComponentData operator * () const;

    private:
        explicit Iterator(uint32 const Index);
    };
}

namespace Components::StaticMesh
{
    extern bool const CreateComponent(Scene::SceneData & Scene, uint32 const ActorHandle, uint32 const StaticMeshHandle, uint32 const MaterialHandle);

    extern bool const GetComponentData(Scene::SceneData const & Scene, uint32 const ActorHandle, Types::ComponentData & OutputComponentData);
}