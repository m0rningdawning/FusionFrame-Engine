#include "Model.hpp"
#include "Light.hpp"
#include "ShadowMaps.hpp"
#include "Animator.hpp"
#include <filesystem>
#include <glew.h>
#include <glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "stb_image.h"

unsigned int counter = 1;
std::unordered_map<unsigned int, FUSIONCORE::Model*> IDmodelMap;

FUSIONCORE::Model::Model()
{
    this->AnimationEnabled = false;
    FinalAnimationMatrices = nullptr;
    this->AlphaMaterial = nullptr;
    this->ModelID = counter;
    IDmodelMap[ModelID] = this;
    counter++;

    this->ObjectType = FF_OBJECT_TYPE_MODEL;

    this->InstanceDataVBO = nullptr;
    this->InstanceCount = 0;
}

FUSIONCORE::Model::Model(std::string const& filePath, bool Async, bool AnimationModel)
{
    this->AnimationEnabled = AnimationModel;
    FinalAnimationMatrices = nullptr;
    this->AlphaMaterial = nullptr;
    this->loadModel(filePath, Async, AnimationModel);
    FindGlobalMeshScales();
    this->ModelID = counter;
    IDmodelMap[ModelID] = this;
    counter++;

    this->InstanceDataVBO = nullptr;
    this->InstanceCount = 0;

    this->ObjectType = FF_OBJECT_TYPE_MODEL;
}

FUSIONCORE::Model::Model(Model& Other)
{
    this->Meshes.reserve(Other.Meshes.size());
    for (auto& mesh : Other.Meshes)
    {
        auto &newMesh = Meshes.emplace_back();
        newMesh.CopyMesh(mesh);
    }
    this->ObjectType = FF_OBJECT_TYPE_MODEL;
}

void FUSIONCORE::Model::Draw(Camera3D& camera, Shader& shader,const std::function<void()> &ShaderPreperations)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        FUSIONCORE::SendLightsShader(shader);
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        shader.setInt("OmniShadowMapCount", 0);
        //shader.setBool("EnableAnimation", false);
        ShaderPreperations();
    };

	for (size_t i = 0; i < Meshes.size(); i++)
	{
		Meshes[i].Draw(camera, shader, shaderPrep);
	}
}

void FUSIONCORE::Model::Draw(Camera3D& camera, Shader& shader, Material material,const std::function<void()>& ShaderPreperations)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        FUSIONCORE::SendLightsShader(shader);
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        shader.setInt("OmniShadowMapCount", 0);
        shader.setBool("EnableAnimation", false);
        ShaderPreperations();
    };

	for (size_t i = 0; i < Meshes.size(); i++)
	{
		Meshes[i].Draw(camera, shader,material, shaderPrep);
	}
}

void FUSIONCORE::Model::Draw(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, CubeMap& cubemap,Material material, float EnvironmentAmbientAmount)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        FUSIONCORE::SendLightsShader(shader);
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        shader.setInt("OmniShadowMapCount", 0);
        shader.setBool("EnableAnimation", false);
        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].Draw(camera, shader, shaderPrep, cubemap ,material, EnvironmentAmbientAmount);
    }
}

void FUSIONCORE::Model::Draw(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, CubeMap& cubemap, Material material, std::vector<OmniShadowMap*> ShadowMaps, float EnvironmentAmbientAmount)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        FUSIONCORE::SendLightsShader(shader);
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        shader.setInt("OmniShadowMapCount", ShadowMaps.size());
        for (size_t i = 0; i < ShadowMaps.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + 8 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ShadowMaps[i]->GetShadowMap());
            glUniform1i(glGetUniformLocation(shader.GetID(), ("OmniShadowMaps[" + std::to_string(i) + "]").c_str()), 8 + i);
            glUniform1f(glGetUniformLocation(shader.GetID(), ("ShadowMapFarPlane[" + std::to_string(i) + "]").c_str()), ShadowMaps[i]->GetFarPlane());
        }
        shader.setBool("EnableAnimation", false);
        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].Draw(camera, shader, shaderPrep, cubemap, material, EnvironmentAmbientAmount);
    }
}

void FUSIONCORE::Model::Draw(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, CubeMap& cubemap, Material material, std::vector<OmniShadowMap*> ShadowMaps, std::vector<glm::mat4>& AnimationBoneMatrices, float EnvironmentAmbientAmount)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        FUSIONCORE::SendLightsShader(shader);
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        shader.setInt("OmniShadowMapCount", ShadowMaps.size());
        for (size_t i = 0; i < ShadowMaps.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + 8 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ShadowMaps[i]->GetShadowMap());
            glUniform1i(glGetUniformLocation(shader.GetID(), ("OmniShadowMaps[" + std::to_string(i) + "]").c_str()), 8 + i);
            glUniform1f(glGetUniformLocation(shader.GetID(), ("ShadowMapFarPlane[" + std::to_string(i) + "]").c_str()), ShadowMaps[i]->GetFarPlane());
        }
        for (int i = 0; i < AnimationBoneMatrices.size(); ++i)
        {
            shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", AnimationBoneMatrices[i]);
        }
        shader.setBool("EnableAnimation", true);
        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].Draw(camera, shader, shaderPrep, cubemap, material, EnvironmentAmbientAmount);
    }
}

void FUSIONCORE::Model::DrawInstanced(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, CubeMap& cubemap, Material material,VBO &InstanceDataVBO,size_t InstanceCount,std::vector<OmniShadowMap*> ShadowMaps, float EnvironmentAmbientAmount)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        FUSIONCORE::SendLightsShader(shader);
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        shader.setInt("OmniShadowMapCount", ShadowMaps.size());
        for (size_t i = 0; i < ShadowMaps.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + 8 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ShadowMaps[i]->GetShadowMap());
            glUniform1i(glGetUniformLocation(shader.GetID(), ("OmniShadowMaps[" + std::to_string(i) + "]").c_str()), 8 + i);
            glUniform1f(glGetUniformLocation(shader.GetID(), ("ShadowMapFarPlane[" + std::to_string(i) + "]").c_str()), ShadowMaps[i]->GetFarPlane());
        }
        shader.setBool("EnableAnimation", false);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.GetConvDiffCubeMap());
        shader.setInt("ConvDiffCubeMap", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.GetPreFilteredEnvMap());
        shader.setInt("prefilteredMap", 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, FUSIONCORE::brdfLUT);
        shader.setInt("LUT", 3);

        shader.setBool("EnableIBL", true);
        shader.setFloat("ao", EnvironmentAmbientAmount);

        material.SetMaterialShader(shader);

        if (InstanceDataVBO.IsVBOchanged())
        {
            InstanceDataVBO.Bind();
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribDivisor(7, 1);
            InstanceDataVBO.SetVBOstate(false);
        }

        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].DrawInstanced(camera, shader, shaderPrep, cubemap, material, EnvironmentAmbientAmount, InstanceCount);
    }
}

void FUSIONCORE::Model::DrawDeferredInstanced(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, Material material, VBO& InstanceDataVBO, size_t InstanceCount)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        material.SetMaterialShader(shader);

        if (InstanceDataVBO.IsVBOchanged())
        {
            InstanceDataVBO.Bind();
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribDivisor(7, 1);
            InstanceDataVBO.SetVBOstate(false);
        }
        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].DrawDeferredInstanced(camera, shader, shaderPrep, InstanceCount);
    }
}

void FUSIONCORE::Model::DrawDeferredInstanced(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, VBO& InstanceDataVBO, size_t InstanceCount)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);

        if (InstanceDataVBO.IsVBOchanged())
        {
            InstanceDataVBO.Bind();
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribDivisor(7, 1);
            InstanceDataVBO.SetVBOstate(false);
        }
        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].DrawDeferredInstanced(camera, shader, shaderPrep, InstanceCount);
    }
}

void FUSIONCORE::Model::DrawDeferredInstancedImportedMaterial(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, VBO& InstanceDataVBO, size_t InstanceCount)
{
    auto shaderPrep = [&](Material& material) {
        return [&]() {
            this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
            shader.setFloat("ModelID", this->GetObjectID());
            shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
            material.SetMaterialShader(shader);

            if (InstanceDataVBO.IsVBOchanged())
            {
                InstanceDataVBO.Bind();
                glEnableVertexAttribArray(7);
                glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glVertexAttribDivisor(7, 1);
                InstanceDataVBO.SetVBOstate(false);
            }
            ShaderPreperations();
        };
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        auto& mesh = Meshes[i];
        mesh.DrawDeferredInstanced(camera, shader, shaderPrep(mesh.ImportedMaterial), InstanceCount);
    }
}

void FUSIONCORE::Model::DrawDeferred(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, Material material, std::vector<glm::mat4>& AnimationBoneMatrices)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);

        AnimationUniformBufferObject->Bind();
        for (size_t i = 0; i < AnimationBoneMatrices.size(); i++)
        {
            glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4), sizeof(glm::mat4), &AnimationBoneMatrices[i]);
        }
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        
        shader.setBool("EnableAnimation", true);
        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].DrawDeferred(camera, shader, shaderPrep, material);
    }
}

void FUSIONCORE::Model::DrawDeferred(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, Material material)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        shader.setBool("EnableAnimation", false);
        ShaderPreperations();
        };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].DrawDeferred(camera, shader, shaderPrep, material);
    }
}

void FUSIONCORE::Model::DrawDeferredIndirect(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, Material material)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        shader.setBool("EnableAnimation", false);
        ShaderPreperations();
        };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].DrawDeferredIndirect(camera, shader, shaderPrep, material);
    }
}

void FUSIONCORE::Model::DrawDeferredImportedMaterial(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        shader.setBool("EnableAnimation", false);
        ShaderPreperations();
        };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].DrawDeferredImportedMaterial(camera, shader, shaderPrep);
    }
}

void FUSIONCORE::Model::Draw(Camera3D& camera, Shader& shader, std::vector<Material> materials, const std::function<void()>& ShaderPreperations, CubeMap& cubemap, float EnvironmentAmbientAmount)
{
    std::function<void()> shaderPrep = [&]() 
    {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        FUSIONCORE::SendLightsShader(shader);
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setInt("OmniShadowMapCount", 0);
        shader.setBool("EnableAnimation", false);
        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].Draw(camera, shader, shaderPrep, cubemap, materials[i], EnvironmentAmbientAmount);
    }
}

void FUSIONCORE::Model::Draw(Camera3D& camera, Shader& shader, std::vector<Material> materials, const std::function<void()>& ShaderPreperations, CubeMap& cubemap, std::vector<OmniShadowMap*> ShadowMaps, float EnvironmentAmbientAmount)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        FUSIONCORE::SendLightsShader(shader);
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setInt("OmniShadowMapCount", ShadowMaps.size());
        for (size_t i = 0; i < ShadowMaps.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + 7 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ShadowMaps[i]->GetShadowMap());
            glUniform1i(glGetUniformLocation(shader.GetID(), ("OmniShadowMaps[" + std::to_string(i) + "]").c_str()), 7 + i);
        }
        shader.setBool("EnableAnimation", false);
        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].Draw(camera, shader, shaderPrep, cubemap, materials[i], EnvironmentAmbientAmount);
    }
}

void FUSIONCORE::Model::Draw(Camera3D& camera, Shader& shader, std::vector<Material> materials, const std::function<void()>& ShaderPreperations, CubeMap& cubemap, std::vector<OmniShadowMap*> ShadowMaps, std::vector<glm::mat4>& AnimationBoneMatrices, float EnvironmentAmbientAmount)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        FUSIONCORE::SendLightsShader(shader);
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setInt("OmniShadowMapCount", ShadowMaps.size());
        for (size_t i = 0; i < ShadowMaps.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + 7 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ShadowMaps[i]->GetShadowMap());
            glUniform1i(glGetUniformLocation(shader.GetID(), ("OmniShadowMaps[" + std::to_string(i) + "]").c_str()), 7 + i);
        }
        for (int i = 0; i < AnimationBoneMatrices.size(); ++i)
        {
            shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", AnimationBoneMatrices[i]);
        }
        shader.setBool("EnableAnimation", true);
        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].Draw(camera, shader, shaderPrep, cubemap, materials[i], EnvironmentAmbientAmount);
    }
}

void FUSIONCORE::Model::DrawImportedMaterial(Camera3D& camera, Shader& shader, const std::function<void()>& ShaderPreperations, CubeMap& cubemap, float EnvironmentAmbientAmount)
{
    std::function<void()> shaderPrep = [&]() {
        this->GetTransformation().SetModelMatrixUniformLocation(shader.GetID(), "model");
        FUSIONCORE::SendLightsShader(shader);
        shader.setFloat("ModelID", this->GetObjectID());
        shader.setFloat("ObjectScale", this->GetTransformation().scale_avg);
        shader.setBool("EnableAnimation", false);
        ShaderPreperations();
    };

    for (size_t i = 0; i < Meshes.size(); i++)
    {
        Meshes[i].DrawImportedMaterial(camera, shader, shaderPrep, cubemap, EnvironmentAmbientAmount);
    }
}

void FUSIONCORE::Model::FindGlobalMeshScales()
{
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    Vertex vertex;

    std::vector<glm::vec3> originPoints;
	for (size_t j = 0; j < Meshes.size(); j++)
	{
        auto& Mesh = Meshes[j];
		auto& VertexArray = Mesh.GetVertices();

        float MeshminX = std::numeric_limits<float>::max();
        float MeshmaxX = std::numeric_limits<float>::lowest();
        float MeshminY = std::numeric_limits<float>::max();
        float MeshmaxY = std::numeric_limits<float>::lowest();
        float MeshminZ = std::numeric_limits<float>::max();
        float MeshmaxZ = std::numeric_limits<float>::lowest();

		for (unsigned int k = 0; k < VertexArray.size(); k++) 
        {
			vertex.Position.x = VertexArray[k]->Position.x;
			vertex.Position.y = VertexArray[k]->Position.y;
			vertex.Position.z = VertexArray[k]->Position.z;

            MeshmaxX = std::max(MeshmaxX, vertex.Position.x);
            MeshmaxY = std::max(MeshmaxY, vertex.Position.y);
            MeshmaxZ = std::max(MeshmaxZ, vertex.Position.z);
            MeshminX = std::min(MeshminX, vertex.Position.x);
            MeshminY = std::min(MeshminY, vertex.Position.y);
            MeshminZ = std::min(MeshminZ, vertex.Position.z);
		}
        Mesh.SetInitialMeshMin({ MeshminX,MeshminY ,MeshminZ });
        Mesh.SetInitialMeshMax({ MeshmaxX,MeshmaxY ,MeshmaxZ });

        maxX = std::max(MeshmaxX, maxX);
        maxY = std::max(MeshmaxY, maxY);
        maxZ = std::max(MeshmaxZ, maxZ);
        minX = std::min(MeshminX, minX);
        minY = std::min(MeshminY, minY);
        minZ = std::min(MeshminZ, minZ);

        float MeshCenterX = (MeshminX + MeshmaxX) * 0.5f;
        float MeshCenterY = (MeshminY + MeshmaxY) * 0.5f;
        float MeshCenterZ = (MeshminZ + MeshmaxZ) * 0.5f;

        Mesh.SetMeshOriginPoint({ MeshCenterX,MeshCenterY ,MeshCenterZ });
	}

	float meshWidth = maxX - minX;
	float meshHeight = maxY - minY;
	float meshDepth = maxZ - minZ;

	transformation.ObjectScales.x = meshWidth;
	transformation.ObjectScales.y = meshHeight;
	transformation.ObjectScales.z = meshDepth;

	transformation.scale_avg = (transformation.ObjectScales.x + transformation.ObjectScales.y + transformation.ObjectScales.z) / 3;
	transformation.dynamic_scale_avg = transformation.scale_avg;

	transformation.InitialObjectScales = transformation.ObjectScales;

    float overallCenterX = (maxX + minX) * 0.5f,
          overallCenterY = (maxY + minY) * 0.5f,
          overallCenterZ = (maxZ + minZ) * 0.5f;
  
    originpoint = { overallCenterX,overallCenterY,overallCenterZ };
    transformation.OriginPoint = &this->originpoint;
    transformation.Position = glm::vec3(originpoint.x, originpoint.y, originpoint.z);
    dynamic_origin = glm::vec3(overallCenterX, overallCenterY, overallCenterZ);
}

void FUSIONCORE::Model::SetIndirectCommandBuffer(unsigned int InstanceCount, unsigned int BaseVertex, unsigned int BaseIndex, unsigned int BaseInstance)
{
    for (auto& mesh : this->Meshes)
    {
        mesh.SetIndirectCommandBuffer(InstanceCount,BaseVertex,BaseIndex,BaseInstance);
    }
}

void FUSIONCORE::Model::SetInstanced(VBO& InstanceDataVBO, size_t InstanceCount)
{
    this->InstanceCount = InstanceCount;
    this->InstanceDataVBO = &InstanceDataVBO;
    this->InstanceDataVBO->SetVBOstate(true);
}

void FUSIONCORE::Model::SetAlphaMaterial(Material& material)
{
    this->AlphaMaterial = &material;
}

FUSIONCORE::Model::~Model()
{
    for (size_t i = 0; i < this->Meshes.size(); i++)
    {
        Meshes[i].Clean();
    }
    LOG_INF("Model" << this->ModelID << " buffers cleaned!");
}

void FUSIONCORE::Model::SetVertexBoneDataDefault(Vertex& vertex)
{
    for (size_t i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        vertex.m_BoneIDs[i] = -1;
        vertex.m_Weights[i] = 0.0f;
    }
}

void FUSIONCORE::Model::loadModel(std::string const& path, bool Async , bool AnimationModel)
{
    Assimp::Importer importer;
    int flags;
    if (AnimationModel)
    {
        flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;
    }
    else
    {
        flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices;
    }
    const aiScene* scene = importer.ReadFile(path, flags);
    this->scene = (aiScene*)scene;
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        this->ModelImportStateCode = FF_ERROR_CODE;
        LOG_ERR("Error reading the object file :: " << importer.GetErrorString());
        return;
    }

    directory = path;
    if (Async)
    {
        processNodeAsync(scene->mRootNode, scene);
    }
    else
    {
        processNode(scene->mRootNode, scene);
    }
    this->ModelImportStateCode = FF_SUCCESS_CODE;
}

void FUSIONCORE::Model::processNode(aiNode* node, const aiScene* scene)
{
    this->ModelName = scene->mName.data;
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

void FUSIONCORE::Model::processNodeAsync(aiNode* node, const aiScene* scene)
{
    this->ModelName = scene->mName.data;
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        PreMeshDatas.push_back(processMeshAsync(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNodeAsync(node->mChildren[i], scene);
    }
}

void FUSIONCORE::Model::ConstructMeshes(std::vector<PreMeshData> PreMeshDatas)
{
    for (size_t i = 0; i < PreMeshDatas.size(); i++)
    {
        PreMeshData& data = PreMeshDatas[i];
        Meshes.push_back(Mesh(data.vertices,data.indices,data.textures));
    }
}

FUSIONCORE::PreMeshData FUSIONCORE::Model::processMeshAsync(aiMesh* mesh, const aiScene* scene)
{
    std::vector<std::shared_ptr<Vertex>> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<Texture2D>> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector;

        SetVertexBoneDataDefault(vertex);

        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }

        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;

            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;

            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;

            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(std::make_shared<Vertex>(vertex));
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }


    ExtractBones(vertices, mesh, scene);

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    return PreMeshData(textures , indices , vertices);
}

void FUSIONCORE::Model::ProcessMeshMaterial(aiMaterial* material,FUSIONCORE::Material& TempMaterial)
{
    LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", FF_TEXTURE_DIFFUSE, TempMaterial);
    aiString path;
    if (material->GetTexture(aiTextureType_SHININESS, 0, &path) == AI_SUCCESS) {
        LoadMaterialTextures(material, aiTextureType_SHININESS, "texture_specular", FF_TEXTURE_SPECULAR, TempMaterial);
    }
    else if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path) == AI_SUCCESS) {
        LoadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_specular", FF_TEXTURE_SPECULAR, TempMaterial);
    }
    else if (material->GetTexture(aiTextureType_SPECULAR, 0, &path) == AI_SUCCESS) {
        LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", FF_TEXTURE_SPECULAR, TempMaterial);
    }
    LoadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", FF_TEXTURE_NORMAL, TempMaterial);
    LoadMaterialTextures(material, aiTextureType_METALNESS, "texture_metalic", FF_TEXTURE_METALLIC, TempMaterial);
    LoadMaterialTextures(material, aiTextureType_OPACITY, "texture_alpha", FF_TEXTURE_ALPHA, TempMaterial);
    LoadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive", FF_TEXTURE_EMISSIVE, TempMaterial);
    
    aiColor4D color;
    float value;
    aiString name;
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, color)) {
        TempMaterial.Emission.x = color.r;
        TempMaterial.Emission.y = color.g;
        TempMaterial.Emission.z = color.b;
        TempMaterial.Emission.w = color.a;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
        TempMaterial.Albedo.x = color.r;
        TempMaterial.Albedo.y = color.g;
        TempMaterial.Albedo.z = color.b;
        TempMaterial.Albedo.w = color.a;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, value)) {
        TempMaterial.Metallic = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_ROUGHNESS_FACTOR, value)) {
        TempMaterial.Roughness = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_SPECULAR_FACTOR, value)) {
        TempMaterial.Roughness = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_CLEARCOAT_FACTOR, value)) {
        TempMaterial.ClearCoat = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_OPACITY, value)) {
        TempMaterial.Alpha = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_TRANSMISSION_FACTOR, value)) {
        TempMaterial.Alpha = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_REFRACTI, value)) {
        TempMaterial.IOR = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_ANISOTROPY_FACTOR, value)) {
        TempMaterial.Anisotropy = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_CLEARCOAT_FACTOR, value)) {
        TempMaterial.ClearCoat = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, value)) {
        TempMaterial.ClearCoatRoughness = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_SHEEN_ROUGHNESS_FACTOR, value)) {
        TempMaterial.Sheen = value;
    }
    if (aiReturn_SUCCESS == material->Get(AI_MATKEY_NAME, name)) {
        TempMaterial.MaterialName = name.C_Str();
    }
    //TempMaterial.PrintMaterialAttributes();
}

void FUSIONCORE::Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<std::shared_ptr<Vertex>> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<Texture2D>> textures;
    std::vector<std::shared_ptr<Face>> Faces;

    std::unordered_map<glm::vec3, unsigned int, Vec3Hash> PositionIndexMap;

    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector;

        SetVertexBoneDataDefault(vertex);

        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }

        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;

            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;

            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;

            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(std::make_shared<Vertex>(vertex));
    }

    Faces.reserve(mesh->mNumFaces);
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        Face NewFace;
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            if (PositionIndexMap.find(vertices[face.mIndices[j]]->Position) == PositionIndexMap.end())
            {
                NewFace.Indices.push_back(face.mIndices[j]);
                PositionIndexMap[vertices[face.mIndices[j]]->Position] = face.mIndices[j];
            }
            else
            {
                NewFace.Indices.push_back(PositionIndexMap[vertices[face.mIndices[j]]->Position]);
            }
            indices.push_back(face.mIndices[j]);
        }
        Faces.push_back(std::make_shared<Face>(NewFace));
    }

    ExtractBones(vertices, mesh, scene);

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    Material TempMaterial;

    ProcessMeshMaterial(material, TempMaterial);

    auto & newMesh = Meshes.emplace_back(vertices, indices, Faces, TempMaterial);
    newMesh.MeshName = mesh->mName.data;
}

void FUSIONCORE::Model::ExtractBones(std::vector<std::shared_ptr<Vertex>>& vertices, aiMesh* mesh, const aiScene* scene)
{
    for (size_t i = 0; i < mesh->mNumBones; i++)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[i]->mName.C_Str();
        if (Bones.find(boneName) == Bones.end())
        {
            BoneInfo newBone;
            newBone.id = boneCounter;
            newBone.OffsetMat = ConvertMatrixToGLMFormat(mesh->mBones[i]->mOffsetMatrix);
            Bones[boneName] = newBone;
            boneID = boneCounter;
            boneCounter++;
        }
        else
        {
            boneID = Bones[boneName].id;
        }

        assert(boneID != -1);
        auto weights = mesh->mBones[i]->mWeights;
        int NumberWeights = mesh->mBones[i]->mNumWeights;

        for (size_t x = 0; x < NumberWeights; x++)
        {
            int vertexID = weights[x].mVertexId;
            float weight = weights[x].mWeight;
            assert(vertexID <= vertices.size());

            auto& vertextemp = vertices[vertexID];
            for (size_t e = 0; e < MAX_BONE_INFLUENCE; e++)
            {
                if (vertextemp->m_BoneIDs[e] < 0)
                {
                    vertextemp->m_Weights[e] = weight;
                    vertextemp->m_BoneIDs[e] = boneID;
                    break;
                }
            }
        }

    }
}

void FUSIONCORE::Model::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName,const char* TextureKey,FUSIONCORE::Material& DestinationMaterial)
{
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        if (textures_loaded.find(str.C_Str()) != textures_loaded.end())
        {
            DestinationMaterial.PushTextureMap(TextureKey, textures_loaded[str.C_Str()].get());
            skip = true;
            break;
        }
        if (!skip)
        {
            if (auto texture = scene->GetEmbeddedTexture(str.C_Str())) {
                int Width = texture->mWidth;
                int Height = texture->mHeight;
                int ChannelCount = 4;

                unsigned char* TextureData = nullptr;
                if (Height == 0)
                {
                    TextureData = stbi_load_from_memory(reinterpret_cast<unsigned char*>(texture->pcData), texture->mWidth, &Width, &Height, &ChannelCount, 0);
                }
                else
                {
                    TextureData = stbi_load_from_memory(reinterpret_cast<unsigned char*>(texture->pcData), texture->mWidth * texture->mHeight, &Width, &Height, &ChannelCount, 0);
                }
                const char* TexturePath = texture->mFilename.C_Str();

                auto SharedTexture = std::make_shared<Texture2D>(TextureData, Width, Height, ChannelCount, TexturePath, GL_REPEAT, GL_REPEAT, GL_TEXTURE_2D, GL_UNSIGNED_BYTE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
                if (SharedTexture->GetTextureState() == FF_TEXTURE_ERROR) continue;
                SharedTexture->PbrMapType = typeName;

                DestinationMaterial.PushTextureMap(TextureKey, SharedTexture.get());
                textures_loaded[str.C_Str()] = SharedTexture;
            }
            else
            {
                std::string FilePath;
                if (directory.find('\\') != std::string::npos)
                {
                    FilePath = directory.substr(0, directory.find_last_of('\\'));
                    FilePath += "\\" + std::string(str.C_Str());
                }
                else if (directory.find('/') != std::string::npos)
                {
                    FilePath = directory.substr(0, directory.find_last_of('/'));
                    FilePath += "/" + std::string(str.C_Str());
                }

                auto SharedTexture = std::make_shared<Texture2D>(FilePath.c_str(), GL_REPEAT, GL_REPEAT, GL_TEXTURE_2D, GL_UNSIGNED_BYTE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
                if (SharedTexture->GetTextureState() == FF_TEXTURE_ERROR) continue;
                SharedTexture->PbrMapType = typeName;

                DestinationMaterial.PushTextureMap(TextureKey, SharedTexture.get());
                textures_loaded[str.C_Str()] = SharedTexture;
            }
        }
    }
}

glm::mat4 FUSIONCORE::ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
{
    glm::mat4 to;

    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

bool isFormatSupported(const std::string& extension) {
    Assimp::Importer importer;
    return importer.IsExtensionSupported(extension);
}

//Import multiple models into a vector from a directory. It's not safe for files other than mtl(except object supported formats).
std::vector<std::shared_ptr<FUSIONCORE::Model>> FUSIONCORE::ImportMultipleModelsFromDirectory(const char* DirectoryFilePath, bool AnimationModel)
{
    std::vector<std::shared_ptr<FUSIONCORE::Model>> Models;
    std::string path = DirectoryFilePath;
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        std::string EntryPath = entry.path().generic_string();
        auto DotFound = EntryPath.find_last_of(".");
        if (DotFound != std::string::npos && isFormatSupported(EntryPath.substr(DotFound)))
        {
           std::shared_ptr<FUSIONCORE::Model> NewModel = std::make_shared<Model>(EntryPath, false, AnimationModel);
           if (NewModel->GetModelImportStateCode() == FF_SUCCESS_CODE)
           {
              Models.push_back(std::move(NewModel));
           }
        }
    }
    return Models;
}

FUSIONCORE::Model* FUSIONCORE::GetModel(unsigned int ModelID)
{
    if (IDmodelMap.find(ModelID) != IDmodelMap.end())
    {
        return IDmodelMap[ModelID];
    }
    return nullptr;
}

FUSIONCORE::PreMeshData::PreMeshData(std::vector<std::shared_ptr<FUSIONCORE::Texture2D>>& textures, std::vector<unsigned int>& indices, std::vector<std::shared_ptr<Vertex>>& vertices)
{
    this->textures = textures;
    this->indices = indices;
    this->vertices = vertices;
}
