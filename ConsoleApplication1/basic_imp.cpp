#include "shader_m2.h"
#include "primitive_m2.h"
#include "input_m2.h"
#include "primitive_move_m2.h"
#include "light_m2.h"

FInputReceiver& FInputReceiver::GetInputReceiver()
{
    static FInputReceiver innerReceiver;
    return innerReceiver;
}

long long FObject::GenComponentID()
{
    static long long int curId = 0;
    return curId++;
}

FCameraComponent::FDeferredDrawer& FCameraComponent::GetDeferredCmds()
{
    static FDeferredDrawer innerDrawer;
    return innerDrawer;
}

FCameraComponent::FCameraComponent(glm::vec3 position) :frameBufferRef(FFrameBuffer::GetDefaultFrameBuffer()), Zoom(ZOOM)
{
    SetWorldLocation(position);
}

void FCameraComponent::DrawDeferred() const
{
    auto&& drawer = GetDeferredCmds();
    if(drawer.registeredCamera.find(this) == drawer.registeredCamera.end())
    {
        drawer.registeredCamera.emplace(this);

        std::weak_ptr<const FCameraComponent> weakThis = std::static_pointer_cast<const FCameraComponent>(this->shared_from_this());

        if (!frameBufferRef || frameBufferRef->IsEmpty())
        {
            drawer.deferredCommands.emplace([weakThis]()->void
                {
                    auto safeCamera = weakThis.lock();
                    if (safeCamera)
                    {
                        safeCamera->Draw();
                    }
                });
        }
        else
        {
            drawer.preDeferredCommands.emplace([weakThis]()->void
                {
                    auto safeCamera = weakThis.lock();
                    if (safeCamera)
                    {
                        safeCamera->Draw();
                    }
                });
        }
    	
    }
}

void FCameraComponent::Draw() const
{
    std::vector<FRenderBatch> renderBatches;
    std::vector<FLightRenderBatch> lights;

    auto safe_scene = scene.lock();
    auto&& allComponents = safe_scene->GetAllComponents();
    for (auto&& component : allComponents)
    {
        auto primitiveComponent = std::dynamic_pointer_cast<FPrimitiveComponent>(component);
        if (primitiveComponent)
        {
            if(renderOnlyPrimitives.size() == 0)
            {
                if (ignorePrimitives.find(primitiveComponent) == ignorePrimitives.end())
                {
                    primitiveComponent->GenerateRenderBatch(renderBatches);
                }
            }
            else
            {
                if (renderOnlyPrimitives.find(primitiveComponent) != renderOnlyPrimitives.end())
                {
                    primitiveComponent->GenerateRenderBatch(renderBatches);
                }
            }
        }
        else
        {
            auto lightComponent = std::dynamic_pointer_cast<FLightComponent>(component);
            if(lightComponent)
            {
                lightComponent->GetLightRenderBatch(lights);
            }
        }
    }
    FFrameBufferRef useFrameBuffer = frameBufferRef ? frameBufferRef : FFrameBuffer::GetDefaultFrameBuffer();
    useFrameBuffer->Use();

    bool bViewportSet = false;
    if(!useFrameBuffer->IsEmpty())
    {
        if(useFrameBuffer->Color[0]->IsValid())
        {
            const glm::vec2 viewportSize = useFrameBuffer->Color[0]->GetSize();
            glViewport(0, 0, static_cast<GLsizei>(viewportSize.x), static_cast<GLsizei>(viewportSize.y));
            bViewportSet = true;
        }
        else if(useFrameBuffer->Depth->IsValid())
        {
            const glm::vec2 viewportSize = useFrameBuffer->Depth->GetSize();
            glViewport(0, 0, static_cast<GLsizei>(viewportSize.x), static_cast<GLsizei>(viewportSize.y));
            bViewportSet = true;
        }
    }
    if(!bViewportSet)
    {
        int x, y;
        glfwGetFramebufferSize(glfwGetCurrentContext(), &x, &y);
        glViewport(0, 0, x, y);
    }

    glm::vec4 clearColor = useFrameBuffer->clearColor;

    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto&& renderBatch : renderBatches)
    {
        renderBatch.Shader->setVec3("DirectionalLightDir", glm::vec3(1,0,0));
        renderBatch.Shader->setVec3("DirectionalLightColor", glm::vec3(0,0,0));

        for (int pointLightId = 0; pointLightId < 4; ++pointLightId)
        {
            renderBatch.Shader->setVec4(std::string("PointLightLocationAndRadius[") + std::to_string(pointLightId) + "]", glm::vec4(0,0,0,0));
            renderBatch.Shader->setVec3(std::string("PointLightColor[") + std::to_string(pointLightId) + "]", glm::vec3(0,0,0));
        }

        renderBatch.Shader->setVec3("EnvLightColor", glm::vec3(0, 0, 0));

        int pointLightNum = 0;
        for(auto&& light : lights)
        {
	        switch (light.lightType)
	        {
	        case ELightType::LT_Directional: 
                renderBatch.Shader->setVec3("DirectionalLightDir", -light.direction);
                renderBatch.Shader->setVec3("DirectionalLightColor", light.color);
                break;
	        case ELightType::LT_Point:
                if(pointLightNum < 4)
                {
                    renderBatch.Shader->setVec4(std::string("PointLightLocationAndRadius[") + std::to_string(pointLightNum) + "]", glm::vec4(light.location, light.radius));
                    renderBatch.Shader->setVec3(std::string("PointLightColor[") + std::to_string(pointLightNum) + "]", light.color);
                    ++pointLightNum;
                }
                break;
	        case ELightType::LT_Env:
                renderBatch.Shader->setVec3("EnvLightColor", light.color);
                break;

	        }
        }
        renderBatch.Draw(std::static_pointer_cast<FCameraComponent>(((FCameraComponent*)this)->shared_from_this()));
    }
    glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::shared_ptr<FTexture>& FTexture::GetBlack()
{
    static FTextureRef inner = std::make_shared<FTexture>(glm::vec3(0, 0, 0));
    return inner;
}

std::shared_ptr<FTexture>& FTexture::GetWhite()
{
    static FTextureRef inner = std::make_shared<FTexture>(glm::vec3(1, 1, 1));
    return inner;
}

const std::shared_ptr<FFrameBuffer>& FFrameBuffer::GetDefaultFrameBuffer()
{
    static std::shared_ptr<FFrameBuffer> inner = std::make_shared<FFrameBuffer>();
    return inner;
}


