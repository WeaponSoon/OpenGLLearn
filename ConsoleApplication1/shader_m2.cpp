#include "shader_m2.h"

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
    auto safe_scene = scene.lock();
    auto&& allComponents = safe_scene->GetAllComponents();
    for (auto&& component : allComponents)
    {
        auto primitiveComponent = std::dynamic_pointer_cast<FPrimitiveComponent>(component);
        if (primitiveComponent)
        { 
            primitiveComponent->GenerateRenderBatch(renderBatches);
        }
    }
    FFrameBufferRef useFrameBuffer = frameBufferRef ? frameBufferRef : FFrameBuffer::GetDefaultFrameBuffer();
    useFrameBuffer->Use();

    glm::vec4 clearColor = useFrameBuffer->clearColor;

    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto&& renderBatch : renderBatches)
    {
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


