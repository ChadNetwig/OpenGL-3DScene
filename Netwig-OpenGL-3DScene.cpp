//==================================================================================================
// Filename      : Netwig-OpenGL-3D Scene.cpp
// Author        : Chad Netwig
// Last Updated  : 02/14/2022
//               :
// Description   : Renders a three-dimensional scene with the following four textured objects,
//               : which rest upon a textured 3D plane:
//               :
//               :    1. Tri-case textured in black matte with Optic Chicago logo
//               :    2. Cylinder can of La Croix sparking water with Peach-Pear La Croix texture
//               :    3. Sphere foam ball textured with blue foam texture
//               :    4. Cubic sticky notes with a yellow paper texture and writing
//               : 
//               : For the sphere and cylinder objects in the scene, open source code Cylinder.h,
//               : Sphere.h, Cylinder.cpp, and Sphere.cpp by author Song Ho Ahn, has been used to
//               : create the vertices, normals, and texture coordinates. Changes have been made
//               : to this source in order to make it compatible with the vertex/index structure
//               : of the custom GLObject class.
//               :
//               : The GLObject class is a custom class that includes methods for creating the
//               : meshes and textures of the objects mentioned above, including the sphere and
//               : cylinder. This class is instantiated for each each object, respectively.
//               :
//               : The textures for the objects are located in the 'images/' folder of the project
//               : and each texture is a constant char pointer that holds the name of the file,
//               : such as, const char* texFilename1 = "images/white-marble-plane-500x500.jpg";
//               :
//               : The 3D scene is illuminated using two light sources with the Phong reflection
//               : model, which is caluculated in the fragmentShaderSource shader code.
//               : Specifically ambient, diffuse, and specular lighting values are added
//               : together and multiplied with the object texture's colors to create a final 
//               : color in the shader, such that
//               :    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;
//               :
//               : The lights are represented as a fixed, white cubes in the scene, but they can
//               : orbit if 'true' is passed to the GLObject.Render() function in main()
//               : 
//               : The light's color can be controlled by setting the gLightColor.r, gLightColor.r, 
//               : gLightColor.b parameters in the main() function.
// 
//               : The main light (MainLight object) is white point light set at 100% intensity
//               : with values R=255, G=255, B=255 and is placed high above the 3D scene toward
//               : the front of the camera view, which creates diffuse lighting of the overall
//               : front of the world.
//               : The second light is a fill light (FillLight object) set as an offwhite color
//               : at 20% intensity, and is placed to the right center of the world scene, which
//               : illuminates the objects from the side.
//               :
//               : Separate vertex and fragment shaders exist for the lamp object, which takes on
//               : the color of the gLightColor with the addition of a uniform vec3 lightColor
//               : within the lampFragmentShaderSource final fragment output of
//               : "fragmentColor = vec4(lightColor, 1.0);"
//               : 
//               : Each frame processes keyboard and mouse input using UProcessInput(),
//               : which allows for panning, zooming, and orbiting the 3D scene, using the
//               : keyboard, mouse, and movement combinations:
//               :
//               : WASD keys    : Control the forward, backward, left, and right motion
//               : QE keys      : Control the upwardand downward movement
//               : P key        : Toggles between perspective and orthographic views
//               : Mouse cursor : Changes the orientation of the camera so it can look up 
//               :                and down or right and left
//               : Mouse scroll : Adjusts the speed of the movement, or the speed the camera
//               :              : travels around the scene
//               : 
//               : Uses Modern OpenGL ver. 4.x
//               :
//               : Comments have been added throughout the code to document functionality,
//               : which are preceded by 'CLN:'
//               : 
//==================================================================================================

#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

// GLM Math Header inclusions (used for matrix transformations)
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>           // CLN: Added to handle cylinder and sphere vertices and indices
#include "Cylinder.h"       // CLN: Header file from open source author Song Ho Ahn
#include "Sphere.h"         // CLN: Header file from open source author Song ho Ahn

// CLN: Include LearnOpenGL's Camera class in the working directory
#include "camera.h"

// CLN: [Texture] Added image loading library to handle various image formats for texturing objects
#define STB_IMAGE_IMPLEMENTATION    // CLN: the preprocessor modifies stb_image.h such that it only contains the relevant definition source code
#include "stb_image.h"              // CLN: Image loading library of utility functions


// CLN: [Texture] const for the filenames of the texture images to use on the objects
const char* texFilename1 = "images/white-marble-plane-500x500.jpg";
const char* texFilename2 = "images/tricase-blacktexture-325x325.jpg";
const char* texFilename3 = "images/optic-chicago-logo-325x325.png";
const char* texFilename4 = "images/LaCroix-texture-cropped-mirrored-1228-1800-doubled.jpg";
const char* texFilename5 = "images/blue-foam-texture-600x400.jpg";
const char* texFilename6 = "images/yellow-paper-texture-writing.jpg";


using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Chad Netwig - Project One - 3D Scene"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 1024;
    const int WINDOW_HEIGHT = 768;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handles for the vertex buffer objects
        GLuint nIndices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // GLMesh gMesh;

    // CLN: [Lighting] Used to scale texture (but not needed for this 3D scene)
    //glm::vec2 gUVScale(5.0f, 5.0f);

    // Shader program
    GLuint gProgramId;
    // CLN: [Lighting] Added shader for light source
    GLuint gLampProgramId;

    // CLN: setup Camera targeting the origin and zoomed out from scene to see entire 3D scene
    Camera gCamera(glm::vec3(2.0f, 2.0f, 12.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;
}

// CLN: [Lighting] Added colors for the light and object
glm::vec3 gObjectColor(1.0f, 1.0f, 1.0f); // CLN: Color for the object (r=255, g=255, b=255)
glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);  // CLN: Color for the lamp (r=255, g=255, b=255)

// CLN: [Lighting] Added position and scale for the light object (used for light orbiting)
// Light position and scale
glm::vec3 gLightPosition(1.5f, 2.0f, 10.0f);
glm::vec3 gLightScale(0.3f);

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


// ------------------------------------------------
// GLSL source code for vertex and fragment shaders
// ------------------------------------------------
/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
    // CLN: [Lighting] Added layout for normals
    layout(location = 1) in vec3 normal;   // VAP position 1 for normals
    layout(location = 2) in vec2 textureCoordinate;  // CLN: [Texture] Added a location with two vectors for the texture coordinates
    
    // CLN: [Lighting] Added vec3's for vertexNormal and vertexFragmentPos
    out vec3 vertexNormal;              // For outgoing normals to fragment shader
    out vec3 vertexFragmentPos;         // For outgoing color / pixels to fragment shader
    out vec2 vertexTextureCoordinate;   // CLN: [Texture] Outputs the two texture vectors that will be input for the fragment shader

    //Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
    
    // CLN: [Lighting] Added vertexFragmentPos and vertexNormal
    vertexFragmentPos = vec3(model * vec4(position, 1.0f));  // Gets fragment / pixel position in world space only (exclude view and projection)
    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    
    vertexTextureCoordinate = textureCoordinate;  // CLN: [Texture] Handles texture coordinate assignment
}
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    
    // CLN: [Lighting] Added two vec3's for vertextNormal and vertexFragmentPos
    in vec3 vertexNormal; // For incoming normals
    in vec3 vertexFragmentPos; // For incoming fragment position

    in vec2 vertexTextureCoordinate; // CLN: [Texture] Added to handle texture coord. input from vertex shader above

    out vec4 fragmentColor;

    // CLN: [Lighting] Added uniforms for objectColor, lightColor, lightPos, viewPosition, and uvScale
    // Uniform / Global variables for object color, light color, light position, and camera/view position
    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;
    //uniform vec2 uvScale;
    // CLN: [Texture] added uniform of sampler2D tyupe to handle the texture image
    uniform sampler2D uTextureBase;


void main()
{
    // CLN: [Lighting] Added the following code for Phong lighting model calcs
    //      Changed uTexture to uTextureBase to match previous code variable

    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.1f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.8f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    //vec4 textureColor = texture(uTextureBase, vertexTextureCoordinate * uvScale);
    
    // CLN: [Lighting] Removed uvScale
    vec4 textureColor = texture(uTextureBase, vertexTextureCoordinate);
    
    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;
    
    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    
    //fragmentColor = vec4(vertexColor);
    //fragmentColor = texture(uTextureBase, vertexTextureCoordinate); // CLN: Notice, not passing vertexColor because now using a texture
}
);


// CLN: [Lighting] Added shader sources for lampVertexShaderSource and lampFragmentShaderSource shaders
/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

   //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

    // CLN: Added lightColor uniform to pass light color to lamp object
    uniform vec3 lightColor;

void main()
{
    //fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
    //CLN: passed lightColor to fragment with alpha 1.0
    fragmentColor = vec4(lightColor, 1.0); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);


//-------------------------------------------------------
// CLN: Added variables code to control projection matrix
//-------------------------------------------------------
// CLN: Sets Projection matrix as global, which is used in URender() and modified in UProcessInput() when the 'P' key is pressed
glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f); // CLN: Creates a perspective projection matrix
unsigned int PCount = 0; // CLN: Uses this to determine if the 'P' key has been pressed an even or odd amount of times. Odd = Orthographic view, Even = Perspective view


//--------------------------------------------------------------------
// CLN: Class for creating meshes, rendering, and texturing 3D objects
//--------------------------------------------------------------------
class GLObject {
public:
    // Create a GLMesh struct, which contains the the GLuint's for the VBO, VAO, and Indices count for each object instance
    GLMesh mesh;
    // CLN: [Texture] Added texture id for the object instance
    GLuint gTextureId;
   
    // CLN: [Lighting] Added angularVelocity and cameraPosition const
    const float angularVelocity = glm::radians(45.0f);
    const glm::vec3 cameraPosition = gCamera.Position;

    // CLN: Default constructor
    GLObject() {
        mesh.nIndices = 0;
        mesh.vao = 0;
        mesh.vbos[0] = { 0 }; // CLN: initialize array with zeros
        mesh.vbos[1] = { 0 };
        gTextureId = 0;
    }

    // Functioned called to render a frame
    // CLN: Updated Render to include lamp bool and r, g, b values for lamp color
    void Render(glm::mat4 scale, glm::mat4 rotation, glm::mat4 translation, bool lamp, bool orbit)
    {
        // Enable z-depth. This is used with the fragement shader whenever the fragment shader wants to output its color
        // If the current fragment is behind the other fragment, the color is discarded, otherwise it is written.
        glEnable(GL_DEPTH_TEST);

        // CLN: camera (view) transformation matrix
        glm::mat4 view = gCamera.GetViewMatrix();

        // CLN: Model matrix: transformations are applied right-to-left order due to matrix multiplication non-commutative
        glm::mat4 model = translation * rotation * scale;

        if (!lamp)
        {
            // CLN: NOTE, the projection matrix is set as a global variable above, which is "toggled" perspective/orthographic through UProcessInput() by pressing 'P'

            // Set the shader to be used
            glUseProgram(gProgramId);

            // Retrieves and passes transform matrices to the Shader program
            GLint modelLoc = glGetUniformLocation(gProgramId, "model");
            GLint viewLoc = glGetUniformLocation(gProgramId, "view");
            GLint projLoc = glGetUniformLocation(gProgramId, "projection");

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

            // CLN: [Lighting] Added objectColorLoc, lightColorLoc, LightPositionLoc, and vewPositionLoc
            // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
            GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
            GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
            GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
            GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

            // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
            glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
            glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
            glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

            glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

            // CLN: [Lighting] UVScaleLoc (removed because scales texture, which isn't needed for the 3D scene)
            // GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
            // glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

            // CLN: Activate the VBOs contained within the mesh's VAO
            glBindVertexArray(mesh.vao);

            // CLN: [Texture] bind textures on corresponding texture units
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gTextureId);

            // CLN:Draws the 3D object
            glDrawElements(GL_TRIANGLES, mesh.nIndices, GL_UNSIGNED_SHORT, NULL);

            // CLN: Deactivate the Vertex Array Object
            glBindVertexArray(0);
        }
        else
        {
            // CLN: [Lighting] Added Lamp draw code
            // LAMP: draw lamp
            //-------------------------------------
            glUseProgram(gLampProgramId);
            
            // CLN: [Lighting] Lamp orbits around the origin if orbit is true
            if (orbit) {    
                glm::vec4 newPosition = glm::rotate(angularVelocity * gDeltaTime * 2, glm::vec3(0.0f, 2.0f, 0.0f)) * glm::vec4(gLightPosition, 1.0f);
                  gLightPosition.x = newPosition.x;
                  gLightPosition.y = newPosition.y;
                  gLightPosition.z = newPosition.z;

                //Transform the smaller cube used as a visual que for the light source
                model = glm::translate(gLightPosition) * glm::scale(gLightScale);
            }

            // Reference matrix uniforms from the Lamp Shader program
            GLint modelLoc = glGetUniformLocation(gLampProgramId, "model");
            GLint viewLoc = glGetUniformLocation(gLampProgramId, "view");
            GLint projLoc = glGetUniformLocation(gLampProgramId, "projection");

            // Pass matrix data to the Lamp Shader program's matrix uniforms
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

            // CLN: Added lightColor uniform to fragment shader to pass along the r, g, b colors that the lamp is emitting
            GLint lightColorLoc = glGetUniformLocation(gLampProgramId, "lightColor");
            glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);

            // CLN: Activate the VBOs contained within the mesh's VAO
            glBindVertexArray(mesh.vao);

            // CLN: [Texture] bind textures on corresponding texture units
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gTextureId);

            // CLN: [Lighting] Changed to glDrawElements
            glDrawElements(GL_TRIANGLES, mesh.nIndices, GL_UNSIGNED_SHORT, NULL);

            // CLN: [Lighting] Deactivate shader program
            glUseProgram(0);
        }
        // CLN: Moved swap buffers to main() under the rendering of objects to prevent "flickering"
        // glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
    }

    // Implements the UCreateMesh function
    void CreateMesh(GLfloat &objVertices, size_t verts, GLushort &objIndices, size_t indices)
    {
        const GLuint floatsPerVertex = 3; // CLN: this is x, y, and z coordinates
        // CLN: [Lighting] Added floatsPerNormal for three additional vertices in stride for the x, y, z normals
        const GLuint floatsPerNormal = 3;
        // CLN: [Texture] Added two vertices for texture
        const GLuint floatsPerUV = 2;

        glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
        glBindVertexArray(mesh.vao);

        // Create 2 buffers: first one for the vertex data; second one for the indices
        glGenBuffers(2, mesh.vbos);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // CLN: Activates the buffer
        glBufferData(GL_ARRAY_BUFFER, verts, &objVertices, GL_STATIC_DRAW); // CLN: Sends vertex or coordinate data to the GPU

        // CLN: Used for debugging
         std::cout << "Number of Vertices: " << verts << std::endl;

        mesh.nIndices = indices / sizeof(GLushort);   // CLN: Calculates the total number of indicies
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);    // CLN: Activates the buffer for the indicies
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices, &objIndices, GL_STATIC_DRAW); // CLN: Sends vertex or coordinate data to the GPU

        // CLN: [Texture] Updated stride to accomodate two vertices for texture. Strides between vertex coordinates is 6 (x, y, z, r, g, b, a, s, t). A tightly packed stride is 0.
        // CLN: [Lighting] Updated stride to include offset for floatsPerNormal. Strides between vertex coordinates is 5 (x, y, z, nx, ny, nz, s, t).
        GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

        // Create Vertex Attribute Pointers
        glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
        glEnableVertexAttribArray(0);

        // CLN: [Lighting] Added floatsPerNormal to stride
        glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
        glEnableVertexAttribArray(1);

        // CLN: [Texture] Added 2 to the stride to handle texture vertices offset, and removed floatsPerColor from stride (texture now used instead of color)
        // CLN: [Lighting] Changed buffer 1 to buffer 2 and added (floatsPerVertex + floatsPerNormal)
        glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
        glEnableVertexAttribArray(2);

    }

        // CLN: [Texture] Load and create the texture
        // ------------------------------------------
        bool CreateTexture(const char* filename, GLuint & textureId)
        {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);     // CLN: Used the stbi library function to flip about the y-axis instead of the flipImageVertically custom function
        unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
        if (image)
        {
            //flipImageVertically(image, width, height, channels);

            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);    // CLN: Binds the texture

            // set the texture wrapping parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // set texture filtering parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            if (channels == 3)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
            else if (channels == 4) // CLN: The fourth channel is alpha for image formats that support transparency (i.e., .png files)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
            else
            {
                cout << "Not implemented to handle image with " << channels << " channels" << endl;
                return false;
            }

            glGenerateMipmap(GL_TEXTURE_2D);

            stbi_image_free(image);
            glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture
            
            cout << filename << " loaded successfully!" << endl;
            return true;
        }

        // Error loading the image
        return false;
    }


    void DestroyTexture(GLuint textureId)
    {
        glGenTextures(1, &textureId);
    }

    void DestroyMesh(GLMesh& mesh)
    {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(2, mesh.vbos);
    }

};


// ---------------------------------------------------------------------------------
// CLN: Declares arrays to hold vertices and indices for all objects in the 3D scene
// ---------------------------------------------------------------------------------

// CLN: Plane vertices
GLfloat PlaneVertices[] = {
    // Vertex Positions         // Normals (nx, ny, nz)   // Texture (s, t) // Colors (r,g,b,a) (removed)

     // CLN: 3D Plane for Scene
    -2.0f, 0.0f,  2.0f,         0.0f,  0.0f, 1.0f,        0.0f, 1.0f,      // 0.80f, 0.55f, 0.06f, 1.0f,    // Upper left           / Vertex 0 of plane        / color = brown     / upper left texture
     2.0f, 0.0f,  2.0f,         0.0f,  0.0f, 1.0f,        1.0f, 1.0f,      // 0.80f, 0.55f, 0.06f, 1.0f,    // Upper right          / Vertex 1 of plane        / color = brown     / upper right texture
    -2.0f, 0.0f, -2.0f,         0.0f,  0.0f, 1.0f,        0.0f, 0.0f,      // 0.80f, 0.55f, 0.06f, 1.0f,    // Bottom left          / Vertex 2 of plane        / color = brown     / bottom left texture
     2.0f, 0.0f, -2.0f,         0.0f,  0.0f, 1.0f,        1.0f, 0.0f       // 0.80f, 0.55f, 0.06f, 1.0f,    // Bottom right         / Vertex 3 of plane        / color = brown     / bottom right texture
};

// CLN: Plane indices
GLushort PlaneIndices[] = {
    // CLN: draw plane for 3D scene
    0, 1, 2,  // Verticies for upper left triangle
    1, 2, 3   // Verticies for lower right triganle
};


// CLN: TriCase vertices
GLfloat TriCaseVertices[] = {
    // CLN: [Texture] Added texture coordinates for s and t (x and y axes)
    // Vertex Positions         // Normals (nx, ny, nz)   // Texture (s, t) // Colors (r,g,b,a) (removed)

    // CLN: Front triangle of Tri-Case
     0.0f, 0.5f, 0.0f,          0.0f,  0.0f, 1.0f,        0.5f, 1.0f,       // 0.0f, 1.0f, 0.0f, 1.0f,       // Apex                 / Vertex 0 of the Tri-case / color = green    / top middle of texture
    -0.3f, 0.0f, 0.0f,          0.0f,  0.0f, 1.0f,        0.0f, 0.0f,       // 1.0f, 1.0f, 0.0f, 1.0f,       // Bottom left front    / Vertex 1 of the Tri-case / color = yellow   / bottom left of texture
     0.3f, 0.0f, 0.0f,          0.0f,  0.0f, 1.0f,        1.0f, 0.0f,       // 0.0f, 0.0f, 1.0f, 1.0f,       // Bottom right front   / Vertex 2 of the Tri-case / color = blue     / bottom right of texture

     // CLN: Rear triangle of Tri-case
     0.0f, 0.5f, -1.0f,         0.0f, -1.0f,  0.0f,        0.0f, 0.0f,       // 0.0f, 1.0f, 1.0f, 1.0f,       // Apex                 / Vertex 3 of the Tri-case / color = cyan     / bottom left of texture
    -0.3f, 0.0f, -1.0f,         0.0f, -1.0f,  0.0f,        1.0f, 0.0f,       // 1.0f, 0.0f, 1.0f, 1.0f,       // Bottom left rear     / Vertex 4 of the Tri-case / color = magenta  / bottom right of texture
     0.3f, 0.0f, -1.0f,         0.0f, -1.0f,  0.0f,        1.0f, 1.0f        // 1.0f, 0.0f, 0.0f, 1.0f,       // Bottom right rear    / Vertex 5 of the Tri-case / color = red      / for base texture
};

// CLN: TriCase indices
GLushort TriCaseIndices[] = {
    // CLN: draw base of Tri-case first (plane primitive)
    1, 2, 4,  // Vertices for Triangle 1 - Left base triangle of Tri-case
    2, 4, 5,  // Vertices for Triangle 2 - Right base triangle of Tri-case

    // CLN: draw front and rear triangles of the Tri-case (triangle primitive)
    0, 1, 2,  // Vertices for Triangle 3 - Front triangle of Tri-case
    3, 4, 5,  // Vertices for Triangle 4 - Rear triangle of Tri-case

    // CLN: draw left face of the Tri-case (plane primitive)
    0, 1, 4,  // Vertices for Triangle 5 - Front left face of Tri-case
    0, 3, 4,  // Vertices for Triangle 6 - Rear left face of Tri-case

    // CLN: draw right face of the Tri-case (plane primitive)
    0, 2, 3,  // Vertices for Triangle 7 - Front right face of Tri-case
    2, 3, 5   // Vertices for Triangle 8 - Rear right face of Tri-case
};

// CLN: TriCaseLogo vertices
GLfloat TriCaseLogoVertices[] = {
    // CLN: [Texture] Added texture coordinates for s and t (x and y axes)
    // Vertex Positions          // Normals (nx, ny, nz)   // Texture (s, t)  // Colors (r,g,b,a) (removed)

    // CLN: Right face of Tri-Case
     0.001f, 0.5f, -0.5f,     0.0f,  0.0f, 1.0f,        0.0f, 1.0f,    // 0.0f, 1.0f, 0.0f, 1.0f,         // upper middle       / Vertex 0 of the Tri-case / color = green    / top left of texture
     0.001f, 0.5f, -1.0f,     0.0f,  0.0f, 1.0f,        1.0f, 1.0f,    // 0.0f, 1.0f, 1.0f, 1.0f,         // upper rear         / Vertex 1 of the Tri-case / color = cyan     / top right of texture
     0.301f, 0.0f, -0.5f,     0.0f,  0.0f, 1.0f,        0.0f, 0.0f,    // 0.0f, 0.0f, 1.0f, 1.0f,         // lower middle       / Vertex 2 of the Tri-case / color = blue     / bottom left of texture 
     0.301f, 0.0f, -1.0f,     0.0f,  0.0f, 1.0f,        1.0f, 0.0f     // 1.0f, 0.0f, 0.0f, 1.0f,         // lower rear         / Vertex 3 of the Tri-case / color = red      / bottom right of texture
};

// CLN: TriCaseLogo indices
GLushort TriCaseLogoIndices[] = {
    // CLN: draw logo on right face of Tri-case
    0, 1, 2,  // Vertices for Triangle 7 - Front right face of Tri-case
    1, 2, 3   // Vertices for Triangle 8 - Rear right face of Tri-case
};

// CLN: Cube vertices
GLfloat CubeVertices[] = {
    // Vertex Positions          // Normals (nx, ny, nz)   // Texture (s, t) 
    -0.5f, -0.5f, -0.5f,         0.0f,  0.0f, -1.0f,       0.0f,  0.0f,     // Vertex 0 of the cube
     0.5f, -0.5f, -0.5f,         0.0f,  0.0f, -1.0f,       1.0f,  0.0f,     // Vertex 1 of the cube
     0.5f,  0.5f, -0.5f,         0.0f,  0.0f, -1.0f,       1.0f,  1.0f,     // Vertex 2 of the cube
     0.5f,  0.5f, -0.5f,         0.0f,  0.0f, -1.0f,       1.0f,  1.0f,     // Vertex 3 of the cube
    -0.5f,  0.5f, -0.5f,         0.0f,  0.0f, -1.0f,       0.0f,  1.0f,     // Vertex 4 of the cube
    -0.5f, -0.5f, -0.5f,         0.0f,  0.0f, -1.0f,       0.0f,  0.0f,     // Vertex 5 of the cube

    -0.5f, -0.5f,  0.5f,         0.0f,  0.0f,  1.0f,       0.0f,  0.0f,     // Vertex 6 of the cube
     0.5f, -0.5f,  0.5f,         0.0f,  0.0f,  1.0f,       1.0f,  0.0f,     // Vertex 7 of the cube
     0.5f,  0.5f,  0.5f,         0.0f,  0.0f,  1.0f,       1.0f,  1.0f,     // Vertex 8 of the cube
     0.5f,  0.5f,  0.5f,         0.0f,  0.0f,  1.0f,       1.0f,  1.0f,     // Vertex 9 of the cube
    -0.5f,  0.5f,  0.5f,         0.0f,  0.0f,  1.0f,       0.0f,  1.0f,     // Vertex 10 of the cube
    -0.5f, -0.5f,  0.5f,         0.0f,  0.0f,  1.0f,       0.0f,  0.0f,     // Vertex 11 of the cube

    -0.5f,  0.5f,  0.5f,        -1.0f,  0.0f,  0.0f,       1.0f,  0.0f,     // Vertex 12 of the cube
    -0.5f,  0.5f, -0.5f,        -1.0f,  0.0f,  0.0f,       1.0f,  1.0f,     // Vertex 13 of the cube
    -0.5f, -0.5f, -0.5f,        -1.0f,  0.0f,  0.0f,       0.0f,  1.0f,     // Vertex 14 of the cube
    -0.5f, -0.5f, -0.5f,        -1.0f,  0.0f,  0.0f,       0.0f,  1.0f,     // Vertex 15 of the cube
    -0.5f, -0.5f,  0.5f,        -1.0f,  0.0f,  0.0f,       0.0f,  0.0f,     // Vertex 16 of the cube
    -0.5f,  0.5f,  0.5f,        -1.0f,  0.0f,  0.0f,       1.0f,  0.0f,     // Vertex 17 of the cube

     0.5f,  0.5f,  0.5f,         1.0f,  0.0f,  0.0f,       1.0f,  0.0f,     // Vertex 18 of the cube
     0.5f,  0.5f, -0.5f,         1.0f,  0.0f,  0.0f,       1.0f,  1.0f,     // Vertex 19 of the cube
     0.5f, -0.5f, -0.5f,         1.0f,  0.0f,  0.0f,       0.0f,  1.0f,     // Vertex 20 of the cube
     0.5f, -0.5f, -0.5f,         1.0f,  0.0f,  0.0f,       0.0f,  1.0f,     // Vertex 21 of the cube
     0.5f, -0.5f,  0.5f,         1.0f,  0.0f,  0.0f,       0.0f,  0.0f,     // Vertex 22 of the cube
     0.5f,  0.5f,  0.5f,         1.0f,  0.0f,  0.0f,       1.0f,  0.0f,     // Vertex 23 of the cube

    -0.5f, -0.5f, -0.5f,         0.0f, -1.0f,  0.0f,       0.0f,  1.0f,     // Vertex 24 of the cube
     0.5f, -0.5f, -0.5f,         0.0f, -1.0f,  0.0f,       1.0f,  1.0f,     // Vertex 25 of the cube
     0.5f, -0.5f,  0.5f,         0.0f, -1.0f,  0.0f,       1.0f,  0.0f,     // Vertex 26 of the cube
     0.5f, -0.5f,  0.5f,         0.0f, -1.0f,  0.0f,       1.0f,  0.0f,     // Vertex 27 of the cube
    -0.5f, -0.5f,  0.5f,         0.0f, -1.0f,  0.0f,       0.0f,  0.0f,     // Vertex 28 of the cube
    -0.5f, -0.5f, -0.5f,         0.0f, -1.0f,  0.0f,       0.0f,  1.0f,     // Vertex 29 of the cube

    -0.5f,  0.5f, -0.5f,         0.0f,  1.0f,  0.0f,       0.0f,  1.0f,     // Vertex 30 of the cube
     0.5f,  0.5f, -0.5f,         0.0f,  1.0f,  0.0f,       1.0f,  1.0f,     // Vertex 31 of the cube
     0.5f,  0.5f,  0.5f,         0.0f,  1.0f,  0.0f,       1.0f,  0.0f,     // Vertex 32 of the cube
     0.5f,  0.5f,  0.5f,         0.0f,  1.0f,  0.0f,       1.0f,  0.0f,     // Vertex 33 of the cube
    -0.5f,  0.5f,  0.5f,         0.0f,  1.0f,  0.0f,       0.0f,  0.0f,     // Vertex 34 of the cube
    -0.5f,  0.5f, -0.5f,         0.0f,  1.0f,  0.0f,       0.0f,  1.0f      // Vertex 35 of the cube
};

// CLN: Cube indices
GLushort CubeIndices[] = {
    // CLN: draw cube
    0, 1, 2,     // Right face of cube - triangle 1
    3, 4, 5,     // Right face of cube - triangle 2
    6, 7, 8,     // Left face of cube - triangle 1
    9, 10, 11,   // Left face of cube - triangle 2
    12, 13, 14,  // Front face of cube - triangle 1
    15, 16, 17,  // Front face of cube - triangle 2
    18, 19, 20,  // Back face of cube - triangle 1
    21, 22, 23,  // Back face of cube - triangle 2
    24, 25, 26,  // Bottom face of cube - triangle 1
    27, 28, 29,  // Bottom face of cube - triangle 2
    30, 31, 32,  // Top face of cube - triangle 1
    33, 34, 35   // Top face of cube - triangle 2
};


//------------------
// CLN: main (begin)
//------------------
int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;
    
    // CLN: Instantiates Cynlinder object and builds its vertices, texture coordinates, and indices
    //      creates a cylinder with base radius=0.27f, top radius=0.27f, height=0.9f, sectors=36, stacks=1, smooth=true
    //-----------------------------------------------------------------------------------------------------------------
    Cylinder cylinder(0.27, 0.27, 0.9, 36, 1, true);

    // CLN: Instantiates Sphere object and builds its vertices, texture coordinates, and indices
    //      creates a sphere with radius=0.4, sectors=36, stacks=18, smooth=true (default)
    //------------------------------------------------------------------------------------------
    Sphere sphere(0.4f, 36, 18);


    // CLN: For debugging
   /*
    const float* cylNormals = cylinder.getNormals();

    std::cout << "DEBUG: The cylinder normals are : ";
    for (int i = 0; i < cylinder.getNormalSize(); i++)
        std::cout << cylNormals[i] << ", ";

    const float* sphereNormals = sphere.getNormals();

    std::cout << "\n\nDEBUG: The sphere normals are : ";
    for (int i = 0; i < cylinder.getNormalSize(); i++)
        std::cout << sphereNormals[i] << ", ";
    */


    // CLN: Instantiate the various objects for the 3D Scene
    // -----------------------------------------------------
    GLObject Plane;
    GLObject TriCase;
    GLObject TriCaseLogo;
    GLObject LaCroixCan;
    GLObject FoamBall;
    GLObject StickyNotes;
    GLObject MainLight;
    GLObject FillLight;

    // CLN: Load in the textures for the objects
    // -----------------------------------------
    if (!Plane.CreateTexture(texFilename1, Plane.gTextureId))
    {
        cout << "Failed to load texture " << texFilename1 << endl;
        return EXIT_FAILURE;
    }
    else if (!TriCase.CreateTexture(texFilename2, TriCase.gTextureId))
    {
        cout << "Failed to load texture " << texFilename2 << endl;
        return EXIT_FAILURE;
    }
    else if (!TriCaseLogo.CreateTexture(texFilename3, TriCaseLogo.gTextureId))
    {
        cout << "Failed to load texture " << texFilename3 << endl;
        return EXIT_FAILURE;
    }
    else if (!LaCroixCan.CreateTexture(texFilename4, LaCroixCan.gTextureId))
    {
        cout << "Failed to load texture " << texFilename4 << endl;
        return EXIT_FAILURE;
    }
    else if (!FoamBall.CreateTexture(texFilename5, FoamBall.gTextureId))
    {
        cout << "Failed to load texture " << texFilename5 << endl;
        return EXIT_FAILURE;
    }
    else if (!StickyNotes.CreateTexture(texFilename6, StickyNotes.gTextureId))
    {
        cout << "Failed to load texture " << texFilename6 << endl;
        return EXIT_FAILURE;
    }
    else
    {
        cout << "All textures loaded successfully!" << endl;
    }

    // CLN: Create meshes for the various 3D objects by transferring vertices and indices into each
    //      respective object's VBO, then bind over to the GPU
    // ---------------------------------------------------------------------------------------------
    Plane.CreateMesh(*PlaneVertices, sizeof(PlaneVertices), *PlaneIndices, sizeof(PlaneIndices));
    TriCase.CreateMesh(*TriCaseVertices, sizeof(TriCaseVertices), *TriCaseIndices, sizeof(TriCaseIndices));
    TriCaseLogo.CreateMesh(*TriCaseLogoVertices, sizeof(TriCaseLogoVertices), *TriCaseLogoIndices, sizeof(TriCaseLogoIndices));
    LaCroixCan.CreateMesh(*cylinder.getVertices(), cylinder.getVertexSize(), *cylinder.getIndices(), cylinder.getIndexSize());
    FoamBall.CreateMesh(*sphere.getVertices(), sphere.getVertexSize(), *sphere.getIndices(), sphere.getIndexSize());
    StickyNotes.CreateMesh(*CubeVertices, sizeof(CubeVertices), *CubeIndices, sizeof(CubeIndices));
    MainLight.CreateMesh(*CubeVertices, sizeof(CubeVertices), *CubeIndices, sizeof(CubeIndices));
    FillLight.CreateMesh(*CubeVertices, sizeof(CubeVertices), *CubeIndices, sizeof(CubeIndices));
     
    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    // CLN: [Lighting] Added to create shader for lamp object
    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // CLN: [Texture] tell opengl for each sampler to which texture unit it belongs (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTextureBase"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // CLN: This is the render loop that keeps on running at the monitor's refresh
    //      rate, until it is canceled by the user (i.e. ESC key)
    // ---------------------------------------------------------------------------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // UProcessInput function processes all user input into the window object using glfwGetKey() 
        // function.
        // CLN: If the 'ESC' key is pressed, will close the OpenGL window, otherwise ASDWQEP keys
        //      are processed accordingly
        // -----------------------------------------------------------------------------------------
        UProcessInput(gWindow);

        // CLN: This renders the window's background color. Set glClearColor RGB values to 0 for a black background
        // and clears the frame and z buffers
        // --------------------------------------------------------------------------------------------------------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // CLN: Renders the 3D Scene by passing the scale, rotate, and translate matrices, and lamp & orbit bools to the object's Render method
        // ------------------------------------------------------------------------------------------------------------------------------------
        Plane.Render(glm::scale(glm::vec3(2.5f, 2.5f, 2.5f)), glm::rotate(glm::radians(30.0f), glm::vec3(0.3f, 1.0f, 0.0f)), glm::translate(glm::vec3(0.0f, 0.0f, 0.0f)), false, false);
        TriCase.Render(glm::scale(glm::vec3(2.0f, 2.0f, 2.0f)), glm::rotate(glm::radians(30.0f), glm::vec3(0.3f, 1.0f, 0.0f)), glm::translate(glm::vec3(-1.0f, -0.54f, 4.0f)), false, false);
        TriCaseLogo.Render(glm::scale(glm::vec3(2.0f, 2.0f, 2.0f)), glm::rotate(glm::radians(30.0f), glm::vec3(0.3f, 1.0f, 0.0f)), glm::translate(glm::vec3(-1.0f, -0.54f, 4.0f)), false, false);
        LaCroixCan.Render(glm::scale(glm::vec3(2.0f, 2.0f, 2.0f)), glm::rotate(glm::radians(99.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::translate(glm::vec3(1.0f, 0.75f, 1.0f)), false, false);
        FoamBall.Render(glm::scale(glm::vec3(1.0f, 1.0f, 1.0f)), glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::translate(glm::vec3(1.0f, -0.24f, 4.2f)), false, false);
        StickyNotes.Render(glm::scale(glm::vec3(1.0f, 0.1f, 1.0f)), glm::rotate(glm::radians(30.0f), glm::vec3(0.3f, 1.0f, 0.0f)), glm::translate(glm::vec3(2.5f, -0.31f, 2.0f)), false, false);
        // sets light color (set to white, 100% intensity)
        gLightColor.r = 1.0f;
        gLightColor.g = 1.0f;
        gLightColor.b = 1.0f; 
        MainLight.Render(glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)), glm::rotate(glm::radians(30.0f), glm::vec3(0.3f, 1.0f, 0.0f)), glm::translate(glm::vec3(2.5f, 2.0f, 7.0)), true, true);
        //gLightColor.r, gLightColor.g, gLightColor.b = 0.1f; // sets color white 10% intensity for fill light (FillLight)
        FillLight.Render(glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)), glm::rotate(glm::radians(30.0f), glm::vec3(0.3f, 1.0f, 0.0f)), glm::translate(glm::vec3(5.0f, 1.0f, -1.0)), true, false);
        
        // CLN: Moved the swap buffers here, instead of in the object's Rendedr() method, to prevent flickering
        glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
        glfwPollEvents();
    }

    // CLN: Teardown
    // -----------------------------------------------------
    // CLN: Release the mesh data for each respective object
    Plane.DestroyMesh(Plane.mesh);
    TriCase.DestroyMesh(TriCase.mesh);
    TriCaseLogo.DestroyMesh(TriCaseLogo.mesh);
    LaCroixCan.DestroyMesh(LaCroixCan.mesh);
    FoamBall.DestroyMesh(FoamBall.mesh);
    StickyNotes.DestroyMesh(StickyNotes.mesh);
    MainLight.DestroyMesh(MainLight.mesh);
    FillLight.DestroyMesh(FillLight.mesh);

    // CLN: [Texture] Release texture
    Plane.DestroyTexture(Plane.gTextureId);
    TriCase.DestroyTexture(TriCase.gTextureId);
    TriCaseLogo.DestroyTexture(TriCaseLogo.gTextureId);
    LaCroixCan.DestroyTexture(LaCroixCan.gTextureId);
    FoamBall.DestroyTexture(FoamBall.gTextureId);
    StickyNotes.DestroyTexture(StickyNotes.gTextureId);
    MainLight.DestroyTexture(MainLight.gTextureId);
    FillLight.DestroyTexture(FillLight.gTextureId);

    // Release shader program (teardown)
    UDestroyShaderProgram(gProgramId);
    
    // CLN: [Lighting] release lamp shader program
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}
//----------------
// CLN: main (end)
//----------------



// -----------------------------------------
// CLN: Implementations of prototypes above
// -----------------------------------------

// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


    // glfw window creation:
    // This creates a pointer to a GLFWwindow object, which holds all the windowing data for all GLFW functions
    // --------------------------------------------------------------------------------------------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }


    // glfw: whenever the window size changed (by OS or user resize) this callback function executes
    // Callback functions set for cursor position, mouse scroll, and mouse button
    // ---------------------------------------------------------------------------------------------
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
        cout << "ESC key pressed!" << endl;
    }

    // CLN: when 'W' key pressed, move camera forward toward object
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
        cout << "'W' key pressed!" << endl;
    }

    // CLN: when 'S' key pressed, move camera backward away from object
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
        cout << "'S' key pressed!" << endl;
    }

    // CLN: when 'A' key pressed, move camera right so object appears it's moving left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
        cout << "'A' key pressed!" << endl;
    }

    // CLN: when 'D' key pressed, move camera left so object appears it's moving right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
        cout << "'D' key pressed!" << endl;
    }

    // CLN: Added functionality for 'Q' and 'E' keys to move the 'UP' vector
    //      about the z-axis, up and down respectively
    //----------------------------------------------------------------------
    // CLN: when 'Q' key pressed, move camera upward about the z-axis (Up vector)
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(UP, gDeltaTime);
        cout << "'Q' key pressed!" << endl;
    }

    // CLN: when 'E' key pressed, move camera downward about the z-axis
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
        cout << "'E' key pressed!" << endl;
    }

    // CLN: when 'P' key pressed, toggle between perspective and orthographic views
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        // CLN: if odd PCount, then set to orthographic view, else perspective view
        if (PCount % 2 != 0) {
            projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 2.0f, 100.0f);
            cout << "Orthographic View is On!" << endl;
        }
        else {
            projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f); // CLN: Creates a perspective projection matrix

            cout << "Perspective View is On!" << endl;
        }
        ++PCount; // CLN: Increment 'P' counter
        //cout << "'P' key pressed!" << endl;
    }
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

//--------------------------------------------------------
// CLN: Added functions for mouse input handling
//--------------------------------------------------------

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
    cout << "Mouse scroll wheel moved!" << endl;
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// --------------------------------------
// build and compile shader program
// --------------------------------------
// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
