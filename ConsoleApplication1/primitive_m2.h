#pragma once
#include "scene_m2.h"
#include "shader_m2.h"
#include "camera_m2.h"
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

    BVHBound Bound;

    template<typename T>
    void SetDataByStruct(const std::vector<T>& vertexData, const std::vector<unsigned int>& indicesData)
    {
        std::vector<char> dataInBytes;
        dataInBytes.resize(sizeof(T) * vertexData.size());
        memcpy(dataInBytes.data(), vertexData.data(), dataInBytes.size());
        SetData(dataInBytes, indicesData, T::GetVertexLayout());
    }

    void SetData(const std::vector<char>& vertexData, const std::vector<unsigned int>& indicesData, const FPrimitiveVertexDesc& desc)
    {
        numOfVertex = static_cast<int>(vertexData.size() / desc.structSize);
        numOfIndices = static_cast<int>(indicesData.size());

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        if (numOfIndices > 0)
        {
            glGenBuffers(1, &EBO);
        }

        glBindVertexArray(VAO);


        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size(), vertexData.data(), GL_STATIC_DRAW);

        if (numOfIndices > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesData.size() * sizeof(unsigned int), indicesData.data(), GL_STATIC_DRAW);
        }

        for (auto&& prop : desc.props)
        {
            if (prop.index >= 0)
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


class FRenderBatch
{
public:
    FPrimitiveRef Primitive;
    FShaderRef Shader;
    glm::mat4 model;
    GLenum PrimitiveMode = GL_TRIANGLES;
    GLenum PolygonMode = GL_FILL;
    void Draw_InputVP(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos)
    {
        if (Primitive && Shader)
        {
            Shader->setMat4("projection", proj);
            Shader->setMat4("view", view);
            Shader->setMat4("model", model);
            Shader->setVec3("cameraPos", cameraPos);
            Shader->use();
            Primitive->use();

            glPolygonMode(GL_FRONT_AND_BACK, PolygonMode);

            if (Primitive->GetNumOfIndices() > 0)
            {
                glDrawElements(PrimitiveMode, Primitive->GetNumOfIndices(), GL_UNSIGNED_INT, nullptr);
            }
            else
            {
                glDrawArrays(PrimitiveMode, 0, Primitive->GetNumOfVertex());
            }

            glBindVertexArray(0);
            glUseProgram(0);
        }
    }

    void Draw(const FCameraRef& camera)
    {
        if (Primitive && Shader)
        {
            FView&& view = camera->GetView();
            Shader->setMat4("projection", view.project);
            Shader->setMat4("view", view.view);
            Shader->setMat4("model", model);
            Shader->setVec3("cameraPos", camera->GetWorldLocation());
            Shader->use();
            Primitive->use();

            if (Primitive->GetNumOfIndices() > 0)
            {
                glDrawElements(PrimitiveMode, Primitive->GetNumOfIndices(), GL_UNSIGNED_INT, nullptr);
            }
            else
            {
                glDrawArrays(PrimitiveMode, 0, Primitive->GetNumOfVertex());
            }

            glBindVertexArray(0);
            glUseProgram(0);
        }
    }
};

struct FPrimitiveUnit
{
    FPrimitiveRef Primitive;
    FShaderRef Shader;
    BVHBound Bound;
    FPrimitiveUnit(FPrimitiveRef inPrimitive, FShaderRef inShader) : Primitive(inPrimitive), Shader(inShader)
    {

    }
};

class FPrimitiveComponent : public FSceneComponent
{

    std::vector<FPrimitiveUnit> primitives;
public:
    friend class FPrimitiveProxy;
    virtual void OnUpdateBoundCache() override
    {
        if(!primitives.empty())
        {
            BVHBound Bound = primitives[0].Primitive->Bound;

            {
                glm::vec3 newCenter = GetWorldTransform() * glm::vec4(Bound.Center(), 1);
                glm::vec3 newExtent = glm::abs(GetWorldTransform() * glm::vec4(Bound.end - Bound.Center(), 0));
                Bound.begin = newCenter - newExtent;
                Bound.end = newCenter + newExtent;
                primitives[0].Bound = Bound;
            }
            
            for(int pi = 1; pi < primitives.size(); ++pi)
            {
                BVHBound TempBound = primitives[pi].Primitive->Bound;
                glm::vec3 newCenter = GetWorldTransform() * glm::vec4(TempBound.Center(), 1);
                glm::vec3 newExtent = glm::abs(GetWorldTransform() * glm::vec4(TempBound.end - TempBound.Center(), 0));
                TempBound.begin = newCenter - newExtent;
                TempBound.end = newCenter + newExtent;
                Bound.begin = glm::min(Bound.begin, TempBound.begin);
                Bound.end = glm::max(Bound.end, TempBound.end);
                primitives[pi].Bound = Bound;
            }
            BoundCache = Bound;
        }
        else
        {
            return FSceneComponent::OnUpdateBoundCache();
        }
    }

    virtual size_t GetPrimitiveCount() const
    {
        return primitives.size();
    }

    virtual FPrimitiveRef GetPrimitive(int id) const
    {
        return primitives[id].Primitive;
    }

    virtual FShaderRef GetShader(int id) const
    {
        return primitives[id].Shader;
    }

    virtual void SetPrimitive(int id, FPrimitiveRef inPrimitive)
    {
        primitives[id].Primitive = inPrimitive;
        UpdateBoundCache();
    }

    virtual void SetShader(int id, FShaderRef inShader)
    {
        primitives[id].Shader = inShader;
    }

    virtual void AddPrimitiveUnit(FPrimitiveRef inPrimitve, FShaderRef inShader)
    {
        primitives.emplace_back(inPrimitve, inShader);
        UpdateBoundCache();
    }

    virtual void GenerateRenderBatch(std::vector<FRenderBatch>& outRenderBatch) const
    {
        for (auto&& unit : primitives)
        {
            FRenderBatch render_batch;
            render_batch.Shader = unit.Shader;
            render_batch.Primitive = unit.Primitive;
            render_batch.model = GetWorldTransform();
            render_batch.PrimitiveMode = (GLenum)unit.Shader->getPrimitiveMetohd();
            outRenderBatch.push_back(render_batch);
        }
    }
};

class FPrimitiveProxy :FSceneProxy {

    std::vector<FPrimitiveUnit> primitives;


public:
    FPrimitiveProxy(const FPrimitiveComponent* InComponent) :FSceneProxy(InComponent), primitives(InComponent->primitives) {};

    virtual ~FPrimitiveProxy();

    virtual size_t GetPrimitiveCount() const;

    virtual FPrimitiveRef GetPrimitive(int id) const;

    virtual FShaderRef GetShader(int id) const;


    virtual void SetShader(int id, FShaderRef inShader);



    virtual void GenerateRenderBatch(std::vector<FRenderBatch>& outRenderBatch) const;

};
