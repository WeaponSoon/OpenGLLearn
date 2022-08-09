#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <functional>
#include <sstream>
#include <iostream>
#include <map> 
#include <queue>
#include <set> 
#include <stb_image.h>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum class ECameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    Up,
    Down,
};

// Default camera values
const float YAW = 0.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class FObject : public std::enable_shared_from_this<FObject>
{
public:
    virtual ~FObject() = default;
};

class FComponent : public FObject
{
private:
    static long long int GenComponentID();

    long long int componentId = -1;
public:
    std::weak_ptr<class FScene> scene;

    FComponent() : componentId(GenComponentID())
    {
	    
    }

    virtual ~FComponent() = default;

    virtual void PreTick(float deltaSecond) {};

    virtual void EarlyTick(float deltaSecond) {};

    virtual void Tick(float deltaSecond){};

    virtual void PostTick(float deltaSecond) {};

    virtual void FinalTick(float deltaSecond) {};
};

enum class EAttachRule
{
	AR_KeepWorld,
    AR_KeepRelative,
    AR_SnapToTarget,
};

//class FSceneComponent;
//namespace std
//{
//	template<>
//    struct less<std::weak_ptr<FSceneComponent>>
//	{
//        bool operator()(const std::weak_ptr<class FSceneComponent>& a, const std::weak_ptr<class FSceneComponent>& b) const;
//
//	};
//}

class FSceneComponent : public FComponent
{
    glm::mat4 matrix = glm::mat4(1.0f);
    std::weak_ptr<FSceneComponent> parent;
    std::set<std::shared_ptr<FSceneComponent>> children;

    bool HasChild(const std::shared_ptr<FSceneComponent>& inChild) const
    {
        if(children.find(inChild) != children.end())
        {
            return true;
        }

        for (auto&& child : children)
        {
	        if(child->HasChild(inChild))
	        {
                return true;
	        }
        }

        return false;
    }

public:
    std::weak_ptr<FSceneComponent> GetParent() const
    {
        return parent;
    }
    
    std::set<std::shared_ptr<FSceneComponent>> GetAllChildren() const
    {
        return children;
    }

    glm::mat4 GetWorldTransform() const
    {
        std::shared_ptr<FSceneComponent> safe_parent = parent.lock();
        if(safe_parent)
        {
            return safe_parent->GetWorldTransform() * matrix;
        }
        return matrix;
    }

    glm::mat4 GetLocalTransform() const
    {
        return matrix;
    }

    void SetLocalTransform(const glm::mat4& inTransform)
    {
        matrix = inTransform;
    }

    void SetWorldTransform(const glm::mat4& inTransform)
    {
        std::shared_ptr<FSceneComponent> safe_parent = parent.lock();
        if(!safe_parent)
        {
            matrix = inTransform;
        }
        else
        {
            auto&& parentTransform = safe_parent->GetWorldTransform();
            matrix = glm::inverse(parentTransform) * inTransform;
        }
    }

    glm::vec3 GetLocalLocation() const
    {
        return (matrix[3]);
    }

    glm::vec3 GetWorldLocation() const
    {
        return GetWorldTransform()[3];
    }

    void SetLocalLocation(const glm::vec3& inLocation)
    {
        matrix[3] = glm::vec4(inLocation, matrix[3][3]);
    }

    void SetWorldLocation(const glm::vec3& inLocation)
    {
        auto worldMatrix = GetWorldTransform();
        worldMatrix[3] = glm::vec4(inLocation, worldMatrix[3][3]);
        SetWorldTransform(worldMatrix);
    }

    glm::vec3 GetFowardInWorldSpace() const
    {
        return normalize((GetWorldTransform() * glm::vec4(0, 0, -1, 0)));
    }

    glm::vec3 GetRightInWorldSpace() const
    {
        return normalize((GetWorldTransform() * glm::vec4(1, 0, 0, 0)));
    }

    glm::vec3 GetUpInWorldSpace() const
    {
        return normalize((GetWorldTransform() * glm::vec4(0, 1, 0, 0)));
    }

    void AttachTo(std::shared_ptr<FSceneComponent> newParent, EAttachRule inRule)
    {
        auto shared_this = std::static_pointer_cast<FSceneComponent>(this->shared_from_this());

        if(newParent == shared_this)
        {
            return;
        }

        std::shared_ptr<FSceneComponent> safe_parent = parent.lock();
        if(safe_parent == newParent)
        {
            return;
        }

        if(HasChild(newParent))
        {
            return;
        }

        glm::mat4 oldWorld = GetWorldTransform();
    	if(safe_parent)
        {
            safe_parent->children.erase(shared_this);
        }
        parent = newParent;
        switch (inRule)
        {
        case EAttachRule::AR_KeepRelative:
            break;
        case EAttachRule::AR_SnapToTarget:
            SetLocalTransform(glm::mat4(1.0f));
            break;
        case EAttachRule::AR_KeepWorld:
            SetWorldTransform(oldWorld);
            break;
        }
    }
};


class FScene : public FObject
{
    std::vector<std::shared_ptr<FComponent>> components;

public:
    template<typename T>
    std::shared_ptr<T> CreateComponent()
    {
        auto ret = std::make_shared<T>();
        auto base = std::static_pointer_cast<FComponent>(ret);
    	base->scene = std::static_pointer_cast<FScene>(this->shared_from_this());
        components.push_back(base);
        return ret;
    }

    template<typename T, typename...Args>
    std::shared_ptr<T> CreateComponentWithArg(Args...args)
    {
        auto ret = std::make_shared<T>(std::forward<Args>(args)...);
        auto base = std::static_pointer_cast<FComponent>(ret);
        base->scene = std::static_pointer_cast<FScene>(this->shared_from_this());
        components.push_back(base);
        return ret;
    }

    const std::vector<std::shared_ptr<FComponent>>& GetAllComponents() const
    {
        return components;
    }

    void Tick(float deltaSecond)
    {
        auto componentsCopy = components;

        for (auto&& component : componentsCopy)
        {
            component->PreTick(deltaSecond);
        }

        for (auto&& component : componentsCopy)
        {
            component->EarlyTick(deltaSecond);
        }

	    for(auto&& component : componentsCopy)
	    {
            component->Tick(deltaSecond);
	    }

        for (auto&& component : componentsCopy)
        {
            component->PostTick(deltaSecond);
        }

        for (auto&& component : componentsCopy)
        {
            component->FinalTick(deltaSecond);
        }
    }

};

using FSceneRef = std::shared_ptr<FScene>;

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

    class FDeferredDrawer
    {
        friend class FCameraComponent;
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

    FFrameBufferRef frameBufferRef;

    bool bDrawEveryFrame = true;

    float nearPlane = .1f;
    float farPlane = 100.f;
    float aspectRatio = 1.33f;

    float Yaw;
    float Pitch;
    
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // constructor with vectors
    FCameraComponent(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
   
    ~FCameraComponent() override
    {
        GetDeferredCmds().registeredCamera.erase(this);
    }

    virtual void FinalTick(float deltaSecond) override
    {
	    if(bDrawEveryFrame)
	    {
            DrawDeferred();
	    }
    }

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

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(ECameraMovement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == ECameraMovement::FORWARD)
            SetWorldLocation(GetWorldLocation() + GetFowardInWorldSpace() * velocity);

    	if (direction == ECameraMovement::BACKWARD)
            SetWorldLocation(GetWorldLocation() - GetFowardInWorldSpace() * velocity);
            
        if (direction == ECameraMovement::LEFT)
            SetWorldLocation(GetWorldLocation() - GetRightInWorldSpace() * velocity);
            
        if (direction == ECameraMovement::RIGHT)
            SetWorldLocation(GetWorldLocation() + GetRightInWorldSpace() * velocity);

        if (direction == ECameraMovement::Up)
            SetWorldLocation(GetWorldLocation() + GetUpInWorldSpace() * velocity);

        if (direction == ECameraMovement::Down)
            SetWorldLocation(GetWorldLocation() - GetUpInWorldSpace() * velocity);
            
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw -= xoffset;
        Pitch += yoffset;
        
        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        auto transform = glm::translate(glm::mat4(1.0f), GetWorldLocation()) * 
            glm::rotate(glm::mat4(1.0f), glm::radians(Yaw), glm::vec3(0,1,0)) * 
            glm::rotate(glm::mat4(1.0f), glm::radians(Pitch), glm::vec3(1, 0, 0));

        SetWorldTransform(transform);
    }
};


using FCameraRef = std::shared_ptr<FCameraComponent>;

enum class ETextureWarpMethod
{
    TWM_Unknow = 0,
    TWM_Clamp = GL_CLAMP_TO_EDGE,
    TWM_Repeat = GL_REPEAT,
};

enum class ETextureFilterMethod
{
    TFM_Unknow = 0,
    TFM_Nearest = GL_NEAREST,
    TFM_Linear = GL_LINEAR,
    TFM_TriNearest = GL_LINEAR_MIPMAP_NEAREST,
    TFM_TriLinear = GL_LINEAR_MIPMAP_LINEAR,
};

enum class ETexturePixelFormat : int
{
    TPF_Unknow = GL_NONE,
	TPF_RGB = GL_RGB,
    TPF_RGBA = GL_RGBA,
    TPF_RGBA16F = GL_RGBA16F,
    TPF_D24S8 = GL_DEPTH24_STENCIL8,
};

class FTexture
{
public:
    unsigned int ID;

    ETexturePixelFormat TextureFormat;

    FTexture(int width, int height, ETexturePixelFormat inTextureFormat, bool bGenMipmap = false) : ID(GL_NONE)
    {
        if(inTextureFormat == ETexturePixelFormat::TPF_Unknow)
        {
            return;
        }

        TextureFormat = inTextureFormat;

        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);

        int format = GL_NONE;
        int elementType = GL_NONE;
        switch (inTextureFormat)
        {
        case ETexturePixelFormat::TPF_Unknow: break;
        case ETexturePixelFormat::TPF_RGB: 
            format = GL_RGB;
            elementType = GL_UNSIGNED_BYTE;
            break;
        case ETexturePixelFormat::TPF_RGBA: 
            format = GL_RGBA;
            elementType = GL_UNSIGNED_BYTE;
            break;
        case ETexturePixelFormat::TPF_RGBA16F: 
            format = GL_RGBA;
            elementType = GL_FLOAT;
            break;
        case ETexturePixelFormat::TPF_D24S8: 
            format = GL_DEPTH_STENCIL;
            elementType = GL_UNSIGNED_INT_24_8;
            break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(inTextureFormat), width, height, 0, format, elementType, nullptr);

        if(bGenMipmap)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bGenMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bGenMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    FTexture(const std::string& texturePath, ETextureWarpMethod warpMethod, ETextureFilterMethod filterMethod) : ID(GL_NONE), TextureFormat(ETexturePixelFormat::TPF_Unknow)
    {
        int width, height, nrChannels;
        auto data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            int format;
            switch (nrChannels)
            {
            case 3:
                format = GL_RGB;
                TextureFormat = ETexturePixelFormat::TPF_RGB;
                break;
            case 4:
                format = GL_RGBA;
                TextureFormat = ETexturePixelFormat::TPF_RGBA;
                break;
            default:
                format = GL_NONE;
                break;
            }

            if (format == GL_NONE)
            {
                std::cout << "unsupported texture format" << std::endl;
                return;
            }

            glGenTextures(1, &ID);
            glBindTexture(GL_TEXTURE_2D, ID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(warpMethod));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(warpMethod));

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(filterMethod));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(filterMethod));
            
            

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            return;
        }
        
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
    }

    bool IsValid() const
    {
        return ID != GL_NONE;
    }

    virtual ~FTexture()
    {
        if (ID != GL_NONE)
        {
            glDeleteTextures(1, &ID);
        }
    }
};

using FTextureRef = std::shared_ptr<FTexture>;

enum class EFrameBufferColorFormat
{
	FCF_Unknow = (int)ETexturePixelFormat::TPF_Unknow,
    FCF_RGB = (int)ETexturePixelFormat::TPF_RGB,
    FCF_RGBA = (int)ETexturePixelFormat::TPF_RGBA,
    FCF_RGBA16F = (int)ETexturePixelFormat::TPF_RGBA16F,
};

class FFrameBuffer
{

private:

    friend class FCameraComponent;

    GLuint FBO;

    static constexpr int MaxSupportColorAttachment = 4;

    std::vector<std::function<void(void)>> cmds;

    void Use()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    }

public:
	FTextureRef Color[MaxSupportColorAttachment];
    FTextureRef Depth;

    bool IsEmpty() const { return FBO == GL_NONE; }

    glm::vec4 clearColor;

    void Clear()
    {
        Use();
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    FFrameBuffer() : FBO(GL_NONE), clearColor(0,0,0,0){}

    FFrameBuffer(int width, int height, int n, EFrameBufferColorFormat inTextureFormat) : FBO(GL_NONE), clearColor(0,0,0,0)
    {
        const int numOfColorAttachment = (n > MaxSupportColorAttachment) ? MaxSupportColorAttachment : n;
        for(int i = 0; i < numOfColorAttachment; ++i)
        {
            Color[i] = std::make_shared<FTexture>(width, height, static_cast<ETexturePixelFormat>(inTextureFormat));
        }
        Depth = std::make_shared<FTexture>(width, height, ETexturePixelFormat::TPF_D24S8);

        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        for(int i = 0; i < numOfColorAttachment; ++i)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, Color[i]->ID, 0);
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, Depth->ID, 0);


        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~FFrameBuffer()
    {
	    if(FBO != GL_NONE)
	    {
            glDeleteFramebuffers(1, &FBO);
	    }
    }
};


struct FPrimitiveVertexPropDesc
{
    int index;
    void* offset;
    int elementType;
    int elementCount;

    FPrimitiveVertexPropDesc(int inIndex, void* inOffset, int inElementType, int inElementCount) : index(inIndex), offset(inOffset), elementType(inElementType), elementCount(inElementCount)
    {
	    
    }
};

struct FPrimitiveVertexDesc
{
    int structSize;
    std::vector<FPrimitiveVertexPropDesc> props;
};


class FPrimitive
{
    friend class FRenderBatch;
    GLuint VAO = GL_NONE;
    GLuint VBO = GL_NONE;
    GLuint EBO = GL_NONE;

    int numOfVertex = 0;
    int numOfIndices = 0;
public:

    void SetData(const std::vector<char>& vertexData, const std::vector<unsigned int>& indicesData, const FPrimitiveVertexDesc& desc)
    {
        numOfVertex = vertexData.size() / desc.structSize;
        numOfIndices = indicesData.size();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        if(numOfIndices > 0)
        {
            glGenBuffers(1, &EBO);
        }

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size(), vertexData.data(), GL_STATIC_DRAW);

        if(numOfIndices > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesData.size() * sizeof(unsigned int), indicesData.data(), GL_STATIC_DRAW);
        }

        for (auto&& prop : desc.props)
        {
            if(prop.index >= 0)
            {
                glEnableVertexAttribArray(prop.index);
                glVertexAttribPointer(prop.index, prop.elementCount, prop.elementType, GL_FALSE, desc.structSize, prop.offset);
            }
        }

        glBindVertexArray(0);
    }

	void use()
    {
        glBindVertexArray(VAO);
    }

    int GetNumOfVertex() const { return numOfVertex; }
    int GetNumOfIndices() const { return numOfIndices; }

    virtual ~FPrimitive()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }


};

using FPrimitiveRef = std::shared_ptr<FPrimitive>;

enum class ECullMethod
{
    CM_None,
    CM_CullFront,
    CM_CullBack,
    CM_CullFrontAndBack
};

class FShader
{
    friend class FCameraComponent;
    friend class FRenderBatch;

    struct TextureMark
    {
        int slot = -1;
        FTextureRef texture;
    };
    mutable int textureSlot = -1;
    std::map<std::string, TextureMark> textureMap;
    std::map<std::string, bool> boolMap;
    std::map<std::string, int> intMap;
    std::map<std::string, float> floatMap;
    std::map<std::string, glm::vec2> vec2Map;
    std::map<std::string, glm::vec3> vec3Map;
    std::map<std::string, glm::vec4> vec4Map;
    std::map<std::string, glm::mat2> mat2Map;
    std::map<std::string, glm::mat3> mat3Map;
    std::map<std::string, glm::mat4> mat4Map;
    ECullMethod cullMethod = ECullMethod::CM_CullBack;

    void ApplyCullMethod() const
    {
	    switch (cullMethod)
	    {
	    case ECullMethod::CM_CullBack:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
	    case ECullMethod::CM_CullFrontAndBack:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT_AND_BACK);
            break;
	    case ECullMethod::CM_CullFront:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
	    case ECullMethod::CM_None:
            glDisable(GL_CULL_FACE);
            break;
	    }
    }
    bool IsUsing() const
    {
        GLint currentProgram = GL_NONE;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        return ID != GL_NONE && ID == currentProgram;
    }

    // activate the shader
    // ------------------------------------------------------------------------
    void use() const
    {
        glUseProgram(ID);

        ApplyCullMethod();

        {
            for (auto&& pair : textureMap)
            {
                if (pair.second.slot >= 0 && pair.second.texture->IsValid())
                {
                    glActiveTexture(GL_TEXTURE0 + pair.second.slot);
                    glBindTexture(GL_TEXTURE_2D, pair.second.texture->ID);
                    glUniform1i(glGetUniformLocation(ID, pair.first.c_str()), pair.second.slot);
                }
            }
        }


        for (auto&& pair : boolMap)
        {
            glUniform1i(glGetUniformLocation(ID, pair.first.c_str()), static_cast<int>(pair.second));
        }

        for (auto&& pair : intMap)
        {
            glUniform1i(glGetUniformLocation(ID, pair.first.c_str()), pair.second);
        }

        for (auto&& pair : floatMap)
        {
            glUniform1f(glGetUniformLocation(ID, pair.first.c_str()), pair.second);
        }

        for (auto&& pair : vec2Map)
        {
            glUniform2fv(glGetUniformLocation(ID, pair.first.c_str()), 1, &pair.second[0]);
        }

        for (auto&& pair : vec3Map)
        {
            glUniform3fv(glGetUniformLocation(ID, pair.first.c_str()), 1, &pair.second[0]);
        }

        for (auto&& pair : vec4Map)
        {
            glUniform4fv(glGetUniformLocation(ID, pair.first.c_str()), 1, &pair.second[0]);
        }

        for (auto&& pair : mat2Map)
        {
            glUniformMatrix2fv(glGetUniformLocation(ID, pair.first.c_str()), 1, GL_FALSE, &pair.second[0][0]);
        }

        for (auto&& pair : mat3Map)
        {
            glUniformMatrix3fv(glGetUniformLocation(ID, pair.first.c_str()), 1, GL_FALSE, &pair.second[0][0]);
        }

        for (auto&& pair : mat4Map)
        {
            glUniformMatrix4fv(glGetUniformLocation(ID, pair.first.c_str()), 1, GL_FALSE, &pair.second[0][0]);
        }
    }
public:
    unsigned int ID;

    GLuint GetID() const
    {
        return ID;
    }
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    FShader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try 
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();		
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();			
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char * fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        
    }

    

    virtual ~FShader()
    {
        if (IsUsing())
        {
            glUseProgram(GL_NONE);
        }
        if (ID != GL_NONE)
        {
            glDeleteProgram(ID);
        }
    }


    void SetCullMethod(ECullMethod inMethod)
    {
        cullMethod = inMethod;
        if(IsUsing())
        {
            ApplyCullMethod();
        }
    }

    // utility uniform functions
    // ------------------------------------------------------------------------
    bool setBool(const std::string &name, bool value)
    {
        GLint Location = glGetUniformLocation(ID, name.c_str());
        if (Location >= 0)
        {
            boolMap[name] = value;
            if (IsUsing())
            {
                glUniform1i(Location, (int)value);
            }
            return true;
        }
        return false;
        
    }
    // ------------------------------------------------------------------------
    bool setInt(const std::string &name, int value) 
    { 
        GLint Location = glGetUniformLocation(ID, name.c_str());
        if (Location >= 0)
        {
            intMap[name] = value;
            if (IsUsing())
            {
                glUniform1i(Location, value);
            }
            return true;
        }
        return false;
    }
    // ------------------------------------------------------------------------
    bool setFloat(const std::string &name, float value) 
    { 
        GLint Location = glGetUniformLocation(ID, name.c_str());
        if (Location >= 0)
        {
            floatMap[name] = value;
            if (IsUsing())
            {
                glUniform1f(Location, value);
            }
            return true;
        }
        return false;   
    }
    // ------------------------------------------------------------------------
    bool setVec2(const std::string &name, const glm::vec2 &value) 
    { 
        GLint Location = glGetUniformLocation(ID, name.c_str());
        if (Location >= 0)
        {
            vec2Map[name] = value;
            if (IsUsing())
            {
                glUniform2fv(Location, 1, &vec2Map[name][0]);
            }
            return true;
        }
        return false;
    }
    bool setVec2(const std::string &name, float x, float y)
    { 
        return setVec2(name, glm::vec2(x, y));
    }
    // ------------------------------------------------------------------------
    bool setVec3(const std::string &name, const glm::vec3 &value) 
    { 
        GLint Location = glGetUniformLocation(ID, name.c_str());
        if (Location >= 0)
        {
            vec3Map[name] = value;
            if (IsUsing())
            {
                glUniform3fv(Location, 1, &vec3Map[name][0]);
            }
            return true;
        }
        return false;
    }
    bool setVec3(const std::string &name, float x, float y, float z)
    { 
        return setVec3(name, glm::vec3(x, y, z));
    }
    // ------------------------------------------------------------------------
    bool setVec4(const std::string &name, const glm::vec4 &value)
    { 
        GLint Location = glGetUniformLocation(ID, name.c_str());
        if (Location >= 0)
        {
            vec4Map[name] = value;
            if (IsUsing())
            {
                glUniform4fv(Location, 1, &vec4Map[name][0]);
            }
            return true;
        }
        return false;

        
    }
    bool setVec4(const std::string &name, float x, float y, float z, float w)
    { 
        return setVec4(name, glm::vec4(x, y, z, w));
    }
    // ------------------------------------------------------------------------
    bool setMat2(const std::string &name, const glm::mat2 &mat)
    {
        GLint Location = glGetUniformLocation(ID, name.c_str());
        if (Location >= 0)
        {
            mat2Map[name] = mat;
            if (IsUsing())
            {
                glUniformMatrix2fv(Location, 1, GL_FALSE, &mat2Map[name][0][0]);
            }
            return true;
        }
        return false;

        
    }
    // ------------------------------------------------------------------------
    bool setMat3(const std::string &name, const glm::mat3 &mat) 
    {
        GLint Location = glGetUniformLocation(ID, name.c_str());
        if (Location >= 0)
        {
            mat3Map[name] = mat;
            if (IsUsing())
            {
                glUniformMatrix3fv(Location, 1, GL_FALSE, &mat3Map[name][0][0]);
            }
            return true;
        }
        return false;
    }
    // ------------------------------------------------------------------------
    bool setMat4(const std::string &name, const glm::mat4 &mat)
    {
        GLint Location = glGetUniformLocation(ID, name.c_str());
        if (Location >= 0)
        {
            mat4Map[name] = mat;
            if (IsUsing())
            {
                glUniformMatrix4fv(Location, 1, GL_FALSE, &mat4Map[name][0][0]);
            }
            return true;
        }
        return false;
    }

    bool SetTexture(const std::string& name, const FTextureRef& inTexture)
    {
        GLint Location = glGetUniformLocation(ID, name.c_str());
        if (Location >= 0)
        {
            auto&& textureStruct = textureMap[name];
            if (textureStruct.slot < 0)
            {
                textureStruct.slot = ++textureSlot;
            }

            textureStruct.texture = inTexture;
            if (IsUsing())
            {
                glActiveTexture(GL_TEXTURE0 + textureStruct.slot);
                glBindTexture(GL_TEXTURE_2D, textureStruct.texture->ID);
                glUniform1i(Location, textureStruct.slot);
            }
            return true;
        }
        return false;
        
    }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

using FShaderRef = std::shared_ptr<FShader>;


class FRenderBatch
{
public:
    FPrimitiveRef Primitive;
    FShaderRef Shader;
    glm::mat4 model;


    void Draw(const FCameraRef& camera)
    {
        FView&& view = camera->GetView();
        Shader->setMat4("projection", view.project);
        Shader->setMat4("view", view.view);
        Shader->setMat4("model", model);

        Shader->use();
        Primitive->use();

        if(Primitive->GetNumOfIndices() > 0)
        {
            glDrawElements(GL_TRIANGLES, Primitive->GetNumOfIndices(), GL_UNSIGNED_INT, nullptr);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, Primitive->GetNumOfVertex());
        }

        glBindVertexArray(0);
    }
};


class FPrimitiveComponent : public FSceneComponent
{
public:
    FPrimitiveRef Primitive;
    FShaderRef Shader;

    
    virtual FRenderBatch GenerateRenderBatch() const
    {
        FRenderBatch ret;
        ret.model = GetWorldTransform();
        ret.Primitive = Primitive;
        ret.Shader = Shader;
        return ret;
    }
};



#endif
