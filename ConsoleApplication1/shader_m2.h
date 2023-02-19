#ifndef SHADER_H
#define SHADER_H



#include "core_m2.h"
#include "texture_m2.h"
#include "scene_m2.h"

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods

enum class ECullMethod
{
    CM_None,
    CM_CullFront,
    CM_CullBack,
    CM_CullFrontAndBack
};

enum class EBlendMethod
{
	BM_NoBlend,
    BM_Translucent,
    BM_Additive,
};

class FShader
{
    class FInternalShader
    {
        

    public:
        bool bValid = false;
        unsigned int vertexShader = GL_NONE;
        unsigned int fragmentShader = GL_NONE;
        FInternalShader() = default;
        FInternalShader(const FInternalShader&) = delete;
        FInternalShader(FInternalShader&&) = delete;
        FInternalShader& operator=(FInternalShader&) = delete;
        ~FInternalShader()
        {
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
        }
        void InitFromFile(const char* vertexPath, const char* fragmentPath)
        {
            std::string vertexCode;
            std::string fragmentCode;
            std::ifstream vShaderFile;
            std::ifstream fShaderFile;
            // ensure ifstream objects can throw exceptions:
            vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
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
                return;
            }
            const char* vShaderCode = vertexCode.c_str();
            const char* fShaderCode = fragmentCode.c_str();
            // 2. compile shaders
            
            // vertex shader
            vertexShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertexShader, 1, &vShaderCode, NULL);
            glCompileShader(vertexShader);

            if(!checkCompileErrors(vertexShader, "VERTEX"))
            {
                glDeleteShader(vertexShader);
                vertexShader = GL_NONE;
                return;
            }
            // fragment Shader
            fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
            glCompileShader(fragmentShader);
            if(!checkCompileErrors(fragmentShader, "FRAGMENT"))
            {
                glDeleteShader(fragmentShader);
                fragmentShader = GL_NONE;
                return;
            }
            bValid = true;
        }
    };

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
    EBlendMethod blendMothed = EBlendMethod::BM_NoBlend;


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

    void ApplyBlendMethod() const
    {
	    switch (blendMothed)
	    {
	    case EBlendMethod::BM_Additive:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
            break;
	    case EBlendMethod::BM_Translucent:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
	    case EBlendMethod::BM_NoBlend:
            glDisable(GL_BLEND);
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
        ApplyBlendMethod();
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

    std::shared_ptr<FInternalShader> internalShader;

public:
    unsigned int ID;

    GLuint GetID() const
    {
        return ID;
    }

    

    FShader(const FShader& otherShader) : textureSlot(otherShader.textureSlot),
        textureMap(otherShader.textureMap),
		boolMap(otherShader.boolMap),
		intMap(otherShader.intMap),
		floatMap(otherShader.floatMap),
		vec2Map(otherShader.vec2Map),
		vec3Map(otherShader.vec3Map),
		vec4Map(otherShader.vec4Map),
		mat2Map(otherShader.mat2Map),
		mat3Map(otherShader.mat3Map),
		mat4Map(otherShader.mat4Map),
		internalShader(otherShader.internalShader),
		ID(GL_NONE)
    {
	    if(internalShader->bValid)
	    {
            ID = glCreateProgram();
            glAttachShader(ID, internalShader->vertexShader);
            glAttachShader(ID, internalShader->fragmentShader);
            glLinkProgram(ID);
            if (!checkCompileErrors(ID, "PROGRAM"))
            {
                glDeleteProgram(ID);
                ID = GL_NONE;
                return;
            }
	    }
    }

    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    FShader(const char* vertexPath, const char* fragmentPath) : internalShader(std::make_shared<FInternalShader>()), ID(GL_NONE)
    {

        internalShader->InitFromFile(vertexPath, fragmentPath);
        if(internalShader->bValid)
        {
            ID = glCreateProgram();
            glAttachShader(ID, internalShader->vertexShader);
            glAttachShader(ID, internalShader->fragmentShader);
            glLinkProgram(ID);
            if (!checkCompileErrors(ID, "PROGRAM"))
            {
                glDeleteProgram(ID);
                ID = GL_NONE;
                return;
            }
        }
        // shader Program
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

    void SetBlendMethod(EBlendMethod inMethod)
    {
        blendMothed = inMethod;
        if(IsUsing())
        {
            ApplyBlendMethod();
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
    bool static checkCompileErrors(GLuint shader, std::string type)
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
                return false;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
                return false;
            }
        }
        return true;
    }
};

using FShaderRef = std::shared_ptr<FShader>;




#endif
