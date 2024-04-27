///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/granite_texture.jpg",
		"island top");
	bReturn = CreateGLTexture(
		"textures/plaster_texture.jpg",
		"island stand");
	bReturn = CreateGLTexture(
		"textures/wicker_light_texture.jpg",
		"stand base");
	bReturn = CreateGLTexture(
		"textures/light_wood_texture.jpg",
		"top torus");
	bReturn = CreateGLTexture(
		"textures/bamboo_texture.jpg",
		"torus");
	bReturn = CreateGLTexture(
		"textures/granite_texture.jpg",
		"island top");
	bReturn = CreateGLTexture(
		"textures/wood_shiny_texture.jpg",
		"bottom torus");
	bReturn = CreateGLTexture(
		"textures/wood_floor_texture.jpg",
		"floor");
	bReturn = CreateGLTexture(
		"textures/matte_black_metal.jpg",
		"candle snuffer");
	bReturn = CreateGLTexture(
		"textures/ceramic_texture.jpg",
		"pottery");
	bReturn = CreateGLTexture(
		"textures/leather_texture1.jpg",
		"fruit");
	bReturn = CreateGLTexture(
		"textures/paper_texture2.jpg",
		"paper");
	bReturn = CreateGLTexture(
		"textures/wax_texture.jpg",
		"wax");
	bReturn = CreateGLTexture(
		"textures/dark_wood_texture1.jpg",
		"dark wood");
	bReturn = CreateGLTexture(
		"textures/stainless.jpg",
		"steel");
	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
	// First light is the primary light used, positioned center and higher up but with stronger focal strength
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 15.0f, 2.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.1f);

	// Secondary light, pisitioned to the left and further away from Island to mimic the lighting used in the reference image
	m_pShaderManager->setVec3Value("lightSources[1].position", -4.0f, 12.0f, 3.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 15.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);
}

void SceneManager::DefineObjectMaterials()
{
	// Set attributes for shader of metal material - candle snuffer
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	metalMaterial.ambientStrength = 0.5f;
	metalMaterial.diffuseColor = glm::vec3(0.25f, 0.25f, 0.25f);
	metalMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	metalMaterial.shininess = 30.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	// Set attributes for shader of wood material
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.20f, 0.20f, 0.20f);
	woodMaterial.ambientStrength = 0.25f;
	woodMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	woodMaterial.specularColor = glm::vec3(0.15f, 0.15f, 0.15f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	// Set attributes for shader of glass material - for vase 
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	// Set attributes for shader of ceramic material
	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	ceramicMaterial.ambientStrength = 0.25f;
	ceramicMaterial.diffuseColor = glm::vec3(0.86f, 0.82f, 0.78f);
	ceramicMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	glassMaterial.shininess = 25.0;
	ceramicMaterial.tag = "ceramic";

	m_objectMaterials.push_back(ceramicMaterial);

	// Set attributes for shader of walling material
	OBJECT_MATERIAL wallingMaterial;
	wallingMaterial.ambientColor = glm::vec3(0.45f, 0.45f, 0.45f);
	wallingMaterial.ambientStrength = 0.7f;
	wallingMaterial.diffuseColor = glm::vec3(0.4f, 0.5f, 0.35f);
	wallingMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	wallingMaterial.shininess = 0.0;
	wallingMaterial.tag = "walling";

	m_objectMaterials.push_back(wallingMaterial);

	// Set attributes for shader of leather material
	OBJECT_MATERIAL leatherMaterial;
	leatherMaterial.ambientColor = glm::vec3(0.56f, 0.45f, 0.113f);
	leatherMaterial.ambientStrength = 0.2f;
	leatherMaterial.diffuseColor = glm::vec3(0.76f, 0.64f, 0.2f);
	leatherMaterial.specularColor = glm::vec3(0.76f, 0.64f, 0.2f);
	leatherMaterial.shininess = 0.0;
	leatherMaterial.tag = "leather";

	m_objectMaterials.push_back(leatherMaterial);

	// Set attributes for shader of paper material
	OBJECT_MATERIAL paperMaterial;
	paperMaterial.ambientColor = glm::vec3(0.49f, 0.41f, 0.28f);
	paperMaterial.ambientStrength = 0.2f;
	paperMaterial.diffuseColor = glm::vec3(0.69f, 0.63f, 0.53f);
	paperMaterial.specularColor = glm::vec3(0.85f, 0.8f, 0.69f);
	paperMaterial.shininess = 0.5;
	paperMaterial.tag = "parchment";

	m_objectMaterials.push_back(paperMaterial);
}
 /***********************************************************
  *  PrepareScene()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// Load the textures used in scene
	LoadSceneTextures();
	// Define object materials
	DefineObjectMaterials();
	// Setup lights used
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	//Call methods for scaling texture and setting it in scene
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("floor");
	
	// Call shader with wood material
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// === ISLAND BASE - BOX === // 

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(12.0f, 7.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Call methods for scaling texture and setting it in scene
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("island stand");

	// Call shader with walling material
	SetShaderMaterial("walling");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// === ISLAND TOP - BOX === // 

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.0f, 0.3f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.51f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Call methods for scaling texture and setting it in scene
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("island top");

	// Call shader using glass
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//  === RATAN BASKET BASE - CYLINDER === //
	// Flatted to act as base of stand
	scaleXYZ = glm::vec3(2.0f, 0.1f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.60f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Call methods for scaling texture and setting it in scene
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("stand base");

	// Call shader with wood material
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// === RATAN BASKET RIM - TORUS - BOTTOM === //
	scaleXYZ = glm::vec3(2.0f, 2.0f, 1.0f);

	// set the XYZ rotation for the mesh
	// Rotated to be angled parallel with the ground
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.70f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Call methods for scaling texture and setting it in scene
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("bottom torus");

	// Call shader with wood material
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	// === RATAN BASKET RIM - TORUS - TOP === //
	scaleXYZ = glm::vec3(2.0f, 2.0f, 1.0f);

	// set the XYZ rotation for the mesh
	// Rotated to be angled parallel with the ground
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.80f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Call methods for scaling texture and setting it in scene
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("top torus");

	// Call shader with wood material
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	// ========= CANDLE SNUFFER OBJECT ========== //

	// === CANDLE SNUFFER - BODY - CYLINDER === //
	// Scaled to be much smaller
	scaleXYZ = glm::vec3(0.06f, 0.15f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.70f, 1.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Call methods for scaling texture and setting it in scene
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("candle snuffer");

	// Call shader with metal material
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// === CANDLE SNUFFER - HINGE - BOX === //
	// Scaled to be much smaller

	scaleXYZ = glm::vec3(0.05f, 0.07f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.88f, 1.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Call methods for scaling texture and setting it in scene
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("candle snuffer");

	// Call shader with metal material
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// === CANDLE SNUFFER - HANDLE - BOX === //
	// Scaled to be much smaller
	scaleXYZ = glm::vec3(0.5f, 0.025f, 0.025f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 160.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 3.80f, 1.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Call methods for scaling texture and setting it in scene
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("candle snuffer");

	// Call shader with metal material
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// === GLASS VASE - BODY - SPHERE === // 
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 4.6f, -0.65f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Call shader with glass material
	SetShaderMaterial("glass");

	// Setting color for object to show trasparency
	SetShaderColor(0.7, 0.7, 0.8, 0.3f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// === GLASS VASE - NECK - CYLINDER === //
	scaleXYZ = glm::vec3(0.5f, 0.45f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 5.45f, -0.65f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Call shader with glass material
	SetShaderMaterial("glass");

	// Setting color to bject to show transparency
	SetShaderColor(0.7, 0.7, 0.8, 0.3);

	// draw the mesh with transformation values
	// Drawn to only include body of cylinder, not top or bottom faces
	m_basicMeshes->DrawCylinderMesh(false, false, true);
	/****************************************************************/

	// === GLASS VASE - WATER - HALF-SPHERE === // 
	scaleXYZ = glm::vec3(0.90f, 0.9f, 0.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 4.9f, -0.65f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Call shader with glass material
	SetShaderMaterial("glass");

	// Color set to show transparency and blue-ish coloring
	SetShaderColor(0.83, 0.94, 0.976, 0.7f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/


	// === CERAMIC CONTAINER - LOWER BODY - CYLINDER === //
	scaleXYZ = glm::vec3(0.55f, 1.25f, 0.55f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.95f, 3.72f, 0.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Call shader with ceramic material
	SetShaderMaterial("ceramic");

	// Set shader texture to that of pottery
	SetShaderTexture("pottery");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// === CERAMIC CONTAINER - LID - CYLINDER === // 
	scaleXYZ = glm::vec3(0.57f, 0.05f, 0.57f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.95f, 4.96f, 0.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Call shader with caramic material
	SetShaderMaterial("ceramic");

	// Setting shader texture to that of pottery
	SetShaderTexture("pottery");
	SetTextureUVScale(0.5, 0.5);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// === FRUIT - SPHERE === //
	// Scaling sphere to appropriate size
	scaleXYZ = glm::vec3(0.35f, 0.35f, 0.35f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.95f, 4.0f, 0.70f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Call shader with leather material to replicate fruit's skin
	SetShaderMaterial("leather");
	
	// Setting shader texture to that of fruit, some shine
	SetShaderTexture("fruit");
	SetTextureUVScale(0.5, 0.5);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// === CANDLE - JAR - CYLINDER === // 
	scaleXYZ = glm::vec3(0.3f, 0.5f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 3.70f, 0.70f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Call shader with glass material
	SetShaderMaterial("glass");
	
	// Setting color to have transparency
	SetShaderColor(0.7, 0.7, 0.8, 0.4);

	// draw the mesh with transformation values
	// Only drawing body and lower face of clinder
	m_basicMeshes->DrawCylinderMesh(false, true, true);
	/****************************************************************/


	// === CANDLE - LABEL === //
	scaleXYZ = glm::vec3(0.305f, 0.305f, 0.305f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 3.75f, 0.70f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set texture to that of paper
	SetShaderTexture("paper");
	SetTextureUVScale(1.0, 1.0);

	// Call shader with parchment material
	SetShaderMaterial("parchment");

	// draw the mesh with transformation values
	// Drawing the body of the cylinder to leave out top and bottom face to mimic a label
	m_basicMeshes->DrawCylinderMesh(false, false, true);
	/****************************************************************/

	// === CANDLE - WAX	=== // 
	scaleXYZ = glm::vec3(0.29f, 0.35f, 0.29f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 3.70f, 0.70f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting texture to wax texture
	SetShaderTexture("wax");
	SetTextureUVScale(1.0, 1.0);

	// Call shader with ceramic material
	SetShaderMaterial("ceramic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// === CANDLE - WICK === // 
	// Scaled to be a much smaller size
	scaleXYZ = glm::vec3(0.03f, 0.12f, 0.03f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 4.05f, 0.70f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting texture to that of candle snuffer
	SetShaderTexture("candle snuffer");
	SetTextureUVScale(1.0, 1.0);

	// Call shader with metal material
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}
