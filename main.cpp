#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <GL/glew.h>   // The GL Header File
#include <GL/gl.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

double lastFrameratePrintTime = glfwGetTime();
double lastTime = glfwGetTime();
double deltaTime = 0;
int nbFrames = 0;

GLuint gProgram;
GLuint gTextProgram;
glm::mat4 perspMat;
int gWidth = 1080, gHeight = 720;
int modelingMatLoc, modelingMatInvTrLoc, perspectiveMatLoc;

struct Vertex
{
    Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Texture
{
    Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
    GLfloat u, v;
};

struct Normal
{
    Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Face
{
	Face(int v[], int t[], int n[]) {
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
    GLuint vIndex[3], tIndex[3], nIndex[3];
};

bool ParseObj(const string& fileName, vector<Vertex> &gVertices, vector<Texture> &gTextures, vector<Normal> &gNormals, vector<Face> &gFaces);
void initModels();
struct Model
{
    glm::vec3 position;
    glm::vec3 scale;

    glm::mat4 positionM;
    glm::mat4 rotationM;
    glm::mat4 scaleM;

    glm::vec3 color;
    vector<Vertex> vertices;
    vector<Texture> textures;
    vector<Normal> normals;
    vector<Face> faces;

    GLuint VAB;
    GLuint VIB;

    glm::vec3 lightPosition;

    Model(){}
    Model(const string& fileName, glm::vec3 inPosition, glm::vec3 inScale, glm::vec3 inColor, glm::vec3 lightPos) 
    : position(inPosition), scale(inScale), color(inColor), lightPosition(lightPos)
    {
        rotationM = glm::mat4(1.0f);
        positionM = glm::translate(glm::mat4(1.0f), position);
        scaleM = glm::scale(glm::mat4(1.0f), scale);
        ParseObj(fileName, vertices, textures, normals, faces);
    }

    void RotationAdd(float angle, glm::vec3 axis)
    {
        rotationM = glm::rotate(rotationM, glm::radians((float) angle), axis);
    }
    void RotationSet(glm::mat4 rot)
    {
        rotationM = rot;
    }
    void TranslateSet(glm::vec3 pos)
    {
        position = pos;
        positionM = glm::translate(glm::mat4(1.0f), pos);
    }
    void TranslateAdd(glm::vec3 add)
    {
        position += add;
        positionM = glm::translate(positionM, add);
    }

    void Scale(float sc)
    {
        scale = glm::vec3(sc);
        scaleM = glm::scale(glm::mat4(1.0f), scale);
    }

    void Scale(glm::vec3 sc)
    {
        scale = sc;
        scaleM = glm::scale(glm::mat4(1.0f), scale);
    }
};

//OBJECTS
Model checkpoints[3];
Model bunny;
Model ground;
vector<Model*> models;
int groundIndex;
GLuint gTextVBO;

//ANIMATION VARIABLES
float groundOffset = 0;
float bunnyDirection = 0;
glm::vec3 initialCheckpointPos[3];
int goalIndex = 0;
glm::vec3 goalColor = glm::vec3(1.0f, 1.0f, 0.0f);
glm::vec3 obstacleColor = glm::vec3(1.0f, 0.0f, 0.0f);
int score = 0;

int gameState = 0;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

bool ParseObj(const string& fileName, vector<Vertex> &gVertices, vector<Texture> &gTextures, vector<Normal> &gNormals, vector<Face> &gFaces)
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            stringstream str(curLine);
            GLfloat c1, c2, c3;
            GLuint index[9];
            string tmp;

            if (curLine.length() >= 2)
            {
                if (curLine[0] == '#') // comment
                {
                    continue;
                }
                else if (curLine[0] == 'v')
                {
                    if (curLine[1] == 't') // texture
                    {
                        str >> tmp; // consume "vt"
                        str >> c1 >> c2;
                        gTextures.push_back(Texture(c1, c2));
                    }
                    else if (curLine[1] == 'n') // normal
                    {
                        str >> tmp; // consume "vn"
                        str >> c1 >> c2 >> c3;
                        gNormals.push_back(Normal(c1, c2, c3));
                    }
                    else // vertex
                    {
                        str >> tmp; // consume "v"
                        str >> c1 >> c2 >> c3;
                        gVertices.push_back(Vertex(c1, c2, c3));
                    }
                }
                else if (curLine[0] == 'f') // face
                {
                    str >> tmp; // consume "f"
					char c;
					int vIndex[3],  nIndex[3], tIndex[3];
					str >> vIndex[0]; str >> c >> c; // consume "//"
					str >> nIndex[0]; 
					str >> vIndex[1]; str >> c >> c; // consume "//"
					str >> nIndex[1]; 
					str >> vIndex[2]; str >> c >> c; // consume "//"
					str >> nIndex[2]; 

					assert(vIndex[0] == nIndex[0] &&
						   vIndex[1] == nIndex[1] &&
						   vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

                    gFaces.push_back(Face(vIndex, tIndex, nIndex));
                }
                else
                {
                    cout << "Ignoring unidentified line in obj file: " << curLine << endl;
                }
            }

            //data += curLine;
            if (!myfile.eof())
            {
                //data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

	assert(gVertices.size() == gNormals.size());

    return true;
}
bool ReadDataFromFile(
    const string& fileName, ///< [in]  Name of the shader file
    string&       data)     ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

void createVS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("VS compile log: %s\n", output);

    glAttachShader(program, vs);
}

void createFS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);
    printf("FS compile log: %s\n", output);

    glAttachShader(program, fs);
}

void initShaders()
{
    gProgram = glCreateProgram();
    gTextProgram = glCreateProgram();

    createVS(gProgram, "vert.glsl");
    createFS(gProgram, "frag.glsl");

    createVS(gTextProgram, "vert_text.glsl");
    createFS(gTextProgram, "frag_text.glsl");

    glBindAttribLocation(gProgram, 0, "inVertex");
    glBindAttribLocation(gProgram, 1, "inNormal");
    glBindAttribLocation(gTextProgram, 2, "vertex");

    glLinkProgram(gProgram);
    glLinkProgram(gTextProgram);

    glUseProgram(gProgram);
}

void initVBO(Model &model)
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    assert(glGetError() == GL_NONE);

    glGenBuffers(1, &model.VAB);
    glGenBuffers(1, &model.VIB);

    assert(model.VAB > 0 && model.VIB > 0);

    glBindBuffer(GL_ARRAY_BUFFER, model.VAB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.VIB);


    int gVertexDataSizeInBytes = model.vertices.size() * 3 * sizeof(GLfloat);
    int gNormalDataSizeInBytes = model.normals.size() * 3 * sizeof(GLfloat);
    int indexDataSizeInBytes = model.faces.size() * 3 * sizeof(GLuint);
    GLfloat* vertexData = new GLfloat [model.vertices.size() * 3];
    GLfloat* normalData = new GLfloat [model.normals.size() * 3];
    GLuint* indexData = new GLuint [model.faces.size() * 3];



    for (int i = 0; i < model.vertices.size(); ++i)
    {
        vertexData[3*i] = model.vertices[i].x;
        vertexData[3*i+1] = model.vertices[i].y;
        vertexData[3*i+2] = model.vertices[i].z;
    }

    for (int i = 0; i < model.normals.size(); ++i)
    {
        normalData[3*i] = model.normals[i].x;
        normalData[3*i+1] = model.normals[i].y;
        normalData[3*i+2] = model.normals[i].z;
    }

    for (int i = 0; i < model.faces.size(); ++i)
    {
        indexData[3*i] = model.faces[i].vIndex[0];
        indexData[3*i+1] = model.faces[i].vIndex[1];
        indexData[3*i+2] = model.faces[i].vIndex[2];
    }


    glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexData);
    glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, normalData);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

    // done copying; can free now
    delete[] vertexData;
    delete[] normalData;
    delete[] indexData;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));

}

void initFonts(int windowWidth, int windowHeight)
{
    // Set OpenGL options
    //glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gTextProgram);
    glUniformMatrix4fv(glGetUniformLocation(gTextProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSerif-Italic.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
                );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    // Configure VBO for texture quads
    //
    glGenBuffers(1, &gTextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void animate()
{
    if(gameState == -2)
    {
        goto reset;
    }
    if(gameState == -1)
    {
        glm::mat4 mat = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0,1,0));
        mat = glm::rotate(mat, glm::radians(-90.0f), glm::vec3(1,0,0));

        bunny.RotationSet(mat);
        bunny.TranslateSet(glm::vec3(bunny.position.x, 0, bunny.position.z));
        return;
    }
    score++;

    static float speedAdditionIncreaseSpeed = .1f;
    static float speedAddition = 1.0f;
    speedAddition += speedAdditionIncreaseSpeed * deltaTime;

    static float groundSpeed = 5.0f;
    groundSpeed += speedAddition * deltaTime;
    groundOffset -= groundSpeed * deltaTime;

    static int bunnyBounceDirection = 1;
    static float bunnyBounceSpeed = 7.0f;
    static float bunnyBounceMultiplier = 0.1f;
    static float bunnySideSpeed = 10.0f;
    static float bunnySpin = 0.0f;
    static float bunnySpinSpeed = 720.0f;
    bunnyBounceSpeed += speedAddition * deltaTime * bunnyBounceMultiplier;
    bunnySideSpeed += speedAddition * deltaTime * bunnyBounceMultiplier;
    bunnySpinSpeed += speedAddition * deltaTime * bunnyBounceMultiplier;
    if(bunny.position.x + bunnySideSpeed * bunnyDirection * deltaTime < -7.5f)
        bunnyDirection = 0;
    if(bunny.position.x + bunnySideSpeed * bunnyDirection * deltaTime > 7.5f)
        bunnyDirection = 0;
    bunny.TranslateAdd(glm::vec3(bunnySideSpeed * bunnyDirection * deltaTime, bunnyBounceDirection * bunnyBounceSpeed * deltaTime, 0.0f));
    bunny.RotationSet(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f + bunnySpin), glm::vec3(0,1,0)));

    if(gameState == 1)
    {
        bunnySpin += bunnySpinSpeed * deltaTime;
        if(bunnySpin > 360.0f)
        {
            bunnySpin = 0;
            gameState = 0;
        }
    }

    if(bunnyBounceDirection == 1 && bunny.position.y > 2)
        bunnyBounceDirection = -1;
    if(bunnyBounceDirection == -1 && bunny.position.y < 0)
        bunnyBounceDirection = 1;

    static float checkpointMultiplier = 3.0f;
    static bool checkPointReached = false;
    
    for(int i = 0; i<3; i++)
    {
        checkpoints[i].TranslateAdd(glm::vec3(0.0f, 0.0f, groundSpeed * deltaTime*0.95f));
        if(checkpoints[i].position.z > -0.5f)
        {
            float dist = glm::distance(glm::vec3(bunny.position.x, 0, bunny.position.z), glm::vec3(checkpoints[i].position.x, 0, checkpoints[i].position.z));
            bool hit = false;
            if(dist < bunny.scale.x + checkpoints[i].scale.x)
                hit = true;
            if(hit && i == goalIndex && gameState !=1)
            {
                gameState = 1;
                score+=1000;
            }
            else if(hit && i != goalIndex)
            {
                gameState = -1;
            }
            checkPointReached = true;
            checkpoints[i].TranslateSet(initialCheckpointPos[i]);
        }
    }

    if(checkPointReached)
    {
        goalIndex = rand() % 3;
        glm::vec3 color = obstacleColor;
        for(int i = 0; i<3 ;i++)
        {
            if(i == goalIndex)
                color = goalColor;
            else
                color = obstacleColor;
            checkpoints[i].color = color;
        }
        checkPointReached = false;
    }
reset:
    if(gameState == -2)
    {
        bunny.TranslateSet(glm::vec3(0.0f));
        bunny.Scale(0.9f);
        bunny.RotationSet(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

        goalIndex = rand() % 3;
        glm::vec3 color = obstacleColor;
        for(int i = 0; i<3 ;i++)
        {
            if(i == goalIndex)
                color = goalColor;
            else
                color = obstacleColor;
            checkpoints[i].TranslateSet(glm::vec3(-6.0f + i * 6.0f, 0.75f, -50.0f));
            checkpoints[i].Scale(glm::vec3(1.0f, 1.5f, .5f));
            checkpoints[i].color = color;
            initialCheckpointPos[i] = checkpoints[i].position;
        }
        ground.TranslateSet(glm::vec3(0.0f, -1.0f, 0.0f));
        ground.RotationSet(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0)));
        ground.Scale(glm::vec3(15, 300.0f , 1.0f));
        
        groundOffset = 0;
        bunnyDirection = 0;

        speedAdditionIncreaseSpeed = .1f;
        speedAddition = 1.0f;

        groundSpeed = 5.0f;

        bunnyBounceDirection = 1;
        bunnyBounceSpeed = 7.0f;
        bunnyBounceMultiplier = 0.1f;
        bunnySideSpeed = 10.0f;
        bunnySpin = 0.0f;
        bunnySpinSpeed = 720.0f;
        checkpointMultiplier = 3.0f;
        checkPointReached = false;
        
        gameState = 0;
        score = 0;
    }
}

void drawModel(Model model, glm::mat4 T, glm::mat4 R, glm::mat4 S)
{
    glm::mat4 modelMat = T * R * S;
    glm::mat4 modelMatInv = glm::transpose(glm::inverse(modelMat));

    glUniformMatrix4fv(modelingMatLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(modelingMatInvTrLoc , 1, GL_FALSE, glm::value_ptr(modelMatInv));
    glUniformMatrix4fv(perspectiveMatLoc, 1, GL_FALSE, glm::value_ptr(perspMat));

	glBindBuffer(GL_ARRAY_BUFFER, model.VAB);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.VIB);
    
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(model.vertices.size() * 3 * sizeof(GLfloat)));

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

	glDrawElements(GL_TRIANGLES, model.faces.size() * 3, GL_UNSIGNED_INT, 0);
}

void renderText(const std::string& text, GLfloat x, GLfloat y, glm::vec2 scale, glm::vec3 color)
{
    // Activate corresponding render state	
    glUseProgram(gTextProgram);
    glUniform3f(glGetUniformLocation(gTextProgram, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale.x;
        GLfloat ypos = y - (ch.Bearing.y) * scale.y;

        GLfloat w = ch.Size.x * scale.x;
        GLfloat h = ch.Size.y * scale.y;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },            
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }           
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        //glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

        x += (ch.Advance >> 6) * scale.x; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}


void display()
{
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glUseProgram(gProgram);
    animate();

    for(int i=0 ; i < models.size() ; i++)
    {
        Model model = *models[i];
        glm::vec3 lightPos = glm::vec3(model.position.x, model.position.y + 3, model.position.z + 5);
        glUniform3f(glGetUniformLocation(gProgram, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(gProgram, "eyePos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(gProgram, "color"), model.color.x, model.color.y, model.color.z);
        if(i == groundIndex)
        {
            glUniform1i(glGetUniformLocation(gProgram, "isCheckboard"), true);
            glUniform1f(glGetUniformLocation(gProgram, "scale"), .1f);

            glUniform1f(glGetUniformLocation(gProgram, "offset"), groundOffset);
        }

        drawModel(model, model.positionM, model.rotationM, model.scaleM);
        glUniform1i(glGetUniformLocation(gProgram, "isCheckboard"), false);
    }

    assert(glGetError() == GL_NO_ERROR);

    char str[1000];
    sprintf(str, "Score: %d", score);
    if(gameState == -1)
    {
        renderText(str, 0, 720, glm::vec2(1080.0f/gWidth, 720.0f/gHeight), glm::vec3(1, 0, 0));
    }
    else
    {
        renderText(str, 0, 720, glm::vec2(1080.0f/gWidth, 720.0f/gHeight), glm::vec3(1, 1, 0));
    }

    assert(glGetError() == GL_NO_ERROR);
}

void reshape(GLFWwindow* window, int w, int h);

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);

void mouse(GLFWwindow* window, int button, int action, int mods);


void SetCamera()
{
    glm::vec3 cameraPos = glm::vec3(0.0f, 4.0f, +5.0f); 
    glm::vec3 cameraTarget = glm::vec3(0.0f, 5.0f, -5.0f);
    

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraDirection = glm::normalize(cameraTarget - cameraPos); 
    glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
    glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);


	glm::mat4 projectionMatrix = glm::perspective(90.0f, (float)gWidth/ (float)gHeight, 0.1f, 200.0f);

	glm::mat4 viewingMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    perspMat = projectionMatrix * viewingMatrix;
}

void mainLoop(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        // Measure speed
        double currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        nbFrames++;
        if ( currentTime - lastFrameratePrintTime >= 1.0 ){
            printf("%f ms/frame\n", 1000.0/double(nbFrames));
            nbFrames = 0;
            lastFrameratePrintTime += 1.0;
        }
        
        SetCamera();
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void init() 
{
    glEnable(GL_DEPTH_TEST);
    initShaders();
    initFonts(gWidth, gHeight);

    modelingMatLoc = glGetUniformLocation(gProgram, "modelingMat");
    modelingMatInvTrLoc = glGetUniformLocation(gProgram, "modelingMatInvTr");
    perspectiveMatLoc = glGetUniformLocation(gProgram, "perspectiveMat");
    std::cout << "INIT DONE" << std::endl;
}

void initModels()
{
    srand(time(0));
    glm::vec3 lightPos =  glm::vec3(0.0f, 2.0f, 5.0f);
    bunny = Model(string("bunny.obj"), glm::vec3(0.0f), glm::vec3(0.9f), glm::vec3(255.0f/255.0f, 202.0f/255.0f, 58.0f/255.0f), lightPos);
    bunny.RotationSet(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    models.push_back(&bunny);

    goalIndex = rand() % 3;
    glm::vec3 color = obstacleColor;
    for(int i = 0; i<3 ;i++)
    {
        if(i == goalIndex)
            color = goalColor;
        else
            color = obstacleColor;
        checkpoints[i] = Model(string("cube.obj"), glm::vec3(-6.0f + i * 6.0f, 0.75f, -50.0f), glm::vec3(1.0f, 1.5f, .5f), color * 2.0f, lightPos);
        models.push_back(&checkpoints[i]);
        initialCheckpointPos[i] = checkpoints[i].position;
    }
    

    ground = Model(string("quad.obj"), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f), glm::vec3(255.0f/255.0f, 202.0f/255.0f, 58.0f/255.0f), lightPos);
    ground.RotationSet(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0)));
    ground.Scale(glm::vec3(15, 300.0f , 1.0f));
    models.push_back(&ground);
    groundIndex = models.size()-1;

    for(int i=0; i< models.size();i++)
    {
        initVBO(*models[i]);
    }

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
    GLFWwindow* window;
    if (!glfwInit())
    {
        exit(-1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    window = glfwCreateWindow(gWidth, gHeight, "Simple Example", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }
    glfwSetWindowTitle(window, "THE3");

    init();
    initModels();
    SetCamera();

    glfwSetKeyCallback(window, keyboard);
    glfwSetMouseButtonCallback(window, mouse);
    glfwSetWindowSizeCallback(window, reshape);

    reshape(window, gWidth, gHeight); // need to call this once ourselves
    mainLoop(window); // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
void mouse(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
    }
}
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    static int Astate = 0;
    static int Dstate = 0;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if ((key == GLFW_KEY_A || key == GLFW_KEY_LEFT)&& action == GLFW_PRESS)
    {
        Astate = 1;
    }
    if ((key == GLFW_KEY_A || key == GLFW_KEY_LEFT) && action == GLFW_RELEASE)
    {
        Astate = 0;
    }
    if ((key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) && action == GLFW_PRESS)
    {
        Dstate = 1;
    }
    if ((key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) && action == GLFW_RELEASE)
    {
        Dstate = 0;
    }
    if(key == GLFW_KEY_R)
    {
        gameState = -2;
    }

    bunnyDirection = 0;
    if(Astate == 1)
    {
        bunnyDirection = -1;
    }
    if(Dstate == 1)
    {
        bunnyDirection = 1;
    }
    if(Astate == 1 && Dstate == 1)
    {
        bunnyDirection = 0;
    }
}

void reshape(GLFWwindow* window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    gWidth = w;
    gHeight = h;

    glViewport(0, 0, w, h);
}
