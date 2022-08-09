#include "shader_m2.h"

long long FComponent::GenComponentID()
{
    static long long int curId = 0;
    return curId++;
}

FCameraComponent::FCameraComponent(glm::vec3 position, float yaw, float pitch) : MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
    frameBufferRef = std::make_shared<FFrameBuffer>();
    SetWorldLocation(position);
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}
void FCameraComponent::Draw() const
{

    frameBufferRef->Use();

    glm::vec4 clearColor = frameBufferRef->clearColor;

    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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


