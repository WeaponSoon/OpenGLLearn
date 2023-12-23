#ifndef SHADER_H
#define SHADER_H


#include <unordered_map>
#include <regex>

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

enum class EPrimitiveMethod
{
    PM_Triangles = GL_TRIANGLES,
    PM_TriangleStrip = GL_TRIANGLE_STRIP
};

enum class EDepthRightStatus
{
    DWE_Enable,
    DWE_Disable
};

struct FShaderVariantKey
{
    std::vector<uint8_t> Keys;

    size_t size = 0 ;



    void Add(int toAdd)
    {
        size+=toAdd;
        while(size > Keys.size() * 8)
        {
            Keys.push_back(0);
        }
    }

    bool operator[](size_t pos) const
    {
        return !!(Keys[pos / 8] & (((uint8_t)1) << (pos % 8u)));
    }

    void Set(size_t pos, bool value)
    {
        if(pos >= size)
        {
            return;
        }

        uint8_t bucket = Keys[pos / 8];
        if(value)
        {
            bucket |= (((uint8_t)1) << (pos % 8u));
        }
        else
        {
	        bucket &= ~(((uint8_t)1) << (pos % 8u));
        }
        Keys[pos / 8] = bucket;
    }

    bool operator==(const FShaderVariantKey& other) const
    {
        return size == other.size && Keys == other.Keys;
    }

};

template<>
struct std::hash<FShaderVariantKey>
{
	inline void CombineHash(size_t& Seed, size_t Hash) const noexcept
	{
		Seed ^= Hash + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
	}
	size_t operator()(const FShaderVariantKey& v) const noexcept {
		size_t HashIndex = std::hash<size_t>()(v.size);
		for(auto C : v.Keys)
		{
			CombineHash(HashIndex, hash<uint8_t>()(C));
		}
		return HashIndex;
	}
};

class FShader
{




    class FInternalShader
    {
        

    public:
        bool bValid = false;
        std::string vertexCode;
        std::string fragmentCode;
        std::unordered_map<FShaderVariantKey,unsigned int> vertexShaders;
        std::unordered_map<FShaderVariantKey,unsigned int> fragmentShaders;

        std::unordered_map<std::string, size_t> switchNameToPos;

        std::string glslVersion = "330";

        FInternalShader() = default;
        FInternalShader(const FInternalShader&) = delete;
        FInternalShader(FInternalShader&&) = delete;
        FInternalShader& operator=(FInternalShader&) = delete;
        ~FInternalShader()
        {
            for(auto&& vertexShader : vertexShaders)
            {
                glDeleteShader(vertexShader.second);
            }
            for(auto&& fragmentShader : fragmentShaders )
            {
                glDeleteShader(fragmentShader.second);
            }
        }

        FShaderVariantKey StatusToID(const std::map<std::string, bool>& InStatus)
        {
            FShaderVariantKey InKey;
            InKey.Add(switchNameToPos.size());

            for (auto&& item : InStatus)
            {
                auto&& itrr = switchNameToPos.find(item.first);
                if (itrr != switchNameToPos.end())
                {
                    InKey.Set(itrr->second, item.second);
                }
            }
            return InKey;
        }

        bool CompileShaderAtStatus(const std::map<std::string, bool>& InStatus)
        {
            FShaderVariantKey InKey = StatusToID(InStatus);
           

            {
                auto&& res = vertexShaders.emplace(InKey, GL_NONE);
                if (res.second || res.first->second == GL_NONE)
                {
                    // vertex shader
                    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
                    std::string defs;
                    defs += "#version ";
                    defs += glslVersion;
                    defs += " core\n";
                    for (auto&& item : switchNameToPos)
                    {
                        defs += "#define ";
                        defs += item.first;

                        auto&& Ins = InStatus.find(item.first);
                        if (Ins != InStatus.end())
                        {
                            defs += (Ins->second ? " 1\n" : " 0\n");
                        }
                        else
                        {
                            defs += " 0\n";
                        }
                        
                    }
                    defs += "\n";
                    const char* vShaderCode[2] = { defs.c_str(),vertexCode.c_str() };

                    glShaderSource(vertexShader, 2, vShaderCode, NULL);
                    glCompileShader(vertexShader);

                    if (!checkCompileErrors(vertexShader, "VERTEX"))
                    {
                        glDeleteShader(vertexShader);
                        return false;
                    }

                    res.first->second = vertexShader;
                }
            }
            {
                auto&& res = fragmentShaders.emplace(InKey, GL_NONE);
                if (res.second || res.first->second == GL_NONE)
                {
                    // vertex shader
                    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
                    std::string defs;
                    defs += "#version ";
                    defs += glslVersion;
                    defs += " core\n";
                    for (auto&& item : switchNameToPos)
                    {
                        defs += "#define ";
                        defs += item.first;
                        auto&& Ins = InStatus.find(item.first);
                        if (Ins != InStatus.end())
                        {
                            defs += (Ins->second ? " 1\n" : " 0\n");
                        }
                        else
                        {
                            defs += " 0\n";
                        }
                    }
                    defs += "\n";
                    const char* vShaderCode[2] = { defs.c_str(),fragmentCode.c_str() };

                    glShaderSource(fragmentShader, 2, vShaderCode, NULL);
                    glCompileShader(fragmentShader);

                    if (!checkCompileErrors(fragmentShader, "VERTEX"))
                    {
                        glDeleteShader(fragmentShader);
                        return false;
                    }
                    res.first->second = fragmentShader;
                }
            }
            return true;
        }

        void InitFromFile(const char* vertexPath, const char* fragmentPath)
        {

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

            std::vector<std::string> tokenLines;

            {
                int tti = 0; 
                
                //std::string ct("\\/\\*<.*?>");
                std::regex rg("\\/\\*<.*?>");

                for(std::sregex_iterator It(vertexCode.begin(), vertexCode.end(), rg),end; It != end; ++It)
                {
                    tokenLines.push_back(It->str(0));
                }
                for (std::sregex_iterator It(fragmentCode.begin(), fragmentCode.end(), rg), end; It != end; ++It)
                {
                    tokenLines.push_back(It->str(0));
                }
                size_t curPos = 0;
                std::regex rg_switch("Switch=[a-zA-Z]\\w*(,|$)");
                std::regex rg_version("Version=\\d+(,|$)");
                for(auto&& cokl : tokenLines)
                {
                    for (std::sregex_iterator It(cokl.begin()+3, cokl.end()-1, rg_switch), end; It != end; ++It)
                    {
                        std::string ktm = It->str(0);

                        std::string ktmf(ktm.begin() + 7, ktm[ktm.size() - 1] == ',' ? ktm.end() - 1 : ktm.end());

                        auto&&r = switchNameToPos.emplace(ktmf,0ul);
                        if(r.second)
                        {
                            r.first->second = curPos;
                            curPos++;
                        }
                    }
                    for (std::sregex_iterator It(cokl.begin() + 3, cokl.end() - 1, rg_version), end; It != end; ++It)
                    {
                        std::string ktm = It->str(0);

                        std::string ktmf(ktm.begin() + 8, ktm[ktm.size() - 1] == ',' ? ktm.end() - 1 : ktm.end());

                        glslVersion = ktmf;
                    }
                }
                

            }
            


            const char* vShaderCode = vertexCode.c_str();
            const char* fShaderCode = fragmentCode.c_str();
            bValid = CompileShaderAtStatus(std::map<std::string, bool>());
			
            //bValid = true;
        }
    };

    friend class FCameraComponent;
    friend class FRenderBatch;

    struct TextureMark
    {
        int slot = -1;
        GLenum textureType = GL_TEXTURE_2D;
        ITextureRef texture;
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
    EPrimitiveMethod PrimitiveMethod = EPrimitiveMethod::PM_Triangles;
    EDepthRightStatus DepthWrite = EDepthRightStatus::DWE_Enable;
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

    void ApplyDepthWrite() const
    {
	    switch (DepthWrite)
	    {
	    case EDepthRightStatus::DWE_Enable:
            glDepthMask(GL_TRUE);
            break;
        case EDepthRightStatus::DWE_Disable:
            glDepthMask(GL_FALSE);
            break;
	    }
    }

    bool IsUsing() const
    {
        GLint currentProgram = GL_NONE;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        return CurrentID != GL_NONE && CurrentID == currentProgram;
    }

    // activate the shader
    // ------------------------------------------------------------------------
    void use() const
    {
        glUseProgram(CurrentID);

        ApplyCullMethod();
        ApplyBlendMethod();
        ApplyDepthWrite();
        {
            for (auto&& pair : textureMap)
            {
                if (pair.second.slot >= 0 && pair.second.texture->IsValid())
                {
                    GLint Loc = glGetUniformLocation(CurrentID, pair.first.c_str());
                    if(Loc >= 0)
                    {
                        glActiveTexture(GL_TEXTURE0 + pair.second.slot);
                        glBindTexture(pair.second.textureType, pair.second.texture->ID);
                        glUniform1i(Loc, pair.second.slot);
                    }
                }
            }
        }
        

        for (auto&& pair : boolMap)
        {
            GLint Loc = glGetUniformLocation(CurrentID, pair.first.c_str());
            if(Loc < 0)
            {
                continue;
            }
            glUniform1i(Loc, static_cast<int>(pair.second));
            
        }

        for (auto&& pair : intMap)
        {
            GLint Loc = glGetUniformLocation(CurrentID, pair.first.c_str());
            if (Loc < 0)
            {
                continue;
            }
            glUniform1i(glGetUniformLocation(CurrentID, pair.first.c_str()), pair.second);
        }

        for (auto&& pair : floatMap)
        {
            GLint Loc = glGetUniformLocation(CurrentID, pair.first.c_str());
            if (Loc < 0)
            {
                continue;
            }
            glUniform1f(glGetUniformLocation(CurrentID, pair.first.c_str()), pair.second);
        }

        for (auto&& pair : vec2Map)
        {
            GLint Loc = glGetUniformLocation(CurrentID, pair.first.c_str());
            if (Loc < 0)
            {
                continue;
            }
            glUniform2fv(glGetUniformLocation(CurrentID, pair.first.c_str()), 1, &pair.second[0]);
        }

        for (auto&& pair : vec3Map)
        {
            GLint Loc = glGetUniformLocation(CurrentID, pair.first.c_str());
            if (Loc < 0)
            {
                continue;
            }
            glUniform3fv(glGetUniformLocation(CurrentID, pair.first.c_str()), 1, &pair.second[0]);
        }

        for (auto&& pair : vec4Map)
        {
            GLint Loc = glGetUniformLocation(CurrentID, pair.first.c_str());
            if (Loc < 0)
            {
                continue;
            }
            glUniform4fv(glGetUniformLocation(CurrentID, pair.first.c_str()), 1, &pair.second[0]);
        }

        for (auto&& pair : mat2Map)
        {
            GLint Loc = glGetUniformLocation(CurrentID, pair.first.c_str());
            if (Loc < 0)
            {
                continue;
            }
            glUniformMatrix2fv(glGetUniformLocation(CurrentID, pair.first.c_str()), 1, GL_FALSE, &pair.second[0][0]);
        }

        for (auto&& pair : mat3Map)
        {
            GLint Loc = glGetUniformLocation(CurrentID, pair.first.c_str());
            if (Loc < 0)
            {
                continue;
            }
            glUniformMatrix3fv(glGetUniformLocation(CurrentID, pair.first.c_str()), 1, GL_FALSE, &pair.second[0][0]);
        }

        for (auto&& pair : mat4Map)
        {
            GLint Loc = glGetUniformLocation(CurrentID, pair.first.c_str());
            if (Loc < 0)
            {
                continue;
            }
            glUniformMatrix4fv(glGetUniformLocation(CurrentID, pair.first.c_str()), 1, GL_FALSE, &pair.second[0][0]);
        }
    }

    std::shared_ptr<FInternalShader> internalShader;
    unsigned int CurrentID;
    std::unordered_map<FShaderVariantKey, unsigned int> IDs;
    
    std::map<std::string, bool> currentStatus;





    void SwitchShader()
    {
	    if(internalShader->bValid)
	    {
            auto&& Key = internalShader->StatusToID(currentStatus);
            auto&& r = IDs.emplace(Key, GL_NONE);
            if(r.second)
            {
                bool suc = internalShader->CompileShaderAtStatus(currentStatus);
                if(suc)
                {
                    GLuint ID = glCreateProgram();
                    glAttachShader(ID, internalShader->vertexShaders[Key]);
                    glAttachShader(ID, internalShader->fragmentShaders[Key]);
                    glLinkProgram(ID);
                    if (!checkCompileErrors(ID, "PROGRAM"))
                    {
                        glDeleteProgram(ID);
                        return;
                    }
                    r.first->second = ID;
                    CurrentID = ID;
                }
            }
            else
            {
                CurrentID = r.first->second;
            }
	    }
    }

public:

    void setDepthWriteEnable(EDepthRightStatus InD)
    {
        DepthWrite = InD;
    }

    void setPrimitiveMethod(EPrimitiveMethod inM)
    {
        PrimitiveMethod = inM;
    }
    EPrimitiveMethod getPrimitiveMetohd() const { return PrimitiveMethod; }
    GLuint GetID() const
    {
        return CurrentID;
    }

    void setSwitch(const std::string& switchName, bool value)
    {
        currentStatus[switchName] = value;
        SwitchShader();
    }
    bool getSwitch(const std::string& switchName ) const
    {
        auto&& iter = currentStatus.find(switchName);
        if(iter == currentStatus.end())
        {
            return false;
        }
        return iter->second;
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
		CurrentID(GL_NONE)
        
    {
        SwitchShader();
        currentStatus = otherShader.currentStatus;
        SwitchShader();
    }

    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    FShader(const char* vertexPath, const char* fragmentPath) : internalShader(std::make_shared<FInternalShader>()), CurrentID(GL_NONE)
    {

        internalShader->InitFromFile(vertexPath, fragmentPath);
        if(internalShader->bValid)
        {
            SwitchShader();
        }
        // shader Program
    }

    

    virtual ~FShader()
    {
        if (IsUsing())
        {
            glUseProgram(GL_NONE);
        }
        for (auto&& ID : IDs)
        {
            glDeleteProgram(ID.second);
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
        boolMap[name] = value;
        return true;
        
    }
    // ------------------------------------------------------------------------
    bool setInt(const std::string &name, int value) 
    {
        intMap[name] = value;
        return true;
    }
    // ------------------------------------------------------------------------
    bool setFloat(const std::string &name, float value) 
    {
        floatMap[name] = value;
        return true;
    }
    // ------------------------------------------------------------------------
    bool setVec2(const std::string &name, const glm::vec2 &value) 
    {
        vec2Map[name] = value;
        return true;
    }
    bool setVec2(const std::string &name, float x, float y)
    { 
        return setVec2(name, glm::vec2(x, y));
    }
    // ------------------------------------------------------------------------
    bool setVec3(const std::string &name, const glm::vec3 &value) 
    {
        vec3Map[name] = value;
        return true;
    }
    bool setVec3(const std::string &name, float x, float y, float z)
    { 
        return setVec3(name, glm::vec3(x, y, z));
    }
    // ------------------------------------------------------------------------
    bool setVec4(const std::string &name, const glm::vec4 &value)
    {
        vec4Map[name] = value;
        return true;

        
    }
    bool setVec4(const std::string &name, float x, float y, float z, float w)
    { 
        return setVec4(name, glm::vec4(x, y, z, w));
    }
    // ------------------------------------------------------------------------
    bool setMat2(const std::string &name, const glm::mat2 &mat)
    {
        mat2Map[name] = mat;
        return true;

        
    }
    // ------------------------------------------------------------------------
    bool setMat3(const std::string &name, const glm::mat3 &mat) 
    {
        mat3Map[name] = mat;
        return true;
    }
    // ------------------------------------------------------------------------
    bool setMat4(const std::string &name, const glm::mat4 &mat)
    {
        mat4Map[name] = mat;
        return true;
    }

    bool SetTexture(const std::string& name, const FTextureRef& inTexture)
    {
        auto&& textureStruct = textureMap[name];
        if (textureStruct.slot < 0)
        {
            textureStruct.slot = ++textureSlot;
        }
        textureStruct.texture = inTexture;

        return true;
        
    }

    bool SetTextureCube(const std::string& name, const FCubeTextureRef& inTexture)
    {
        auto&& textureStruct = textureMap[name];
        if (textureStruct.slot < 0)
        {
            textureStruct.textureType = GL_TEXTURE_CUBE_MAP;
            textureStruct.slot = ++textureSlot;
        }
        textureStruct.texture = inTexture;

        return true;

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
