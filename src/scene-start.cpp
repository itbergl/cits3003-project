
#include "Angel.h"


// Open Asset Importer header files (in ../../assimp--3.0.1270/include)
// This is a standard open source library for loading meshes, see gnatidread.h
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>

#if defined(__APPLE__) || defined(LAB_PC)
#include <dirent.h>
#define EXISTS opendir
#else
#include <filesystem>
#define EXISTS std::filesystem::exists
#endif

GLint windowHeight = 640, windowWidth = 960;

// gnatidread.cpp is the CITS3003 "Graphics n Animation Tool Interface & Data
// Reader" code.  This file contains parts of the code that you shouldn't need
// to modify (but, you can).
#include "gnatidread.h"

using namespace std; // Import the C++ standard functions (e.g., min)

// IDs for the GLSL program and GLSL variables.
GLuint shaderProgram;                 // The number identifying the GLSL shader program
GLuint vPosition, vNormal, vTexCoord; // IDs for vshader input vars (from glGetAttribLocation)
GLuint projectionU, modelViewU, viewU;       // IDs for uniform variables (from glGetUniformLocation)

static float viewDist = 1.5;          // Distance from the camera to the centre of the scene
static float camRotSidewaysDeg = 0;   // rotates the camera sideways around the centre
static float camRotUpAndOverDeg = 20; // rotates the camera up and over the centre.

mat4 projection; // Projection matrix - set in the reshape function
mat4 view;       // View matrix - set in the display function.

// These are used to set the window title
char lab[] = "Project1";
char *programName = NULL; // Set in main
int numDisplayCalls = 0;  // Used to calculate the number of frames per second

//------Meshes----------------------------------------------------------------
// Uses the type aiMesh from ../../assimp--3.0.1270/include/assimp/mesh.h
//                           (numMeshes is defined in gnatidread.h)
aiMesh *meshes[numMeshes]; // For each mesh we have a pointer to the mesh to draw
GLuint vaoIDs[numMeshes];  // and a corresponding VAO ID from glGenVertexArrays

// -----Textures--------------------------------------------------------------
//                           (numTextures is defined in gnatidread.h)
texture *textures[numTextures]; // An array of texture pointers - see gnatidread.h
GLuint textureIDs[numTextures]; // Stores the IDs returned by glGenTextures

//------Scene Objects---------------------------------------------------------
//
// For each object in a scene we store the following
// Note: the following is exactly what the sample solution uses, you can do things differently if you want.
typedef struct
{
    vec4 loc;
    float scale;
    float angles[3];                  // rotations around X, Y and Z axes.
    float diffuse, specular, ambient; // Amount of each light component
    float shine;
    vec3 rgb;
    float brightness; // Multiplies all colours
    int meshId;
    int texId;
    float texScale;
} SceneObject;

const int maxObjects = 1024; // Scenes with more than 1024 objects seem unlikely

bool lightactive[3] = {false,false,false}; //Spotlight on or off

SceneObject sceneObjs[maxObjects]; // An array storing the objects currently in the scene.
int nObjects = 0;                  // How many objects are currenly in the scene.
int currObject = -1;               // The current object
int toolObj = -1;                  // The object currently being modified
int removeObjectId;
int duplicateObjectId;
int lightMenuId;

//----------------------------------------------------------------------------
//
// Loads a texture by number, and binds it for later use.
void loadTextureIfNotAlreadyLoaded(int i)
{
    if (textures[i] != NULL)
        return; // The texture is already loaded.

    textures[i] = loadTextureNum(i);
    CheckError();
    glActiveTexture(GL_TEXTURE0);
    CheckError();

    // Based on: http://www.opengl.org/wiki/Common_Mistakes
    glBindTexture(GL_TEXTURE_2D, textureIDs[i]);
    CheckError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textures[i]->width, textures[i]->height,
                 0, GL_RGB, GL_UNSIGNED_BYTE, textures[i]->rgbData);
    CheckError();
    glGenerateMipmap(GL_TEXTURE_2D);
    CheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    CheckError();

    glBindTexture(GL_TEXTURE_2D, 0);
    CheckError(); // Back to default texture
}

//------Mesh loading----------------------------------------------------------
//
// The following uses the Open Asset Importer library via loadMesh in
// gnatidread.h to load models in .x format, including vertex positions,
// normals, and texture coordinates.
// You shouldn't need to modify this - it's called from drawMesh below.

void loadMeshIfNotAlreadyLoaded(int meshNumber)
{
    if (meshNumber >= numMeshes || meshNumber < 0)
    {
        printf("Error - no such model number");
        exit(1);
    }

    if (meshes[meshNumber] != NULL)
        return; // Already loaded

    aiMesh *mesh = loadMesh(meshNumber);
    meshes[meshNumber] = mesh;

#ifdef __APPLE__
    glBindVertexArrayAPPLE(vaoIDs[meshNumber]);
#else
    glBindVertexArray(vaoIDs[meshNumber]);
#endif

    // Create and initialize a buffer object for positions and texture coordinates, initially empty.
    // mesh->mTextureCoords[0] has space for up to 3 dimensions, but we only need 2.
    GLuint buffer[1];
    glGenBuffers(1, buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (3 + 3 + 3) * mesh->mNumVertices,
                 NULL, GL_STATIC_DRAW);

    int nVerts = mesh->mNumVertices;
    // Next, we load the position and texCoord data in parts.
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * nVerts, mesh->mVertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 3 * nVerts, sizeof(float) * 3 * nVerts, mesh->mTextureCoords[0]);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 6 * nVerts, sizeof(float) * 3 * nVerts, mesh->mNormals);

    // Load the element index data
    //GLuint elements[mesh->mNumFaces*3];
    std::vector<GLuint> elements = std::vector<GLuint>(mesh->mNumFaces * 3, 0);

    for (GLuint i = 0; i < mesh->mNumFaces; i++)
    {
        elements[i * 3] = mesh->mFaces[i].mIndices[0];
        elements[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
        elements[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
    }

    GLuint elementBufferId[1];
    glGenBuffers(1, elementBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh->mNumFaces * 3, elements.data(), GL_STATIC_DRAW);

    // vPosition it actually 4D - the conversion sets the fourth dimension (i.e. w) to 1.0
    glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vPosition);

    // vTexCoord is actually 2D - the third dimension is ignored (it's always 0.0)
    glVertexAttribPointer(vTexCoord, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(float) * 3 * mesh->mNumVertices));
    glEnableVertexAttribArray(vTexCoord);
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(float) * 6 * mesh->mNumVertices));
    glEnableVertexAttribArray(vNormal);
    CheckError();
}

//----------------------------------------------------------------------------

void zoomIn()
{
    viewDist = (viewDist < 0.0 ? viewDist : viewDist * 0.8) - 0.05;
}

void zoomOut()
{
    viewDist = (viewDist < 0.0 ? viewDist : viewDist * 1.25) + 0.05;
}

static void mouseClickOrScroll(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        if (glutGetModifiers() != GLUT_ACTIVE_SHIFT)
            activateTool(button);
        else
            activateTool(GLUT_LEFT_BUTTON);
    }
    else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
        deactivateTool();
    else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
    {
        activateTool(button);
    }
    else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_UP)
        deactivateTool();

    else if (button == 3)
    { // scroll up
        zoomIn();
    }
    else if (button == 4)
    { // scroll down
        zoomOut();
    }
}

//----------------------------------------------------------------------------

static void mousePassiveMotion(int x, int y)
{
    mouseX = x;
    mouseY = y;
}

//----------------------------------------------------------------------------

mat2 camRotZ()
{
    return rotZ(-camRotSidewaysDeg) * mat2(10.0, 0, 0, -10.0);
}

//------callback functions for doRotate below and later-----------------------

static void adjustCamrotsideViewdist(vec2 cv)
{
    //cout << cv << endl;
    camRotSidewaysDeg += cv[0];
    viewDist += cv[1];
}

static void adjustcamSideUp(vec2 su)
{
    camRotSidewaysDeg += su[0];
    camRotUpAndOverDeg += su[1];
}

static void adjustLocXZ(vec2 xz)
{
    sceneObjs[toolObj].loc[0] += xz[0];
    sceneObjs[toolObj].loc[2] += xz[1];
}

static void adjustScaleY(vec2 sy)
{
    sceneObjs[toolObj].scale += sy[0];
    sceneObjs[toolObj].loc[1] += sy[1];
}

//----------------------------------------------------------------------------
//------Set the mouse buttons to rotate the camera----------------------------
//------around the centre of the scene.---------------------------------------
//----------------------------------------------------------------------------

static void doRotate()
{
    setToolCallbacks(adjustCamrotsideViewdist, mat2(400, 0, 0, -2),
                     adjustcamSideUp, mat2(400, 0, 0, -90));
}




//------Add an object to the scene--------------------------------------------

static void addObject(int id)
{
//if adding  light
    if(56 <=id && id <= 58){
        sceneObjs[id-55].brightness = 0.2;
        sceneObjs[id-55].scale = 0.1;
        lightactive[id-56] = true;
        toolObj = currObject = id-55;
    }else{
        vec2 currPos = currMouseXYworld(camRotSidewaysDeg);
        sceneObjs[nObjects].loc[0] = currPos[0];
        sceneObjs[nObjects].loc[1] = 0.0;
        sceneObjs[nObjects].loc[2] = currPos[1];
        sceneObjs[nObjects].loc[3] = 1.0;

        sceneObjs[nObjects].angles[0] = 0.0;
        sceneObjs[nObjects].angles[1] = 180.0;
        sceneObjs[nObjects].angles[2] = 0.0;

        if (id != 0 && id != 55){
            sceneObjs[nObjects].scale = 0.005;
        }else{sceneObjs[nObjects].scale = 0.1; }

        sceneObjs[nObjects].rgb[0] = 0.7;
        sceneObjs[nObjects].rgb[1] = 0.7;
        sceneObjs[nObjects].rgb[2] = 0.7;
        sceneObjs[nObjects].brightness = 1.0;

        sceneObjs[nObjects].diffuse = 1.0;
        sceneObjs[nObjects].specular = 0.5;
        sceneObjs[nObjects].ambient = 0.7;
        sceneObjs[nObjects].shine = 10.0;

        sceneObjs[nObjects].meshId = id;
        sceneObjs[nObjects].texId = rand() % numTextures;
        sceneObjs[nObjects].texScale = 2.0;
        
        toolObj = currObject = nObjects++;
    }
    setToolCallbacks(adjustLocXZ, camRotZ(),adjustScaleY, mat2(0.05, 0, 0, 10.0));
    glutPostRedisplay();
}

//------The init function-----------------------------------------------------

void init(void)
{
    srand(time(NULL)); /* initialize random seed - so the starting scene varies */
    aiInit();

    // for (int i=0; i < numMeshes; i++)
    //     meshes[i] = NULL;

#ifdef __APPLE__
    glGenVertexArraysAPPLE(numMeshes, vaoIDs);
    CheckError(); // Allocate vertex array objects for meshes
#else
    glGenVertexArrays(numMeshes, vaoIDs);
    CheckError(); // Allocate vertex array objects for meshes
#endif

    glGenTextures(numTextures, textureIDs);
    CheckError(); // Allocate texture objects

    // Load shaders and use the resulting shader program
    shaderProgram = InitShader("res/shaders/vStart.glsl", "res/shaders/fStart.glsl");

    glUseProgram(shaderProgram);
    CheckError();

    // Initialize the vertex position attribute from the vertex shader
    vPosition = glGetAttribLocation(shaderProgram, "vPosition");
    vNormal = glGetAttribLocation(shaderProgram, "vNormal");
    CheckError();

    // Likewise, initialize the vertex texture coordinates attribute.
    vTexCoord = glGetAttribLocation(shaderProgram, "vTexCoord");
    CheckError();

    projectionU = glGetUniformLocation(shaderProgram, "Projection");
    modelViewU = glGetUniformLocation(shaderProgram, "ModelView");
    viewU = glGetUniformLocation(shaderProgram, "view");


    // Objects 0, and 1 are the ground and the first light.
    addObject(0); // Square for the ground
    sceneObjs[0].loc = vec4(0.0, 0.0, 0.0, 1.0);
    sceneObjs[0].scale = 10.0;
    sceneObjs[0].angles[0] = 90.0; // Rotate it.
    sceneObjs[0].texScale = 5.0;   // Repeat the texture.

    addObject(55); // Sphere for the first light
    sceneObjs[1].loc = vec4(2.0, 1.0, 1.0, 1.0);
    sceneObjs[1].scale = 0.0;
    sceneObjs[1].texId = 0;        // Plain texture
    sceneObjs[1].brightness = 0.0; // The light's brightness is 5 times this (below).

    addObject(55); // Sphere for the second light
    sceneObjs[2].loc = vec4(2.0, 1.0, 1.0, 1.0);
    sceneObjs[2].scale = 0.0;
    sceneObjs[2].texId = 0;        // Plain texture
    sceneObjs[2].brightness = 0.0; // The light's brightness is 5 times this (below).

    addObject(55); // Sphere for the third light
    sceneObjs[3].loc = vec4(2.0, 1.0, 1.0, 1.0);
    sceneObjs[3].scale = 0.0;
    sceneObjs[3].texId = 0;        // Plain texture
    sceneObjs[3].brightness = 0.0; // The light's brightness is 5 times this (below).
    addObject(rand() % numMeshes); // A test mesh

    // We need to enable the depth test to discard fragments that
    // are behind previously drawn fragments for the same pixel.
    glEnable(GL_DEPTH_TEST);
    doRotate();                       // Start in camera rotate mode.
    glClearColor(0.0, 0.0, 0.0, 1.0); /* black background */
}

//----------------------------------------------------------------------------

void drawMesh(SceneObject sceneObj)
{

    // Activate a texture, loading if needed.
    loadTextureIfNotAlreadyLoaded(sceneObj.texId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIDs[sceneObj.texId]);

    // Texture 0 is the only texture type in this program, and is for the rgb
    // colour of the surface but there could be separate types for, e.g.,
    // specularity and normals.
    glUniform1i(glGetUniformLocation(shaderProgram, "texture"), 0);

    // Set the texture scale for the shaders
    glUniform1f(glGetUniformLocation(shaderProgram, "texScale"), sceneObj.texScale);

    // Set the projection matrix for the shaders
    glUniformMatrix4fv(projectionU, 1, GL_TRUE, projection);


    // Set the model matrix - this should combine translation, rotation and scaling based on what's
    // in the sceneObj structure (see near the top of the program).

    mat4 model = Translate(sceneObj.loc) * Scale(sceneObj.scale) * RotateX(sceneObj.angles[0]) * RotateY(sceneObj.angles[1]);

    // Set the model-view matrix for the shaders
    glUniformMatrix4fv(modelViewU, 1, GL_TRUE, view * model);

    // Activate the VAO for a mesh, loading if needed.
    loadMeshIfNotAlreadyLoaded(sceneObj.meshId);
    CheckError();
#ifdef __APPLE__
    glBindVertexArrayAPPLE(vaoIDs[sceneObj.meshId]);
#else
    glBindVertexArray(vaoIDs[sceneObj.meshId]);
#endif
    CheckError();

    glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3,
                   GL_UNSIGNED_INT, NULL);
    CheckError();
}

//----------------------------------------------------------------------------

void display(void)
{
    numDisplayCalls++;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckError(); // May report a harmless GL_INVALID_OPERATION with GLEW on the first frame

    // Set the view matrix. 

    view = Translate(0.0, 0.0, -viewDist) * RotateX(camRotUpAndOverDeg) * RotateY(camRotSidewaysDeg);

    vec4 lightPosition[3];
    lightPosition[0] = view * sceneObjs[1].loc;
    lightPosition[1] = view * sceneObjs[2].loc;
    lightPosition[2] = view * sceneObjs[3].loc;

    glUniform4fv(glGetUniformLocation(shaderProgram, "LightPositionArray"),
                 3, *lightPosition);

    GLfloat lightBrightness[3];
    lightBrightness[0] = sceneObjs[1].brightness;
    lightBrightness[1] = sceneObjs[2].brightness;
    lightBrightness[2] = sceneObjs[3].brightness;

    vec3 lightRGB[3];
    lightRGB[0] = sceneObjs[1].rgb;
    lightRGB[1] = sceneObjs[2].rgb;
    lightRGB[2] = sceneObjs[3].rgb;

    glUniform3fv(glGetUniformLocation(shaderProgram, "LightRGBrray"),
                 3, *lightRGB);

    glUniform1fv(glGetUniformLocation(shaderProgram, "LightBrightnessArray"),
                 3, lightBrightness);
    glUniformMatrix4fv(viewU, 1, GL_TRUE, view);
    
    CheckError();

    for (int i = 0; i < nObjects; i++)
    {
        SceneObject so = sceneObjs[i];

        vec3 rgb = so.rgb * so.brightness;

        glUniform3fv(glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, so.ambient * rgb);
        CheckError();
        glUniform3fv(glGetUniformLocation(shaderProgram, "DiffuseProduct"), 1, so.diffuse * rgb);
        glUniform3fv(glGetUniformLocation(shaderProgram, "SpecularProduct"), 1, so.specular * rgb);
        glUniform1f(glGetUniformLocation(shaderProgram, "Shininess"), so.shine);
        CheckError();

        drawMesh(sceneObjs[i]);
    }

    glutSwapBuffers();
}

//----------------------------------------------------------------------------
//------Menus-----------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void updateMenu(){
    deactivateTool();

// remove all menu items from duplicate, remove menus and spotlight menus
    glutSetMenu(duplicateObjectId);
    while(glutGet(GLUT_MENU_NUM_ITEMS) != 0){
        glutRemoveMenuItem(1);
    }

    glutSetMenu(removeObjectId); 
    while(glutGet(GLUT_MENU_NUM_ITEMS) != 0){
        glutRemoveMenuItem(1);
    }
    glutSetMenu(lightMenuId);
    while(glutGet(GLUT_MENU_NUM_ITEMS) != 0){
        glutRemoveMenuItem(1);
    }

//if light is on, add it to remove menu and light menus
    if(lightactive[0]){
        glutSetMenu(removeObjectId);
        glutAddMenuEntry(" 56 Traditional Light",1);
        glutSetMenu(lightMenuId);
        glutAddMenuEntry("Move Light 1", 70);
        glutAddMenuEntry("R/G/B/All Light 1", 71);
    }
    if(lightactive[1]){

        glutSetMenu(removeObjectId);
        glutAddMenuEntry(" 57 Directional Light",2);
        glutSetMenu(lightMenuId);
        glutAddMenuEntry("Move Light 2 (Directional)", 80);
        glutAddMenuEntry("R/G/B/All Light 2", 81);
    }
    if(lightactive[2]){
        glutSetMenu(removeObjectId);
        glutAddMenuEntry(" 58 Spotlight",3);
        glutSetMenu(lightMenuId);
        glutAddMenuEntry("Move Light 3 (Spotlight)", 90);
        glutAddMenuEntry("R/G/B/All Light 3", 91);
    }

// add all current objects to both duplicate and remove menus
    int repeats[numMeshes+1] ={0};
    for (int i =4; i < nObjects; i++){
        char menuName[128] = " ";
        char a[numMeshes*4] = " ";
        ++ repeats[sceneObjs[i].meshId];
        strcat(menuName,objectMenuEntries[sceneObjs[i].meshId-1]);

//if multiple of the same items, give unique indicator
        if(repeats[sceneObjs[i].meshId] >1 ){
            sprintf(a, "%d",repeats[sceneObjs[i].meshId]);
            strcat(strcat(strcat(menuName, " ("), a), ")");
        }
        glutSetMenu(removeObjectId);
        glutAddMenuEntry(menuName,i);
        glutSetMenu(duplicateObjectId);
        glutAddMenuEntry(menuName,i);
    }
    glutPostRedisplay();
}


///TODO (maybe?)
static void duplicateObject(int id){
    deactivateTool();
    addObject(sceneObjs[id].meshId);
    sceneObjs[nObjects-1].texId =sceneObjs[id].texId;
    for(int i=0; i <2;i++){
        sceneObjs[nObjects-1].angles[i] = sceneObjs[id].angles[i];
    }
    sceneObjs[nObjects-1].scale =sceneObjs[id].scale;
    updateMenu();
}

static void removeObject(int id){
    deactivateTool();

//if removing lights
    if( 1<= id && id <=3){
        lightactive[id-1] = false;
        sceneObjs[id].scale =0.0;
        sceneObjs[id].brightness =0.0;
        updateMenu();
        return;
    }

    int j = 4;
    for(int i = 4; i <maxObjects; i++){
        if(i ==id){
            j++;
        }
        sceneObjs[i] = sceneObjs[j];
        j++;
    }
    toolObj = currObject = nObjects--;
    updateMenu();
}



static void objectMenu(int id)
{
    deactivateTool();
    addObject(id);
    updateMenu();
    
}

static void texMenu(int id)
{
    deactivateTool();
    if (currObject >= 0)
    {
        sceneObjs[currObject].texId = id;
        glutPostRedisplay();
    }
}

static void groundMenu(int id)
{
    deactivateTool();
    sceneObjs[0].texId = id;
    glutPostRedisplay();
}

static void adjustBrightnessY(vec2 by)
{
    if (sceneObjs[toolObj].brightness + by[0] > 0.0) {
        sceneObjs[toolObj].brightness += by[0];
    }

    sceneObjs[toolObj].brightness = max(sceneObjs[toolObj].brightness, (float)0.0);//
    sceneObjs[toolObj].loc[1] += by[1];
}

static void adjustRedGreen(vec2 rg)
{
    sceneObjs[toolObj].rgb[0] += rg[0];
    sceneObjs[toolObj].rgb[1] += rg[1];
}

static void adjustBlueBrightness(vec2 bl_br)
{
    sceneObjs[toolObj].rgb[2] += bl_br[0];
    sceneObjs[toolObj].brightness += bl_br[1];
}

static void lightMenu(int id)
{
    deactivateTool();
    if (id == 70)
    {
        toolObj = 1;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2(1.0, 0.0, 0.0, 10.0));
    }
    else if (id >= 71 && id <= 74)
    {
        toolObj = 1;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0));
    }
    else if (id == 80)
    {
        toolObj = 2;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2(1.0, 0.0, 0.0, 10.0));
    }
    else if (id >= 81 && id <= 84)
    {
        toolObj = 2;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0));
    }
    else if (id == 90)
    {
        toolObj = 3;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2(1.0, 0.0, 0.0, 10.0));
    }
    else if (id >= 91 && id <= 94)
    {
        toolObj = 3;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0));
    }
    else
    {
        printf("Error in lightMenu\n");
        exit(1);
    }
}

static int createArrayMenu(int size, const char menuEntries[][128], void (*menuFn)(int))
{
    int nSubMenus = (size - 1) / 10 + 1;
    //int subMenus[nSubMenus];
    std::vector<int> subMenus = std::vector<int>(nSubMenus, 0);

    for (int i = 0; i < nSubMenus; i++)
    {
        subMenus[i] = glutCreateMenu(menuFn);
        for (int j = i * 10 + 1; j <= min(i * 10 + 10, size); j++)
            glutAddMenuEntry(menuEntries[j - 1], j);
        CheckError();
    }
    int menuId = glutCreateMenu(menuFn);

    for (int i = 0; i < nSubMenus; i++)
    {
        char num[6];
        sprintf(num, "%d-%d", i * 10 + 1, min(i * 10 + 10, size));
        glutAddSubMenu(num, subMenus[i]);
        CheckError();
    }
    return menuId;
}

//CHANGE POSITION TODO + x and y arbitary (rememeber what these do)
static void adjustDiffuse(vec2 ad)
{
    sceneObjs[toolObj].ambient += ad[0];
    sceneObjs[toolObj].diffuse += ad[1];
}

static void adjustShine(vec2 sh)
{
    sceneObjs[toolObj].specular += sh[0];
    sceneObjs[toolObj].shine += sh[1];
}

static void materialMenu(int id)
{
    deactivateTool();
    if (currObject < 0)
        return;
    if (id == 10)
    {
        toolObj = currObject;
        setToolCallbacks(adjustRedGreen, mat2(1, 0, 0, 1),
                         adjustBlueBrightness, mat2(1, 0, 0, 1));
    }
    if (id == 20)
    {
        toolObj = currObject;
        setToolCallbacks(adjustDiffuse, mat2(1, 0, 0, 1),
                         adjustShine, mat2(1, 0, 0, 50));
    }
    else
    {
        printf("Error in materialMenu\n");
    }
}



static void adjustAngleYX(vec2 angle_yx)
{
    sceneObjs[currObject].angles[1] += angle_yx[0];
    sceneObjs[currObject].angles[0] += angle_yx[1];
}

static void adjustAngleZTexscale(vec2 az_ts)
{
    sceneObjs[currObject].angles[2] += az_ts[0];
    sceneObjs[currObject].texScale += az_ts[1];
}

static void mainmenu(int id)
{
    deactivateTool();
    if (id == 41 && currObject >= 0)
    {
        toolObj = currObject;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustScaleY, mat2(0.05, 0, 0, 10));
    }
    if (id == 50)
        doRotate();
    if (id == 55 && currObject >= 0)
    {
        setToolCallbacks(adjustAngleYX, mat2(400, 0, 0, -400),
                         adjustAngleZTexscale, mat2(400, 0, 0, 15));
    }
    if (id == 99)
        exit(0);
}




static void makeMenu()
{
    int objectId = createArrayMenu(numMeshes, objectMenuEntries, objectMenu);

    int materialMenuId = glutCreateMenu(materialMenu);
    glutAddMenuEntry("R/G/B/All", 10);
    glutAddMenuEntry("Ambient/Diffuse/Specular/Shine", 20);

    int texMenuId = createArrayMenu(numTextures, textureMenuEntries, texMenu);
    int groundMenuId = createArrayMenu(numTextures, textureMenuEntries, groundMenu);

    lightMenuId = glutCreateMenu(lightMenu);


    removeObjectId = glutCreateMenu(removeObject);
    duplicateObjectId = glutCreateMenu(duplicateObject);
	updateMenu();

    glutCreateMenu(mainmenu);
    glutAddMenuEntry("Rotate/Move Camera", 50);
    glutAddSubMenu("Add object", objectId);
    glutAddSubMenu("Remove object",removeObjectId);
    glutAddSubMenu("Duplicate object", duplicateObjectId);
    glutAddMenuEntry("Position/Scale", 41);
    glutAddMenuEntry("Rotation/Texture Scale", 55);
    glutAddSubMenu("Material", materialMenuId);
    glutAddSubMenu("Texture", texMenuId);
    glutAddSubMenu("Ground Texture", groundMenuId);
    glutAddSubMenu("Lights", lightMenuId);
    glutAddMenuEntry("EXIT", 99);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}



//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 033:
    {
        exit(EXIT_SUCCESS);
        break;
    }
    case 'w':
    {
        if (glutGetModifiers() == GLUT_ACTIVE_ALT)
        { // up + alt
            zoomIn();
        }
        break;
    }
    case 's':
    {
        if (glutGetModifiers() == GLUT_ACTIVE_ALT)
        { // down + alt
            zoomOut();
        }
        break;
    }
    }
}

void specialKeys(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_UP:
    {
        if (glutGetModifiers() == GLUT_ACTIVE_ALT)
        { // up + alt
            zoomIn();
        }
        break;
    }
    case GLUT_KEY_DOWN:
    {
        if (glutGetModifiers() == GLUT_ACTIVE_ALT)
        { // down + alt
            zoomOut();
        }
        break;
    }
    }
}
//----------------------------------------------------------------------------

void idle(void)
{
    glutPostRedisplay();
}

//----------------------------------------------------------------------------

void reshape(int width, int height)
{
    windowWidth = width;
    windowHeight = height;

    glViewport(0, 0, width, height);

    // You'll need to modify this so that the view is similar to that in the
    // sample solution.
    // In particular:
    //     - the view should include "closer" visible objects (slightly tricky)
    //     - when the width is less than the height, the view should adjust so
    //         that the same part of the scene is visible across the width of
    //         the window.

    GLfloat nearDist = 0.01;
    if ((float)width < (float)height)
    {
        projection = Frustum(-nearDist, nearDist,
                             -nearDist * (float)height / (float)width, nearDist * (float)height / (float)width,
                             nearDist, 100.0);
    }
    else
    {
        projection = Frustum(-nearDist * (float)width / (float)height, nearDist * (float)width / (float)height,
                             -nearDist, nearDist, nearDist, 100.0);
    }
}

//----------------------------------------------------------------------------

void timer(int unused)
{
    char title[256];
    sprintf(title, "%s %s: %d Frames Per Second @ %d x %d",
            lab, programName, numDisplayCalls, windowWidth, windowHeight);

    glutSetWindowTitle(title);

    numDisplayCalls = 0;
    glutTimerFunc(1000, timer, 1);
}

//----------------------------------------------------------------------------

char dirDefault1[] = "res/models-textures";
char dirDefault3[] = "/tmp/models-textures";
char dirDefault4[] = "/d/models-textures";
char dirDefault2[] = "/cslinux/examples/CITS3003/project-files/models-textures";

void fileErr(char *fileName)
{
    printf("Error reading file: %s\n", fileName);
    printf("When not in the CSSE labs, you will need to include the directory containing\n");
    printf("the models on the command line, or put it in the res folder next to the executable.");
    exit(1);
}

//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    // Get the program name, excluding the directory, for the window title
    programName = argv[0];
    for (char *cpointer = argv[0]; *cpointer != 0; cpointer++)
        if (*cpointer == '/' || *cpointer == '\\')
            programName = cpointer + 1;

    // Set the models-textures directory, via the first argument or some handy defaults.
    if (argc > 1)
        strcpy(dataDir, argv[1]);
    else if (EXISTS(dirDefault1))
        strcpy(dataDir, dirDefault1);
    else if (EXISTS(dirDefault2))
        strcpy(dataDir, dirDefault2);
    else if (EXISTS(dirDefault3))
        strcpy(dataDir, dirDefault3);
    else if (EXISTS(dirDefault4))
        strcpy(dataDir, dirDefault4);
    else
        fileErr(dirDefault1);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);

#if !defined(__APPLE__) && !defined(LAB_PC)
    glutInitContextVersion(3, 2);
    glutInitContextProfile(GLUT_CORE_PROFILE);
#endif

    glutCreateWindow("Initialising...");

#ifndef __APPLE__
    glewInit(); // With some old hardware yields GL_INVALID_ENUM, if so use glewExperimental.
#endif

    CheckError(); // This bug is explained at: http://www.opengl.org/wiki/OpenGL_Loading_Library

    init();
    CheckError();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idle);

    glutMouseFunc(mouseClickOrScroll);
    glutPassiveMotionFunc(mousePassiveMotion);
    glutMotionFunc(doToolUpdateXY);

    glutReshapeFunc(reshape);
    glutTimerFunc(1000, timer, 1);
    CheckError();

    makeMenu();
    CheckError();

    glutMainLoop();
    return 0;
}
