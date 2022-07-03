#pragma once

struct VkApplicationInfo;

namespace Scene
{
    struct SceneData;
}

namespace ForwardRenderer
{
    extern bool const Initialise(VkApplicationInfo const & ApplicationInfo);

    extern bool const Shutdown();

    extern void CreateNewActorResources(Scene::SceneData const & Scene);

    extern void Render(Scene::SceneData const & Scene);
}