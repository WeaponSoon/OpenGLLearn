#pragma once
#include "scene_m2.h"

class FView
{
public:
    glm::mat4 view;
    glm::mat4 project;
};

using FFrameBufferRef = std::shared_ptr<class FFrameBuffer>;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class FCameraComponent : public FSceneComponent
{
public:

    static std::shared_ptr<FShader> FinalShader;
    static std::shared_ptr<class FPrimitive> FinalPrimitive;

    std::set<std::shared_ptr<class FPrimitiveComponent>> ignorePrimitives;
    std::set<std::shared_ptr<class FPrimitiveComponent>> renderOnlyPrimitives;

    class FDeferredDrawer
    {
        friend class FCameraComponent;
        friend class FCameraProxy;
        std::queue<std::function<void()>> preDeferredCommands;
        std::queue<std::function<void()>> deferredCommands;
        std::set<const FCameraComponent*> registeredCamera;
    public:
        void Execute()
        {
            while (!preDeferredCommands.empty())
            {
                preDeferredCommands.front()();
                preDeferredCommands.pop();
            }
            while (!deferredCommands.empty())
            {
                deferredCommands.front()();
                deferredCommands.pop();
            }
            registeredCamera.clear();
        }
    };

    static FDeferredDrawer& GetDeferredCmds();

    FCubeTextureRef TextureEnv;
    FFrameBufferRef frameBufferRef;
    FFrameBufferRef gBufferRef;
    FFrameBufferRef gFlipBufferRefs[1];

    bool bDeferredPipeline = true;

    void AdjustGBuffer();

    bool bDrawEveryFrame = true;

    float nearPlane = .1f;
    float farPlane = 100.f;
    float aspectRatio = 1.33f;

    float Zoom = 90;

    // constructor with vectors
    FCameraComponent() = default;

    void Init(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f));

    ~FCameraComponent() override
    {
        GetDeferredCmds().registeredCamera.erase(this);
    }

    virtual void FinalTick(float deltaSecond) override
    {
        FComponent::FinalTick(deltaSecond);
        if (bDrawEveryFrame)
        {
            DrawDeferred();
        }
    }
    bool ShouldAdd() const override { return false; }
    void DrawDeferred() const;

    void Draw() const;


    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix() const
    {
        return glm::inverse(GetWorldTransform());
    }

    glm::mat4 GetProjectionMatrix() const
    {
        return glm::perspective(glm::radians(Zoom), aspectRatio, nearPlane, farPlane);
    }

    FView GetView() const
    {
        FView ret;
        ret.view = GetViewMatrix();
        ret.project = GetProjectionMatrix();
        return ret;
    }

};


using FCameraRef = std::shared_ptr<FCameraComponent>;

class FCameraProxy :FSceneProxy {
    static std::shared_ptr<FShader> FinalShader;
    static std::shared_ptr<class FPrimitive> FinalPrimitive;

    std::set<std::shared_ptr<class FPrimitiveComponent>> ignorePrimitives;
    std::set<std::shared_ptr<class FPrimitiveComponent>> renderOnlyPrimitives;
    FCubeTextureRef TextureEnv;
    FFrameBufferRef frameBufferRef;
    FFrameBufferRef gBufferRef;
    FFrameBufferRef gFlipBufferRefs[1];

    bool bDeferredPipeline = true;


    bool bDrawEveryFrame = true;

    float nearPlane = .1f;
    float farPlane = 100.f;
    float aspectRatio = 1.33f;

    float Zoom = 90;
public:
    FCameraProxy(const FCameraComponent* InComponent) :FSceneProxy(InComponent),
        ignorePrimitives(InComponent->ignorePrimitives),
        renderOnlyPrimitives(InComponent->renderOnlyPrimitives),
        TextureEnv(InComponent->TextureEnv),
        frameBufferRef(InComponent->frameBufferRef),
        gBufferRef(InComponent->gBufferRef),
        bDeferredPipeline(InComponent->bDeferredPipeline),
        bDrawEveryFrame(InComponent->bDrawEveryFrame),
        nearPlane(InComponent->nearPlane),
        farPlane(InComponent->farPlane),
        aspectRatio(InComponent->aspectRatio),
        Zoom(InComponent->Zoom)

    {
        gFlipBufferRefs[0] = InComponent->gFlipBufferRefs[0];
    };



    glm::mat4 GetViewMatrix() const
    {
        return glm::inverse(GetWorldTransform());
    }

    glm::mat4 GetProjectionMatrix() const
    {
        return glm::perspective(glm::radians(Zoom), aspectRatio, nearPlane, farPlane);
    }



    FView GetView() const
    {
        FView ret;
        ret.view = GetViewMatrix();
        ret.project = GetProjectionMatrix();
        return ret;
    }


    void Draw() const;
    void AdjustGBuffer();
};