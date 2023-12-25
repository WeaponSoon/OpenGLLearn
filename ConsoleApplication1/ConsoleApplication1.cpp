#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "primitive_m2.h"
#include "primitive_move_m2.h"
#include "mode_m2l.h"
#include "light_m2.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;


float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

class FTestLightControlSubobject : public FComponentSubobject
{
public:
    std::weak_ptr<FDirectionalLightComponent> dirRef;
    virtual void Init() override
    {
        dirRef = std::dynamic_pointer_cast<FDirectionalLightComponent>(owner.lock());

        FInputReceiver::FObjectInputKey key;
        key.priority = 0;
        key.objId = dirRef.lock()->GetObjectId();

        auto&& inputHandle = FInputReceiver::GetInputReceiver().keyHandles[key];
        std::weak_ptr<FTestLightControlSubobject> weakThis = std::static_pointer_cast<FTestLightControlSubobject>(this->GetObject());

        inputHandle.repeatCallback = [weakThis](int key)->void
        {
            std::shared_ptr<FTestLightControlSubobject> safeThis = weakThis.lock();
            if (safeThis)
            {
                std::shared_ptr<FDirectionalLightComponent> dirOwner = safeThis->dirRef.lock();
                if (key == GLFW_KEY_N)
                {
                    auto trans = dirOwner->GetWorldTransform();
                    glm::vec3 scale;
                    glm::quat rotation;
                    glm::vec3 location;
                    glm::vec3 skew;
                    glm::vec4 persp;
                    glm::decompose(trans,scale,rotation, location, skew, persp);

                    rotation = glm::angleAxis(glm::radians(20 * FInputReceiver::GetInputReceiver().deltaTime), glm::vec3(0, 1, 0)) * rotation;

                    dirOwner->SetWorldTransform(glm::translate(glm::mat4_cast(rotation), location));
                    
                }
                else if (key == GLFW_KEY_M)
                {
                    auto trans = dirOwner->GetWorldTransform();
                    glm::vec3 scale;
                    glm::quat rotation;
                    glm::vec3 location;
                    glm::vec3 skew;
                    glm::vec4 persp;
                    glm::decompose(trans, scale, rotation, location, skew, persp);

                    rotation = glm::angleAxis(glm::radians(-20 * FInputReceiver::GetInputReceiver().deltaTime), glm::vec3(0, 1, 0)) * rotation;

                    dirOwner->SetWorldTransform(glm::translate(glm::mat4_cast(rotation), location));
                }
            }
        };
    }

};


void GenerateSphereModel(float inRadius, uint32_t inHorizantalSegments, uint32_t inVerticalSegments, std::vector<char>& outSphereData, std::vector<unsigned int>& outIndices, FPrimitiveVertexDesc& outVertexDesc)
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangent;
    std::vector<glm::vec3> bitangent;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    for (unsigned int x = 0; x <= inHorizantalSegments; ++x)
    {
        for (unsigned int y = 0; y <= inVerticalSegments; ++y)
        {
            float xSegment = (float)x / (float)inHorizantalSegments;
            float ySegment = (float)y / (float)inVerticalSegments;

            float xPosEquater = std::cos(xSegment * 2.0f * PI);// *std::sin(ySegment * PI);
            float zPosEquater = std::sin(xSegment * 2.0f * PI);// *std::sin(ySegment * PI);

            float xPos = xPosEquater * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = zPosEquater * std::sin(ySegment * PI); 

            positions.push_back(glm::vec3(xPos, yPos, zPos) * inRadius); 
            uv.push_back(glm::vec2(xSegment, ySegment));
            normals.push_back(glm::normalize(glm::vec3(xPos, yPos, zPos)));
            glm::vec3 tempTangent = glm::normalize(glm::cross(glm::vec3(xPosEquater, 0, zPosEquater), glm::vec3(0, 1, 0)));
            tangent.push_back(tempTangent);
            bitangent.push_back(glm::cross(glm::vec3(xPos, yPos, zPos), tempTangent));
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < inVerticalSegments; ++y)
    {
        if (!oddRow) // even rows: y == 0, y == 2; and so on
        {
            for (unsigned int x = 0; x <= inHorizantalSegments; ++x)
            {
                indices.push_back(y * (inHorizantalSegments + 1) + x);
                indices.push_back((y + 1) * (inHorizantalSegments + 1) + x);
            }
        }
        else
        {
            for (int x = inHorizantalSegments; x >= 0; --x)
            {
                indices.push_back((y + 1) * (inHorizantalSegments + 1) + x);
                indices.push_back(y * (inHorizantalSegments + 1) + x);
            }
        }
        oddRow = !oddRow;
    }
    //uint32_t indexCount = static_cast<unsigned int>(indices.size());

    outVertexDesc.structSize = 14 * sizeof(float);
    int propOffset = 0;
    outVertexDesc.props.emplace_back(0, (void*)propOffset, GL_FLOAT, 3);
    propOffset += 3 * sizeof(float);
    if(normals.size() > 0)
    {
        outVertexDesc.props.emplace_back(1, (void*)propOffset, GL_FLOAT, 3);
        propOffset += 3 * sizeof(float);
    }
    if(uv.size() > 0)
    {
        outVertexDesc.props.emplace_back(2, (void*)propOffset, GL_FLOAT, 2);
        propOffset += 2 * sizeof(float);
    }
    if(tangent.size() > 0)
    {
        outVertexDesc.props.emplace_back(3, (void*)propOffset, GL_FLOAT, 3);
        propOffset += 3 * sizeof(float);
    }
    if (bitangent.size() > 0)
    {
        outVertexDesc.props.emplace_back(4, (void*)propOffset, GL_FLOAT, 3);
        propOffset += 3 * sizeof(float);
    }

    std::vector<float> data;
    for (unsigned int i = 0; i < positions.size(); ++i)
    {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);
        if (normals.size() > 0)
        {
            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);
        }
        if (uv.size() > 0)
        {
            data.push_back(uv[i].x);
            data.push_back(uv[i].y);
        }
        if(tangent.size() > 0)
        {
            data.push_back(tangent[i].x);
            data.push_back(tangent[i].y);
            data.push_back(tangent[i].z);
        }
        if(bitangent.size() > 0)
        {
            data.push_back(bitangent[i].x);
            data.push_back(bitangent[i].y);
            data.push_back(bitangent[i].z);
        }
    }

	

    outSphereData.clear();
    outSphereData.resize(data.size() * sizeof(float));
    memcpy(outSphereData.data(), data.data(), outSphereData.size());
    outIndices.clear();
    outIndices.swap(indices);
}


int main()
{
#if 1 //假装是从文件加载进来的数据



    float vertices[] = {
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,


        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    float planeWithNormal[] = {
         -44.5f, -4.5f, -44.5f,  0.0f, 1.0f, 0.0f,
        44.5f, -4.5f, -44.5f,  0.0f, 1.0f, 0.0f,
         44.5f, -4.5f,  44.5f,  0.0f, 1.0f, 0.0f,
        -44.5f, -4.5f,  44.5f,  0.0f, 1.0f, 0.0f,
    };

    float groundVertex[] =
    {
        -44.5f, -4.5f, -44.5f,  0.0f, 1.0f,
        44.5f, -4.5f, -44.5f,  1.0f, 1.0f,
         44.5f, -4.5f,  44.5f,  1.0f, 0.0f,
        -44.5f, -4.5f,  44.5f,  0.0f, 0.0f,
    };

    unsigned int groundIndices[] =
    {
        0,2,1,0,3,2
    };

    // world space positions of our cubes
    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  0.0f, -15.0f),
        glm::vec3(-1.5f, -0.0f, -2.5f),
        glm::vec3(-3.8f, -0.0f, -12.3f),
        glm::vec3(2.4f, -0.0f, -3.5f),
        glm::vec3(-1.7f,  0.0f, -7.5f),
        glm::vec3(1.3f, -0.0f, -2.5f),
        glm::vec3(1.5f,  0.0f, -2.5f),
        glm::vec3(1.5f,  0.0f, -1.5f),
        glm::vec3(-1.3f,  0.0f, -1.5f)
    };


#endif

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl; 
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
     

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
     
    FShaderRef ourShader2 = std::make_shared<FShader>("shaders/simple_model_shader.vs", "shaders/simple_model_shader.fs");
    //ourShader2 = std::make_shared<FShader>("shaders/simple_model_deferred.vs", "shaders/simple_model_deferred.fs");

    //FModel loadedModel("./objects/backpack/backpack.obj");

    glEnable(GL_DEPTH_TEST);


    FSceneRef scene = std::make_shared<FScene>();

    std::vector<char> vertexData;
    vertexData.resize(sizeof(vertices));
    memcpy(vertexData.data(), vertices, vertexData.size());
    FPrimitiveVertexDesc vertexDesc;
    vertexDesc.structSize = 5 * sizeof(int);
    vertexDesc.props.emplace_back(0, reinterpret_cast<void*>(0), GL_FLOAT, 3);
    vertexDesc.props.emplace_back(2, reinterpret_cast<void*>(3 * sizeof(float)), GL_FLOAT, 2);

    //创建一个立方体模型
    FPrimitiveRef cube = std::make_shared<FPrimitive>();
    cube->SetData(vertexData, std::vector<unsigned int>(), vertexDesc);

    //创建一个平面模型
    FPrimitiveRef plane = std::make_shared<FPrimitive>();
    vertexData.resize(30 * sizeof(float));
    plane->SetData(vertexData, std::vector<unsigned int>(), vertexDesc);

    //再创建一个大一点的平面模型
    FPrimitiveRef ground = std::make_shared<FPrimitive>();
    vertexData.resize(sizeof(groundVertex));
    memcpy(vertexData.data(), groundVertex, vertexData.size());
    std::vector<unsigned int> indices;
    indices.resize(sizeof(groundIndices));
    memcpy(indices.data(), groundIndices, indices.size());
    ground->SetData(vertexData, indices, vertexDesc);

    //创建一个Shader作为模板 
    FShaderRef ourShader = std::make_shared<FShader>("shaders/simple_model_shader.vs", "shaders/simple_model_shader.fs");
    //ourShader = std::make_shared<FShader>("shaders/simple_model_deferred.vs", "shaders/simple_model_deferred.fs");

    FShaderRef ourShaderBasic = std::make_shared<FShader>("shaders/basic_shader.vs", "shaders/basic_shader.fs");
    ourShaderBasic->setSwitch("Test1", true);

    FShaderRef ourShaderBasicToRenderCubemap = std::make_shared<FShader>("shaders/basic_shader.vs", "shaders/texture_to_cube.fs");
    ourShaderBasicToRenderCubemap->SetCullMethod(ECullMethod::CM_None);

    FTextureRef envTex = std::make_shared<FTexture>("objects/rusted_iron/EpicQuadPanorama_CC+EV1.HDR", ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_Linear);
    ourShaderBasicToRenderCubemap->SetTexture("equirectangularMap", envTex);
    FCubeTextureRef cubeTexture = std::make_shared<FCubeTexture>(512, ETexturePixelFormat::TPF_RGBA16F);
    cubeTexture->CaptureData(ourShaderBasicToRenderCubemap);

    //加载俩贴图
    FTextureRef tex[3];
    tex[0] = std::make_shared<FTexture>(("./container.jpg"), ETextureWarpMethod::TWM_Repeat, ETextureFilterMethod::TFM_TriLinear);
    tex[1] = std::make_shared<FTexture>(("./awesomeface.png"), ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_TriLinear);

     
    //给场景添加一个相机
    auto cameraComp = scene->CreateComponentWithArg<FCameraComponent>(glm::vec3(0.0f, 0.0f, 3.0f));
    cameraComp->AddSubobjectWithArgs<FSimpleImputMoveComponentSubobject>(0);//给相机添加个输入移动组件
    cameraComp->bDeferredPipeline = false;
    cameraComp->TextureEnv = cubeTexture;
    //方便起见将刚才的framebuffer的color贴图也放到这个数组里
    tex[2] = tex[0];//frameBuffer->Color[0];


      
     
    ////创建一个渲染组件，作为场景的一个地面
    //auto groundComponent = scene->CreateComponent<FPrimitiveComponent>();
    //groundComponent->AddPrimitiveUnit(ground, std::make_shared<FShader>(*ourShader));//地面用那个大一点的平面模型，方便起见用ourShader作为模板复制一份出来

    //groundComponent->GetShader(0)->SetTexture("Emissive", FTexture::GetBlack());
    //groundComponent->GetShader(0)->SetTexture("Albedo", tex[0]);
    //groundComponent->GetShader(0)->SetTexture("Specular", FTexture::GetWhite());
    //groundComponent->GetShader(0)->SetTexture("Roughness", FTexture::GetWhite());
    //groundComponent->GetShader(0)->SetTexture("Metallic", FTexture::GetWhite());
    //groundComponent->GetShader(0)->SetTexture("AO", FTexture::GetWhite());
    // 
    //创建一个场景组件，并作为相机的子物体
    auto cameraFollower = scene->CreateComponent<FPrimitiveComponent>();
    cameraFollower->AddPrimitiveUnit(cube, std::make_shared<FShader>(*ourShader));//用立方体模型，方便起见用ourShader作为模板复制一份出来
    cameraFollower->GetShader(0)->SetTexture("Emissive", tex[1]);
    cameraFollower->GetShader(0)->SetTexture("Albedo", tex[0]);
    cameraFollower->GetShader(0)->SetTexture("Specular", FTexture::GetWhite());
    cameraFollower->GetShader(0)->SetTexture("Roughness", FTexture::GetWhite());
    cameraFollower->GetShader(0)->SetTexture("Metallic", FTexture::GetWhite());
    cameraFollower->GetShader(0)->SetTexture("AO", FTexture::GetWhite());
	//cameraFollower->AttachTo(cameraComp, EAttachRule::AR_KeepRelative);
    cameraFollower->SetLocalLocation(glm::vec3(0, 0, -3));  

    /*auto cameraFollowPointLight = scene->CreateComponentWithArg<FPointLightComponent>(glm::vec3(1,0,0), glm::vec3(0), glm::vec3(-0.03,0,1), 6.0f);
    cameraFollowPointLight->AttachTo(cameraComp, EAttachRule::AR_SnapToTarget);
    cameraFollowPointLight->SetLocalLocation(glm::vec3(-1, 0, 0));

    auto PointLight = scene->CreateComponentWithArg<FPointLightComponent>(glm::vec3(0, 1, 0), glm::vec3(0), glm::vec3(-0.03, 0, 1), 6.0f);
    PointLight->AttachTo(cameraComp, EAttachRule::AR_SnapToTarget);
    PointLight->SetLocalLocation(glm::vec3(1, 0, 0));*/

    auto PointLight2 = scene->CreateComponentWithArg<FPointLightComponent>(glm::vec3(0, 0, 0.5), glm::vec3(0,0,1.2f), glm::vec3(-0.03, 0, 1), 3.0f);

    FTextureRef pbrtex[5];
    pbrtex[0] = std::make_shared<FTexture>("./objects/backpack/diffuse.jpg", ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_TriLinear);
    pbrtex[1] = std::make_shared<FTexture>("./objects/backpack/normal.png", ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_TriLinear);
    pbrtex[2] = std::make_shared<FTexture>("./objects/backpack/roughness.jpg", ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_TriLinear);
    pbrtex[3] = std::make_shared<FTexture>("./objects/backpack/specular.jpg", ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_TriLinear);
    pbrtex[4] = std::make_shared<FTexture>("./objects/backpack/ao.jpg", ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_TriLinear);

    auto&& envLight = scene->CreateComponentWithArg<FEnvLightComponent>(glm::vec3(0.2f, 0.2f, 0.2f));
    envLight->originEnvLight = cubeTexture;

    envLight->cookedEnvLight = std::make_shared<FCubeTexture>(64, ETexturePixelFormat::TPF_RGBA16F);
    envLight->cookedSpecPrefilterLight = std::make_shared<FCubeTexture>(256, ETexturePixelFormat::TPF_RGBA16F, true);
	envLight->CookEnvLight();
    
     
    glm::quat dirLightDir = glm::angleAxis(glm::radians(60.0f), glm::vec3(0, 1, 0)) * glm::angleAxis(glm::radians(-30.0f), glm::vec3(1, 0, 0));
    scene->CreateComponentWithArg<FDirectionalLightComponent>(glm::vec3(1.f, 1.0f, 1.0f), dirLightDir)->AddSubobject<FTestLightControlSubobject>();



    //auto&& guitaComponent = scene->CreateComponent<FPrimitiveComponent>();
    //guitaComponent->SetWorldTransform(glm::mat4(1));
    //for(auto&& mod : loadedModel.meshes)
    //{  
    //    auto tempShader = std::make_shared<FShader>(*ourShader2); 
    //    guitaComponent->AddPrimitiveUnit(mod.primitive, tempShader);
    //    tempShader->SetTexture("Emissive", FTexture::GetBlack());
    //    tempShader->SetTexture("Albedo", pbrtex[0]);
    //    tempShader->SetTexture("Specular", pbrtex[3]);
    //    tempShader->SetTexture("Roughness", pbrtex[2]);
    //    tempShader->SetTexture("NormalMap", pbrtex[1]);
    //    tempShader->SetTexture("Metallic", FTexture::GetWhite());
    //    tempShader->SetTexture("AO", pbrtex[4]);
    //}

    std::vector<char> sphereData;
    std::vector<unsigned int>  sphereIndices;
    FPrimitiveVertexDesc shpereDesc;
    GenerateSphereModel(1, 64, 64, sphereData, sphereIndices, shpereDesc);
    FShaderRef sphereShader = std::make_shared<FShader>("shaders/simple_model_deferred.vs", "shaders/simple_model_deferred.fs");
    sphereShader = std::make_shared<FShader>("shaders/simple_model_shader.vs", "shaders/simple_model_shader.fs");
    FPrimitiveRef spherePrimitive = std::make_shared<FPrimitive>();
    spherePrimitive->SetData(sphereData, sphereIndices, shpereDesc);
    spherePrimitive->Bound.begin = glm::vec3(2, 2, 2);
    spherePrimitive->Bound.end = glm::vec3(-2, -2, -2);
    FTextureRef sphereAlbedo = std::make_shared<FTexture>("./objects/rusted_iron/albedo.png",ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_Linear);
    FTextureRef sphereAO = std::make_shared<FTexture>("./objects/rusted_iron/ao.png",ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_Linear);
    FTextureRef sphereMetal = std::make_shared<FTexture>("./objects/rusted_iron/metallic.png",ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_Linear);
    FTextureRef sphereNormal = std::make_shared<FTexture>("./objects/rusted_iron/normal.png",ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_Linear);
    FTextureRef sphereRough = std::make_shared<FTexture>("./objects/rusted_iron/roughness.png",ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_Linear);

    //再随便给场景添加10个渲染组件
    for (int i = 0; i < 20; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-(i % 16) % 4, i / 16, (i % 16) / 4) * 2.0f);
        float angle = 20.0f * i;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

        auto primitive = scene->CreateComponent<FPrimitiveComponent>();

        primitive->SetWorldTransform(model);

        //为了方便展示，不同的渲染组件交替使用两个模型     
        //if (i % 2)
        {
            primitive->AddPrimitiveUnit(spherePrimitive, std::make_shared<FShader>(*sphereShader));
        }
        //else
        //{ 
        //    primitive->AddPrimitiveUnit(plane, std::make_shared<FShader>(*ourShader));
        //    primitive->GetShader(0)->SetCullMethod(ECullMethod::CM_None);//平面模型不剔除，渲染双面
        //}
        primitive->GetShader(0)->SetTexture("Emissive", FTexture::GetBlack());
        primitive->GetShader(0)->SetTexture("Albedo", sphereAlbedo); 
        primitive->GetShader(0)->SetTexture("Specular", FTexture::GetWhite());
        primitive->GetShader(0)->SetTexture("Roughness", sphereRough);
        primitive->GetShader(0)->SetTexture("Metallic", sphereMetal);
        //primitive->GetShader(0)->SetTexture("Metallic", FTexture::GetBlack());
        primitive->GetShader(0)->SetTexture("AO", sphereAO);
        primitive->GetShader(0)->SetTexture("NormalMap", sphereNormal);
        primitive->GetShader(0)->setSwitch("USE_NORMAL_MAP",true);
        primitive->GetShader(0)->setPrimitiveMethod(EPrimitiveMethod::PM_TriangleStrip);

    }


    //for (unsigned int i = 0; i < loadedModel.meshes.size(); i++)
    //{
    //    glm::mat4 model = glm::mat4(1.0f);
    //    model = glm::translate(model, glm::vec3((i % 16) % 4,i / 16,(i % 16)/4) * 2.0f);
    //    float angle = 20.0f * i;
    //    model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

    //    auto primitive = scene->CreateComponent<FPrimitiveComponent>();
    //    
    //    primitive->SetWorldTransform(model);

    //    //为了方便展示，不同的渲染组件交替使用两个模型     
    //    //if (i % 2)
    //    {
    //        primitive->AddPrimitiveUnit(loadedModel.meshes[i].primitive, std::make_shared<FShader>(*ourShader2));
    //    }
    //    //else
    //    //{ 
    //    //    primitive->AddPrimitiveUnit(plane, std::make_shared<FShader>(*ourShader));
    //    //    primitive->GetShader(0)->SetCullMethod(ECullMethod::CM_None);//平面模型不剔除，渲染双面
    //    //}
    //    primitive->GetShader(0)->SetTexture("Emissive", FTexture::GetBlack());
    //    primitive->GetShader(0)->SetTexture("Albedo", pbrtex[0]);
    //    primitive->GetShader(0)->SetTexture("Specular", FTexture::GetBlack());
    //    primitive->GetShader(0)->SetTexture("Roughness", FTexture::GetWhite());
    //    primitive->GetShader(0)->SetTexture("Metallic", FTexture::GetWhite());
    //    primitive->GetShader(0)->SetTexture("AO", pbrtex[4]);
    //    

    //} 



    std::vector<char> planeWithNormalVertexData;
    planeWithNormalVertexData.resize(sizeof(planeWithNormal));
    memcpy(planeWithNormalVertexData.data(), planeWithNormal, planeWithNormalVertexData.size());
    FPrimitiveVertexDesc planeWithNormalVertexDesc;
    planeWithNormalVertexDesc.structSize = 6 * sizeof(float);
    planeWithNormalVertexDesc.props.emplace_back(0, reinterpret_cast<void*>(0), GL_FLOAT, 3);//position
    planeWithNormalVertexDesc.props.emplace_back(1, reinterpret_cast<void*>(3 * sizeof(float)), GL_FLOAT, 3);//Normal

    
    FPrimitiveRef planeWithNormalPrim = std::make_shared<FPrimitive>();
    planeWithNormalPrim->SetData(planeWithNormalVertexData, indices, planeWithNormalVertexDesc);
     

    //auto basicShaderCubeComponent = scene->CreateComponent<FPrimitiveComponent>();
    //basicShaderCubeComponent->AddPrimitiveUnit(planeWithNormalPrim, std::make_shared<FShader>(*ourShaderBasic));
    //basicShaderCubeComponent->SetWorldLocation(glm::vec3(1, 5, -1));
    //basicShaderCubeComponent->GetShader(0)->setVec4("InputColor", glm::vec4(0, 1, 0, 1));
    //basicShaderCubeComponent->GetShader(0)->SetTexture("TestColor", FTexture::GetWhite());
    //basicShaderCubeComponent->GetShader(0)->setSwitch("Test1", true);
    
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        glfwPollEvents();
    	FInputReceiver::GetInputReceiver().deltaTime = deltaTime;
        FInputReceiver::GetInputReceiver().Execute();


        // game logic
        // ----------
        scene->Tick(deltaTime);

        FInputReceiver::GetInputReceiver().FinalExecute();
        FInputReceiver::GetInputReceiver().frameIndex++;
        // render
        // ------
        FCameraComponent::GetDeferredCmds().Execute();

        glfwSwapBuffers(window);
        
    }

    glfwTerminate();
    return 0;
}


void processInput(GLFWwindow* window)
{

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
    {
        glfwSetWindowShouldClose(window, true);
    }

    if(action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        std::get<1>(FInputReceiver::GetInputReceiver().keyStatus[key]) = (action == GLFW_PRESS);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    FInputReceiver::GetInputReceiver().mousePos[2] = xposIn;
    FInputReceiver::GetInputReceiver().mousePos[3] = yposIn;
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    //cameraComp.lock()->ProcessMouseScroll(static_cast<float>(yoffset));
}
