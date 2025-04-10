#include "Framebuffer.hpp"
#include <glew.h>
#include <glfw3.h>
#include <FreeImage.h>
#include "Buffer.h"

unsigned int IDiterator = 0;

void FUSIONCORE::CopyDepthInfoFBOtoFBO(GLuint src, glm::vec2 srcSize, GLuint dest)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest);
	glBlitFramebuffer(0, 0, srcSize.x, srcSize.y, 0, 0, srcSize.x, srcSize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
};

FUSIONCORE::Color FUSIONCORE::ReadFrameBufferPixel(int Xcoord, int Ycoord,unsigned int FramebufferAttachmentMode,GLenum AttachmentFormat,glm::vec2 CurrentWindowSize)
{
	glReadBuffer((GLenum)FramebufferAttachmentMode);
	float pixel[4];
	glReadPixels(Xcoord ,CurrentWindowSize.y - Ycoord, 1, 1, AttachmentFormat, GL_FLOAT, &pixel);
	//LOG("PIXEL PICKED: " << pixel[0] << " " << pixel[1] << " " << pixel[2] << " " << pixel[3] << " " << Xcoord << " " << Ycoord);
	glReadBuffer(GL_NONE);
	Color PixelColor;
	PixelColor.SetRGBA({ pixel[0], pixel[1], pixel[2], pixel[3] });
    return PixelColor;
}

void FUSIONCORE::SaveFrameBufferImage(const int& width, const int& height, const char* path, const GLenum& Attachment)
{
	BYTE* pixels;

	Vec2<int> finalImageSize;
	int ChannelSize = 0;

	finalImageSize({ (int)width,(int)height });
	ChannelSize = 3;
	pixels = new BYTE[ChannelSize * finalImageSize.x * finalImageSize.y];

	glReadBuffer(Attachment);
	glReadPixels(0, 0, finalImageSize.x, finalImageSize.y, GL_BGR, GL_UNSIGNED_BYTE, pixels);

	FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, finalImageSize.x, finalImageSize.y, ChannelSize * finalImageSize.x, 8 * ChannelSize, 0x0000FF, 0xFF0000, 0x00FF00, false);
	FreeImage_Save(FIF_PNG, image, path, 0);

	FreeImage_Unload(image);
	delete[] pixels;
}

FUSIONCORE::ScreenFrameBuffer::ScreenFrameBuffer(int width, int height)
{
	static int itr = 0;
	ID = itr;
	FBOSize.SetValues(width, height);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &fboImage);
	glBindTexture(GL_TEXTURE_2D, fboImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboImage, 0);

	glGenTextures(1, &fboDepth);
	glBindTexture(GL_TEXTURE_2D, fboDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fboDepth, 0);

	glGenTextures(1, &SSRtexture);
	glBindTexture(GL_TEXTURE_2D, SSRtexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, SSRtexture, 0);

	glGenTextures(1, &IDtexture);
	glBindTexture(GL_TEXTURE_2D, IDtexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, IDtexture, 0);

	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 ,GL_COLOR_ATTACHMENT2  , GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LOG_ERR("Error Completing the frameBuffer[ID:" << ID << "]!");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	LOG_INF("Completed the frameBuffer[ID:" << ID << "]!");

	itr++;
}

void FUSIONCORE::ScreenFrameBuffer::Bind() 
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo); 
}
void FUSIONCORE::ScreenFrameBuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
};

void FUSIONCORE::ScreenFrameBuffer::Draw(Camera3D& camera, Shader& shader,std::function<void()> ShaderPrep, const glm::ivec2& WindowSize, bool DOFenabled, float DOFdistanceFar, float DOFdistanceClose, float DOFintensity, float Gamma, float Exposure)
{
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, FBOSize.x, FBOSize.y);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	shader.use();
	GetRectangleBuffer()->Bind();
	ShaderPrep();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fboImage);
	shader.setInt("Viewport", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fboDepth);
	shader.setInt("DepthAttac", 1);

	shader.setVec3("CamPos", camera.Position);
	shader.setFloat("FarPlane", camera.FarPlane);
	shader.setFloat("NearPlane", camera.NearPlane);
	shader.setFloat("DOFdistanceFar", DOFdistanceFar);
	shader.setFloat("DOFdistanceClose", DOFdistanceClose);
	shader.setBool("DOFenabled", DOFenabled);
	shader.setFloat("DOFintensity", DOFintensity);
	shader.setFloat("Gamma", Gamma);
	shader.setFloat("Exposure", Exposure);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	BindVAONull();
	UseShaderProgram(0);
	glEnable(GL_DEPTH_TEST);
}

void FUSIONCORE::DrawTextureOnQuad(const GLuint& TargetImage, const glm::vec2& LocalQuadPosition, const glm::vec2& QuadSizeInPixel, FUSIONCORE::Camera3D& camera, FUSIONCORE::Shader& shader,const glm::vec2& SampleMultiplier, float Gamma, float Exposure) {
	glViewport(LocalQuadPosition.x,LocalQuadPosition.y, QuadSizeInPixel.x, QuadSizeInPixel.y);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	shader.use();
	GetRectangleBuffer()->Bind();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, TargetImage);
	shader.setInt("Viewport", 0);
	shader.setVec2("SampleMultiplier", SampleMultiplier);

	shader.setFloat("Gamma", Gamma);
	shader.setFloat("Exposure", Exposure);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	BindVAONull();
	UseShaderProgram(0);
	glEnable(GL_DEPTH_TEST);
}

void FUSIONCORE::ScreenFrameBuffer::clean()
{
	glDeleteTextures(1, &fboImage);
	glDeleteTextures(1, &fboDepth);
	glDeleteTextures(1, &IDtexture);
	glDeleteTextures(1, &SSRtexture);
	glDeleteRenderbuffers(1, &rbo);
	glDeleteFramebuffers(1, &fbo);

	LOG_INF("Cleaned frameBuffer[ID:" << ID << "]!");
};

FUSIONCORE::GeometryBuffer::GeometryBuffer(int width, int height,bool EnableHighPrecisionPositionBuffer)
{
	static int itr = 0;
	ID = itr;
	FBOSize.x = width;
	FBOSize.y = height;

	GLenum PositionBufferFormat = EnableHighPrecisionPositionBuffer ? GL_RGB32F : GL_RGB16F;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &AlbedoSpecularPass);
	glBindTexture(GL_TEXTURE_2D, AlbedoSpecularPass);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, AlbedoSpecularPass, 0);

	glGenTextures(1, &NormalMetalicPass);
	glBindTexture(GL_TEXTURE_2D, NormalMetalicPass);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, NormalMetalicPass, 0);

	glGenTextures(1, &PositionDepthPass);
	glBindTexture(GL_TEXTURE_2D, PositionDepthPass);
	glTexImage2D(GL_TEXTURE_2D, 0, PositionBufferFormat, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, PositionDepthPass, 0);

	glGenTextures(1, &MetalicRoughnessPass);
	glBindTexture(GL_TEXTURE_2D, MetalicRoughnessPass);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, MetalicRoughnessPass, 0);

	glGenTextures(1, &DecalNormalPass);
	glBindTexture(GL_TEXTURE_2D, DecalNormalPass);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, DecalNormalPass, 0);

	SetDrawModeDefault();

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LOG_ERR("Error Completing the G-buffer[ID:" << ID << "]!");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	LOG_INF("Completed the G-buffer[ID:" << ID << "]!");

	itr++;
}


void FUSIONCORE::GeometryBuffer::SetDrawModeDefault()
{
	unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 ,GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3 , GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(5, attachments);
}

void FUSIONCORE::GeometryBuffer::SetDrawModeDecalPass()
{
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT3,GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(3, attachments);
}

void FUSIONCORE::GeometryBuffer::SetDrawModeDefaultRestricted()
{
	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 ,GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);
}

void FUSIONCORE::GeometryBuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}
void FUSIONCORE::GeometryBuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
};

void FUSIONCORE::GeometryBuffer::DrawSceneDeferred(Camera3D& camera, Shader& shader, std::function<void()> ShaderPrep, const glm::ivec2& WindowSize, std::vector<OmniShadowMap*> &ShadowMaps,CubeMap& cubeMap, glm::vec4 BackgroundColor, float EnvironmentAmbientAmount)
{
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(3, attachments);
	glViewport(0, 0, FBOSize.x, FBOSize.y);
	glClearColor(BackgroundColor.x, BackgroundColor.y, BackgroundColor.z, BackgroundColor.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	shader.use();
	GetRectangleBuffer()->Bind();
	ShaderPrep();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, AlbedoSpecularPass );
	shader.setInt("AlbedoSpecularPass", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, NormalMetalicPass);
	shader.setInt("NormalPass", 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, PositionDepthPass);
	shader.setInt("PositionDepthPass", 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, MetalicRoughnessPass);
	shader.setInt("MetalicRoughnessModelIDPass", 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, DecalNormalPass);
	shader.setInt("DecalNormalPass", 5);

	shader.setVec3("CameraPos", camera.Position);
	shader.setFloat("FarPlane", camera.FarPlane);
	shader.setFloat("NearPlane", camera.NearPlane);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap.GetConvDiffCubeMap());
	shader.setInt("ConvDiffCubeMap", 6);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap.GetPreFilteredEnvMap());
	shader.setInt("prefilteredMap", 7);

	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, FUSIONCORE::brdfLUT);
	shader.setInt("LUT", 8);

	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D_ARRAY, FUSIONCORE::GetCascadedShadowMapTextureArray());
	shader.setInt("CascadeShadowMaps", 9);

	auto CascadedShadowMapsMetaData = GetCascadedShadowMapMetaDataSSBO();
	CascadedShadowMapsMetaData->BindSSBO(10);

	shader.setMat4("ViewMatrix", camera.viewMat);

	shader.setBool("EnableIBL", true);
	shader.setFloat("ao", EnvironmentAmbientAmount);
	shader.setInt("OmniShadowMapCount", ShadowMaps.size());

	for (size_t i = 0; i < ShadowMaps.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + 10 + i);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ShadowMaps[i]->GetShadowMap());
		glUniform1i(glGetUniformLocation(shader.GetID(), ("OmniShadowMaps[" + std::to_string(i) + "]").c_str()), 10 + i);
		glUniform1f(glGetUniformLocation(shader.GetID(), ("ShadowMapFarPlane[" + std::to_string(i) + "]").c_str()), ShadowMaps[i]->GetFarPlane());
		glUniform1i(glGetUniformLocation(shader.GetID(), ("OmniShadowMapsLightIDS[" + std::to_string(i) + "]").c_str()), ShadowMaps[i]->GetBoundLightID());
	}

	FUSIONCORE::SendLightsShader(shader);
	shader.setVec2("screenSize", glm::vec2(1920, 1080));

	glDrawArrays(GL_TRIANGLES, 0, 6);

	BindVAONull();
	UseShaderProgram(0);
	glEnable(GL_DEPTH_TEST);
}

void FUSIONCORE::GeometryBuffer::DrawSSR(Camera3D& camera, Shader& shader, std::function<void()> ShaderPrep, Vec2<int> WindowSize)
{
	unsigned int attachments[1] = { GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(1, attachments);
	glViewport(0, 0, WindowSize.x, WindowSize.y);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	shader.use();
	GetRectangleBuffer()->Bind();
	ShaderPrep();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, AlbedoSpecularPass);
	shader.setInt("AlbedoSpecularPass", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, NormalMetalicPass);
	shader.setInt("NormalMetalicPass", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, PositionDepthPass);
	shader.setInt("PositionDepthPass", 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, MetalicRoughnessPass);
	shader.setInt("MetalicRoughnessPass", 3);

	shader.setVec3("CameraPos", camera.Position);
	shader.setFloat("FarPlane", camera.FarPlane);
	shader.setFloat("NearPlane", camera.NearPlane);

	shader.setMat4("ViewMatrix", camera.viewMat);
	shader.setMat4("InverseViewMatrix", glm::inverse(camera.viewMat));
	shader.setMat4("InverseProjectionMatrix", glm::inverse(camera.projMat));
	shader.setMat4("ProjectionMatrix", camera.projMat);

	shader.setVec2("WindowSize", { WindowSize.x,WindowSize.y });

	glDrawArrays(GL_TRIANGLES, 0, 6);

	BindVAONull();
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	UseShaderProgram(0);
	glEnable(GL_DEPTH_TEST);
}

void FUSIONCORE::GeometryBuffer::clean()
{
	glDeleteTextures(1, &AlbedoSpecularPass);
	glDeleteTextures(1, &NormalMetalicPass);
	glDeleteTextures(1, &PositionDepthPass);
	glDeleteTextures(1, &MetalicRoughnessPass);
	glDeleteTextures(1, &DecalNormalPass);
	glDeleteRenderbuffers(1, &rbo);
	glDeleteFramebuffers(1, &fbo);

	LOG_INF("Cleaned G-buffer[ID:" << ID << "]!");
};

FUSIONCORE::Framebuffer::Framebuffer()
{
	ID = IDiterator;
	IDiterator++;

	ColorAttachmentCount = 0;
	FramebufferSize.x = 0;
	FramebufferSize.y = 0;

	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LOG_ERR("Error Completing the G-buffer[ID:" << ID << "]!");
	}
}

FUSIONCORE::Framebuffer::Framebuffer(unsigned int width, unsigned int height)
{
	ID = IDiterator;
	IDiterator++;

	ColorAttachmentCount = 0;

	FramebufferSize.x = width;
	FramebufferSize.y = height;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LOG_ERR("Error Completing the G-buffer[ID:" << ID << "]!");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void FUSIONCORE::Framebuffer::PushFramebufferAttachment2D(FBOattachmentType attachmentType, GLint InternalFormat, GLenum Format, GLenum type, GLint MinFilter, GLint MaxFilter, GLint Wrap_S, GLint Wrap_T)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	Attachments.emplace_back();
	GLuint& attachmentID = Attachments.back();

	glGenTextures(1, &attachmentID);
	glBindTexture(GL_TEXTURE_2D, attachmentID);

	glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, FramebufferSize.x, FramebufferSize.y, 0, Format, type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MaxFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Wrap_S);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Wrap_T);

	GLenum AttachmentType = GL_COLOR_ATTACHMENT0;

	switch (attachmentType)
	{
	case FBO_ATTACHMENT_COLOR:
		AttachmentType = GL_COLOR_ATTACHMENT0 + ColorAttachmentCount;
		break;
	case FBO_ATTACHMENT_DEPTH:
		AttachmentType = GL_DEPTH_ATTACHMENT;
		break;
	default:
		break;
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, AttachmentType, GL_TEXTURE_2D, attachmentID, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LOG_ERR("Error pushing FBO attachment[ID:" << attachmentID << "]!");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ColorAttachmentCount++;
}

void FUSIONCORE::Framebuffer::SetDrawModeDefault()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	std::vector<unsigned int> AtBuffers;
	for (size_t i = 0; i < Attachments.size(); i++)
	{
		AtBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
	}
	glDrawBuffers(Attachments.size(), AtBuffers.data());
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FUSIONCORE::Framebuffer::DrawAttachment(size_t index, Shader& shader,const std::function<void()> &ShaderPrep)
{
	glViewport(0, 0, this->FramebufferSize.x, this->FramebufferSize.y);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	shader.use();
	GetRectangleBuffer()->Bind();
	ShaderPrep();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->GetFBOattachment(index));
	shader.setInt("Viewport", 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	BindVAONull();
	UseShaderProgram(0);
	glEnable(GL_DEPTH_TEST);
}

void FUSIONCORE::Framebuffer::AttachRenderBufferTexture2DTarget(Texture2D& InputTexture,const GLuint& MipmapWidth, const GLuint& MipmapHeight,const GLuint &MipmapLevel)
{
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	if (MipmapWidth > 0 && MipmapHeight > 0) glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, MipmapWidth, MipmapWidth);
	else glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, InputTexture.GetWidth(), InputTexture.GetHeight());
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, InputTexture.GetTexture(), MipmapLevel);
}

void FUSIONCORE::Framebuffer::AttachRenderBufferTexture2DTarget(const GLuint& InputTexture, const GLuint& MipmapWidth, const GLuint& MipmapHeight, const GLuint& MipmapLevel)
{
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, MipmapWidth, MipmapWidth);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, InputTexture, MipmapLevel);
}

void FUSIONCORE::Framebuffer::AttachTexture2DTarget(Texture2D& InputTexture,const GLuint& MipmapLevel)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, InputTexture.GetTexture(), MipmapLevel);
}

void FUSIONCORE::Framebuffer::AttachTexture2DTarget(const GLuint& InputTexture, const GLuint& MipmapLevel)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, InputTexture, MipmapLevel);
}

void FUSIONCORE::Framebuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void FUSIONCORE::Framebuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FUSIONCORE::Framebuffer::~Framebuffer()
{
	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &rbo);

	for (auto& Attachment : Attachments)
	{
		glDeleteTextures(1, &Attachment);
	}
}
