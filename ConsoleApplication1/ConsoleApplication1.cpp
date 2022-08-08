#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "shader_m2.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

FCameraRef cameraComp;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    glEnable(GL_DEPTH_TEST);


    FSceneRef scene = std::make_shared<FScene>();

    std::vector<char> vertexData;
    vertexData.resize(sizeof(vertices));
    memcpy(vertexData.data(), vertices, vertexData.size());
    FPrimitiveVertexDesc vertexDesc;
    vertexDesc.structSize = 5 * sizeof(int);
    vertexDesc.props.emplace_back(0, reinterpret_cast<void*>(0), GL_FLOAT, 3);
    vertexDesc.props.emplace_back(1, reinterpret_cast<void*>(3 * sizeof(float)), GL_FLOAT, 2);

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
    FShaderRef ourShader = std::make_shared<FShader>("simple_shader.vs", "simple_shader.fs");

    //加载俩贴图
    FTextureRef tex[2];
    tex[0] = std::make_shared<FTexture>(("./container.jpg"), ETextureWarpMethod::TWM_Repeat, ETextureFilterMethod::TFM_TriLinear);
    tex[1] = std::make_shared<FTexture>(("./awesomeface.png"), ETextureWarpMethod::TWM_Clamp, ETextureFilterMethod::TFM_TriLinear);

    //给场景添加一个相机
    cameraComp = scene->CreateComponentWithArg<FCameraComponent>(glm::vec3(0.0f, 0.0f, 3.0f));

    //创建一个渲染组件，作为场景的一个地面
    auto groundComponent = scene->CreateComponent<FPrimitiveComponent>();
    groundComponent->Primitive = ground;//地面用那个大一点的平面模型
    groundComponent->SetWorldTransform(glm::mat4(1.0f));
    groundComponent->Shader = std::make_shared<FShader>(*ourShader);//方便起见用ourShader作为模板复制一份出来
    groundComponent->Shader->SetTexture("texture1", tex[0]);
    groundComponent->Shader->SetTexture("texture2", tex[1]);
    groundComponent->Shader->setVec4("uniColor", 1, 1, 1, 1);

    //创建一个场景组件，并作为相机的子物体
    auto cameraFollower = scene->CreateComponent<FPrimitiveComponent>();
    cameraFollower->Primitive = cube;//用立方体模型
    cameraFollower->Shader = std::make_shared<FShader>(*ourShader);//方便起见用ourShader作为模板复制一份出来
    cameraFollower->Shader->SetTexture("texture1", tex[0]);
    cameraFollower->Shader->SetTexture("texture2", tex[1]);
    cameraFollower->Shader->setVec4("uniColor", 1, 1, 1, 1);
    cameraFollower->AttachTo(cameraComp, EAttachRule::AR_KeepRelative);
    cameraFollower->SetLocalLocation(glm::vec3(1, 1, -3));


    //再随便给场景添加10个渲染组件
    for (unsigned int i = 0; i < 10; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePositions[i]);
        float angle = 20.0f * i;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

        auto primitive = scene->CreateComponent<FPrimitiveComponent>();
        primitive->Shader = std::make_shared<FShader>(*ourShader); //为了方便区别不同的渲染组件，每个渲染组件都使用自己独立的shader，方便起见用ourShader作为模板复制一份出来;
        primitive->SetWorldTransform(model);

        //为了方便展示，不同的渲染组件交替使用两个模型    
        if (i % 2)
        {
            primitive->Primitive = cube;
        }
        else
        {
            primitive->Primitive = plane;
            primitive->Shader->SetCullMethod(ECullMethod::CM_None);//平面模型不剔除，渲染双面
        }

        //给每个渲染组件的shader设置不同的参数，方便看效果
        primitive->Shader->setVec4("uniColor", i / 10.0f, 1.0f - i / 10.0f, i / 10.0f, 1);
        primitive->Shader->SetTexture("texture1", tex[i % 2]);
        primitive->Shader->SetTexture("texture2", tex[!(i % 2)]);

    }


    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // game logic
        // ----------
        scene->Tick(deltaTime);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cameraComp->Draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraComp->ProcessKeyboard(ECameraMovement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraComp->ProcessKeyboard(ECameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraComp->ProcessKeyboard(ECameraMovement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraComp->ProcessKeyboard(ECameraMovement::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraComp->ProcessKeyboard(ECameraMovement::Up, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraComp->ProcessKeyboard(ECameraMovement::Down, deltaTime);
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    cameraComp->ProcessMouseMovement(xoffset, yoffset);
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraComp->ProcessMouseScroll(static_cast<float>(yoffset));
}
