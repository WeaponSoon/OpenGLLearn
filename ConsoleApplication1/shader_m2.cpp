#include "shader_m2.h"

long long FComponent::GenComponentID()
{
    static long long int curId = 0;
    return curId++;
}

void FCameraComponent::Draw() const
{
    std::vector<FRenderBatch> renderBatches;
    auto safe_scene = scene.lock();
    auto&& allComponents = safe_scene->GetAllComponents();
    for (auto&& component : allComponents)
    {
        auto primitiveComponent = std::dynamic_pointer_cast<FPrimitiveComponent>(component);
        if (primitiveComponent)
        {
            renderBatches.emplace_back(primitiveComponent->GenerateRenderBatch());
        }
    }

    for (auto&& renderBatch : renderBatches)
    {
        renderBatch.Draw(std::static_pointer_cast<FCameraComponent>(((FCameraComponent*)this)->shared_from_this()));
    }
}


