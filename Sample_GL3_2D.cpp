#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <ao/ao.h>
#include <mpg123.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define BITS 8

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
float eye_x,eye_y,eye_z;
float camera_radius;
int camera_disable_rotation=0;
int camera_follow=0;
int camera_follow_adjust=0;
int camera_top=0;
int camera_fps=0;
float angle=135;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;
    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct COLOR {
    float r;
    float g;
    float b;
};
typedef struct COLOR COLOR;

struct Game {
    string name;
    COLOR color;
    float x,y;
    VAO* object;
    float height,width,radius;
    float x_speed,y_speed;
    float angle;
    int status;
};
typedef struct Game Game;

struct GLMatrices {
    glm::mat4 projection;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
    GLuint TexMatrixID;
} Matrices;
map <string, Game> laserObjects;
map <string, Game> laserbeam;
map <string, Game> backgroundObjects;
map <string, Game> object;
map <string, Game> drum;
map <string, Game> backgroundObjectsmirror;
map <string, Game> lasercircle;
map <string, Game> Brickwall;
map <string, Game> Scorewall;
map <string, Game> Score;
map <string, Game> drumcap;
map <string, Game> mirrorshade;
map <string, Game> prestart;
map <string, Game> heart;
map <string, Game> stage;
map <string, Game> circlebackground;
map <string, Game> circlebackground1;
map <string, Game> circlebackground2;
map <string, Game> circlebackground3;
void createRectangle (string name, float angle, COLOR color,COLOR color2, float x, float y, float height, float width, string component,int status);
GLFWwindow* initGLFW (int width, int height);
void initGL (GLFWwindow* window, int width, int height);
float zoom_camera = 1;
float game_start_timer=0;
int game_timer=90;
int x=0;
float x_change = 0; //For the camera pan
float y_change = 0; //For the camera pan
int life=5;
int points=0;
int start=0;
int stagelevel=1;
int goldenflag=1;

mpg123_handle *mh;
unsigned char *buffer;
size_t buffer_size;
size_t done;
int err;

int driver;
ao_device *dev;

ao_sample_format format;
int channels, encoding;
long rate;

GLuint programID;

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open())
    {
        std::string Line = "";
        while(getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
        std::string Line = "";
        while(getline(FragmentShaderStream, Line))
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

    // Compile Fragment Shader
    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

    // Link the program
    fprintf(stdout, "Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, GLfloat* vertex_buffer_data, GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
            0,                  // attribute 0. Vertices
            3,                  // size (x,y,z)
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
            );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
            1,                  // attribute 1. Color
            3,                  // size (r,g,b)
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
            );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, GLfloat* vertex_buffer_data, GLfloat red, GLfloat green, GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

int keyboard_pressed=0;
COLOR lasercolor={1,0,0};
string d;
float speed=0.5;
int pause=0;
int selectreddrum=0;
int selectgreendrum=0;
void mousescroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset==-1) { 
        zoom_camera /= 1.1; //make it bigger than current size
    }
    else if(yoffset==1){
        zoom_camera *= 1.1; //make it bigger than current size
    }
    if (zoom_camera<=1) {
        zoom_camera = 1;
    }
    if (zoom_camera>=4) {
        zoom_camera=4;
    }
    if(x_change-515.0f/zoom_camera<-515)
        x_change=-515+515.0f/zoom_camera;
    else if(x_change+515.0f/zoom_camera>515)
        x_change=515-515.0f/zoom_camera;
    if(y_change-300.0f/zoom_camera<-300)
        y_change=-300+300.0f/zoom_camera;
    else if(y_change+300.0f/zoom_camera>300)
        y_change=300-300.0f/zoom_camera;
    Matrices.projection = glm::ortho((float)(-515.0f/zoom_camera+x_change), (float)(515.0f/zoom_camera+x_change), (float)(-300.0f/zoom_camera+y_change), (float)(300.0f/zoom_camera+y_change), 0.1f, 500.0f);
}

//Ensure the panning does not go out of the map
void check_pan(){
    if(x_change-515.0f/zoom_camera<-515)
        x_change=-515+515.0f/zoom_camera;
    else if(x_change+515.0f/zoom_camera>515)
        x_change=515-515.0f/zoom_camera;
    if(y_change-300.0f/zoom_camera<-300)
        y_change=-300+300.0f/zoom_camera;
    else if(y_change+300.0f/zoom_camera>300)
        y_change=300-300.0f/zoom_camera;
}
int fl1=0;
int ckeypress=0;
int fkeypress=0;
int akeypress=0;
int skeypress=0;
int dkeypress=0;
int ctrlleftpress=0;
int ctrlrightpress=0;
int altleftpress=0;
int altrightpress=0;
int wkeypress=0;
int ekeypress=0;
int spacekeypress=0;
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch(key){
		        case GLFW_KEY_UP:
		        	if(pause==0)
		        	{
		        		mousescroll(window,0,+1);
		        		check_pan();
		        		break;
		        	}
		        case GLFW_KEY_DOWN:
		        	if(pause==0)
		        	{
		        		mousescroll(window,0,-1);
		        		check_pan();
		        		break;
		        	}
		        case GLFW_KEY_M:
		        	if(speed>=1 && pause==0)
		        	{
		            	speed-=0.5;
		        		break;
		        	}
		        case GLFW_KEY_N:
		        	if(pause==0)
		        	{
		            	speed+=0.5;
		        		break;
		        	}
		        case GLFW_KEY_F:
                    fkeypress=1;
			        break;
		        case 263:
			        if(pause==0)
			        {
				        if(mods==2)
				        {
                            ctrlleftpress=1;
				        }
				        else if(mods==4)
				        {
				            altleftpress=1;
				        }
				        else
				        {
				            x_change-=10;
				            check_pan();
				        }
				        break;
				    }
		        case 262:
			        if(pause==0)
			        {
				        if(mods==2)
				        {
				            ctrlrightpress=1;
				        }
				        else if(mods==4)
				        {
				            altrightpress=1;
				        }
				        else
				        {
				            x_change+=10;
				            check_pan();
				        }
				        break;
				    }
		        case GLFW_KEY_S:
                    skeypress=1;
			        break;
		        case GLFW_KEY_A:
                    akeypress=1;
			        break;
		        case GLFW_KEY_D:
                    dkeypress=1;
			        break;
                case GLFW_KEY_W:
                    wkeypress=1;
                    break;
                case GLFW_KEY_E:
                    ekeypress=1;
                    break;
		        case GLFW_KEY_C:
		        	if(life<0 || points>100)
		        	{
			        	ckeypress=1;
		    		}
		        	break;
		        case GLFW_KEY_SPACE:
			        if(fl1==1 && pause==0)
			        {
			            fl1=0;
			            x++;
			            if(x>0)
			            {
			                char t=x+'0';
			                string c = "laserbeamobject";
			                d = c + t;
			            }
			            createRectangle(d,laserObjects["lasernarrow"].angle,lasercolor,lasercolor,laserObjects["lasernarrow"].x+60*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f),laserObjects["lasernarrow"].y+60*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f),10,20,"laserobj",0);
			        }
                    spacekeypress=1;
			        break;
            	case GLFW_KEY_P:
            		if(pause==0)
            			pause=1;
            		else
            			pause=0;
         			break;
         		case GLFW_KEY_Y:
         			if(start==0)
         			{
           				start=1;
         			}
            	default:
            		break;
        }
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
            quit(window);
            break;
            default:
            break;
        }
        fkeypress=0;
        skeypress=0;
        dkeypress=0;
        akeypress=0;
        ctrlrightpress=0;
        ctrlleftpress=0;
        altrightpress=0;
        altleftpress=0;
        wkeypress=0;
        ekeypress=0;
        spacekeypress=0;
    }
}
void keyboardChar (GLFWwindow* window, unsigned int key)
{
    switch (key) {
        case 'Q':
        case 'q':
        quit(window);
        break;
        default:
        break;
    }
}
int mouse_clicked=0;
int right_mouse_clicked=0;
double mouse_x,mouse_y;
double mouse_x_old,mouse_y_old;

void mouse_click(){
    mouse_clicked=1;
    keyboard_pressed=0;
}
int angleforlaser=0;
void mouse_release(GLFWwindow* window, int button){ 
    mouse_clicked=0;
    selectreddrum=0;
    selectgreendrum=0;
    if(fl1==1 && angleforlaser==1)
    {
        fl1=0;
        x++;
        if(x>0)
        {
            char t=x+'0';
            string c = "laserbeamobject";
            d = c + t;
        }
        createRectangle(d,laserObjects["lasernarrow"].angle,lasercolor,lasercolor,laserObjects["lasernarrow"].x+60*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f),laserObjects["lasernarrow"].y+60*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f),10,20,"laserobj",0);
    }
}
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
    	if(pause==0)
    	{
	        case GLFW_MOUSE_BUTTON_LEFT:
	        if (action == GLFW_PRESS) {
	            mouse_click();
	        }
	        if (action == GLFW_RELEASE) {
	            mouse_release(window,button);
	        }
	        break;
	        case GLFW_MOUSE_BUTTON_RIGHT:
	        if (action == GLFW_PRESS) {
	            right_mouse_clicked=1;
	        }
	        if (action == GLFW_RELEASE) {
	            right_mouse_clicked=0;
	        }
	        break;
	        default:
	        break;
    	}
    }
}
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
       is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    GLfloat fov = 90.0f;

    // sets the viewport of openGL renderer
    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    // set the projection matrix as perspective
    /* glMatrixMode (GL_PROJECTION);
       glLoadIdentity ();
       gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
    // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-515.0f/zoom_camera, 515.0f/zoom_camera, -300.0f/zoom_camera, 300.0f/zoom_camera, 0.1f, 500.0f);
}

void createTriangle (string name, float angle, COLOR color, float x[], float y[], string component, int fill)
{
    /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

    /* Define vertex array acreateRectangle("backgroundobjtree1",0,brown1,brown1,315,-100,100,20,"background",0);s used in glBegin (GL_TRIANGLES) */
    float xc=(x[0]+x[1]+x[2])/3;
    float yc=(y[0]+y[1]+y[2])/3;
    GLfloat vertex_buffer_data [] = {
        x[0]-xc,y[0]-yc,0, // vertex 0
        x[1]-xc,y[1]-yc,0, // vertex 1
        x[2]-xc,y[2]-yc,0 // vertex 2
    };

    GLfloat color_buffer_data [] = {
        color.r,color.g,color.b, // color 1
        color.r,color.g,color.b, // color 2
        color.r,color.g,color.b // color 3
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    VAO *triangle;
    if(fill==1)
        triangle=create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
    else
        triangle=create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
    Game InstanceGame = {};
    InstanceGame.color = color;
    InstanceGame.name = name;
    InstanceGame.object = triangle;
    InstanceGame.x=(x[0]+x[1]+x[2])/3;
    InstanceGame.y=(y[0]+y[1]+y[2])/3;
    InstanceGame.height=-1;
    InstanceGame.width=-1;
    InstanceGame.x_speed=0;
    InstanceGame.y_speed=0;
    InstanceGame.radius=-1;
    InstanceGame.angle=angle;
    if(component=="heart")
        heart[name]=InstanceGame;
}

void createRectangle (string name, float angle, COLOR color,COLOR color2, float x, float y, float height, float width, string component,int status)
{
    // GL3 accepts only Triangles. Quads are not supported
    float w=width/2,h=height/2;
    GLfloat vertex_buffer_data [] = {
        -w,-h,0, // vertex 1
        -w,h,0, // vertex 2
        w,h,0, // vertex 3

        w,h,0, // vertex 3
        w,-h,0, // vertex 4
        -w,-h,0  // vertex 1
    };

    GLfloat color_buffer_data [] = {
        color.r,color.g,color.b, // color 1
        color.r,color.g,color.b, // color 2
        color2.r,color2.g,color2.b, // color 3

        color2.r,color2.g,color2.b, // color 4
        color2.r,color2.g,color2.b, // color 5
        color.r,color.g,color.b // color 6
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    VAO *rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
    Game InstanceGame = {};
    InstanceGame.color = color;
    InstanceGame.name = name;
    InstanceGame.object = rectangle;
    InstanceGame.x=x;
    InstanceGame.y=y;
    InstanceGame.height=height;
    InstanceGame.width=width;
    InstanceGame.x_speed=0;
    InstanceGame.y_speed=0;
    InstanceGame.radius=(sqrt(height*height+width*width))/2;
    InstanceGame.angle=angle;
    InstanceGame.status=status;
    if(component=="laser")
        laserObjects[name]=InstanceGame;
    else if(component=="background")
        backgroundObjects[name]=InstanceGame;
    else if(component=="laserobj")
        laserbeam[name]=InstanceGame;
    else if(component=="object")
        object[name]=InstanceGame;
    else if(component=="drum")
        drum[name]=InstanceGame;
    else if(component=="backgroundmirror")
        backgroundObjectsmirror[name]=InstanceGame;
    else if(component=="wall")
        Brickwall[name]=InstanceGame;
    else if(component=="scoreside")
        Scorewall[name]=InstanceGame;
    else if(component=="score")
        Score[name]=InstanceGame;
    else if(component=="mirrorback")
    	mirrorshade[name]=InstanceGame;
    else if(component=="prestart")
    	prestart[name]=InstanceGame;
    else if(component=="stage")
    	stage[name]=InstanceGame;
}
int lasercollision=0;
int hitcolor=0;
int goldenhit=0;
void collisionoflaser(string name)
{
    for(map<string,Game>::iterator it=object.begin();it!=object.end();)
    {
        string current = it->first;
        float dist1=(laserbeam[name].x-object[current].x);
        float dist2=(laserbeam[name].y-object[current].y);
        if(abs(dist1)<20 && abs(dist2)<15)
        {
        	COLOR black = {30/255.0,30/255.0,21/255.0};
        	if(object[current].color.r!=black.r && object[current].color.b!=black.b && object[current].color.g!=black.g)
        	{
        		hitcolor=1;
        	}
            object.erase(it++);
            lasercollision=1;
        }
        else
        {
            ++it;
        }
    }
    if(goldenhit==0)
    {
        if(abs(laserbeam[name].x-backgroundObjects["backgroundobjtree3"].x)<20 && abs(laserbeam[name].y-backgroundObjects["backgroundobjtree3"].y)<15)
        {
            map<string, Game>::iterator it = backgroundObjects.find("backgroundobjtree3");
            backgroundObjects.erase(it);
            goldenhit=1;
            points+=30;
        }
    } 
}
int bcollide=0;
void collisionofmirror(string name)
{
    for(map<string,Game>::iterator it=backgroundObjectsmirror.begin();it!=backgroundObjectsmirror.end();it++)
    {
        string current = it->first;
        COLOR skyblue1 = {123/255.0,201/255.0,227/255.0};
        COLOR skyblue = {132/255.0,217/255.0,245/255.0};
        float dist1,dist2;
        dist2=laserbeam[name].y+laserbeam[name].width*0.5*sin(laserbeam[name].angle*M_PI/180.0f);
        dist1=laserbeam[name].x+laserbeam[name].width*0.5*cos(laserbeam[name].angle*M_PI/180.0f);
        float dist3=(dist1-backgroundObjectsmirror[current].x)/cos(backgroundObjectsmirror[current].angle*M_PI/180.0f);
        float dist4=(dist2-backgroundObjectsmirror[current].y)/sin(backgroundObjectsmirror[current].angle*M_PI/180.0f);
        if(dist3>-30 && dist3<30 && dist4>-30 && dist4<30 && abs(dist3-dist4)<=10)
        {
            if(backgroundObjectsmirror[current].angle>0 && cos(laserbeam[name].angle*M_PI/180.0f)<-0.5 && sin(laserbeam[name].angle*M_PI/180.0f)<0)
            {
                bcollide=1;
            }
            else
            {
                laserbeam[name].angle=2*backgroundObjectsmirror[current].angle-laserbeam[name].angle;
            }        
        }
    }
}
void brickcollection(string name,COLOR color)
{
    for(map<string,Game>::iterator it=object.begin();it!=object.end();)
    {
        string current = it->first;
        if((drum[name].x-(drum[name].width)/2)<object[current].x+10 && (drum[name].x+(drum[name].width/2))>object[current].x-10 && object[current].y<-220)
        {
            if(drum[name].color.r==object[current].color.r && drum[name].color.g==object[current].color.g && drum[name].color.b==object[current].color.b )
            {
                int flag=0;
                if(abs(drum["reddrum"].x-drum["greendrum"].x)>=drum["reddrum"].width && object[current].y>-230)
                {
                    points+=10;
                }
            }
            COLOR black = {30/255.0,30/255.0,21/255.0};
            if(object[current].color.r==black.r && object[current].color.g==black.g && object[current].color.b==black.b && object[current].y>-230)
            {
                points-=10;
                if(points<0)
                {
                    points=0;
                }
                life--;
            }
            object.erase(it++);
        }      
        else
        {
            ++it;
        }
    }

}
int objectnum=0;
int fl=0;
void createCircle (string name, float angles, COLOR color, float x, float y, float r,float r1, int NoOfParts, string component, int fill)
{
    int parts = NoOfParts;
    float radius = r;
    GLfloat vertex_buffer_data[parts*9];
    GLfloat color_buffer_data[parts*9];
    int i,j;
    float angle=(2*M_PI/parts);
    float current_angle = 0;
    for(i=0;i<parts;i++){
        for(j=0;j<3;j++){
            color_buffer_data[i*9+j*3]=color.r;
            color_buffer_data[i*9+j*3+1]=color.g;
            color_buffer_data[i*9+j*3+2]=color.b;
        }
        vertex_buffer_data[i*9]=0;
        vertex_buffer_data[i*9+1]=0;
        vertex_buffer_data[i*9+2]=0;
        vertex_buffer_data[i*9+3]=radius*cos(current_angle);
        vertex_buffer_data[i*9+4]=r1*sin(current_angle);
        vertex_buffer_data[i*9+5]=0;
        vertex_buffer_data[i*9+6]=radius*cos(current_angle+angle);
        vertex_buffer_data[i*9+7]=r1*sin(current_angle+angle);
        vertex_buffer_data[i*9+8]=0;
        current_angle+=angle;
    }
    VAO* circle;
    if(fill==1)
        circle = create3DObject(GL_TRIANGLES, (parts*9)/3, vertex_buffer_data, color_buffer_data, GL_FILL);
    else
        circle = create3DObject(GL_TRIANGLES, (parts*9)/3, vertex_buffer_data, color_buffer_data, GL_LINE);
    Game InstanceGame = {};
    InstanceGame.color = color;
    InstanceGame.name = name;
    InstanceGame.object = circle;
    InstanceGame.x=x;
    InstanceGame.y=y;
    InstanceGame.height=2*r; //Height of the Game is 2*r
    InstanceGame.width=2*r; //Width of the Game is 2*r
    InstanceGame.x_speed=0;
    InstanceGame.y_speed=0;
    InstanceGame.radius=r;
    InstanceGame.angle=angles;
    if(component=="laser")
        laserObjects[name]=InstanceGame;
    else if(component=="background")
        backgroundObjects[name]=InstanceGame;
    else if(component=="laser1")
        lasercircle[name]=InstanceGame;
    else if(component=="drumcap")
        drumcap[name]=InstanceGame;
    else if(component=="prestart")
    	prestart[name]=InstanceGame;
    else if(component=="heart")
    	heart[name]=InstanceGame;
    else if(component=="circlebackground")
        circlebackground[name]=InstanceGame;
    else if(component=="circlebackground1")
        circlebackground1[name]=InstanceGame;
    else if(component=="circlebackground2")
        circlebackground2[name]=InstanceGame;
    else if(component=="circlebackground3")
        circlebackground3[name]=InstanceGame;
}

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;
float old_time; // Time in seconds
float cur_time; // Time in seconds
double mouse_pos_x, mouse_pos_y;
double new_mouse_pos_x, new_mouse_pos_y;
double new_mouse_pos_x1, new_mouse_pos_y1;

void statusforscore(char val){
    Score["top"].status=0;
    Score["bottom"].status=0;
    Score["middle"].status=0;
    Score["left1"].status=0;
    Score["left2"].status=0;
    Score["right1"].status=0;
    Score["right2"].status=0;
    Score["middle1"].status=0;
    Score["middle2"].status=0;
    Score["diagonal1"].status=0;
    Score["diagonal2"].status=0;
    Score["diagonal3"].status=0;
    Score["diagonal4"].status=0;
    if(val=='A'|| val=='G' || val=='E' || val=='F' || val=='T' || val=='P' || val=='O' || val=='C' || val=='S' || val=='0' || val=='2' || val=='3' || val=='5' || val=='6'|| val=='7' || val=='8' || val=='9'){
        Score["top"].status=1;
    }
    if(val=='A' || val=='E' || val=='F' || val=='P' || val=='S' || val=='2' || val=='3' || val=='4' || val=='5' || val=='6' || val=='8' || val=='9'){
        Score["middle"].status=1;
    }
    if(val=='G' || val=='W' || val=='U' || val=='E' || val=='L' || val=='O' || val=='C' || val=='S' || val=='0' || val=='2' || val=='3' || val=='5' || val=='6' || val=='8' || val=='9'){
        Score["bottom"].status=1;
    }
    if(val=='A' || val=='G' || val=='M' || val=='W' || val=='U' || val=='E' || val=='F' || val=='L' || val=='N' || val=='P' || val=='O' || val=='C' || val=='S' || val=='0' || val=='4' || val=='5' || val=='6' || val=='8' || val=='9' ){
        Score["left1"].status=1;
    }
    if(val=='A' || val=='G' || val=='M' || val=='W' || val=='U' || val=='E' || val=='F' || val=='L' || val=='N' || val=='P' || val=='O' || val=='C' || val=='0' || val=='2' || val=='6' || val=='8'){
        Score["left2"].status=1;
    }
    if(val=='A' || val=='M' || val=='W' || val=='U' || val=='N' || val=='P'  || val=='O' || val=='0' || val=='1' || val=='2' || val=='3' || val=='4' || val=='7' || val=='8' || val=='9'){
        Score["right1"].status=1;
    }
    if(val=='A' || val=='G' || val=='M' || val=='W' || val=='U' || val=='N' || val=='O' || val=='S' || val=='0' || val=='1' || val=='3' || val=='4' || val=='5' || val=='6' || val=='7' || val=='8' || val=='9'){
        Score["right2"].status=1;
    }
    if(val=='T' || val=='I')
    {
        Score["middle1"].status=1;
    }
    if(val=='W' || val=='T' || val=='I' || val=='Y')
    {
        Score["middle2"].status=1;
    }
    if(val=='M' || val=='N' || val=='Y')
    {
        Score["diagonal1"].status=1;
    }
    if(val=='M' || val=='Y')
    {
    	Score["diagonal2"].status=1;
    }
    if(val=='N')
    {
        Score["diagonal4"].status=1;
    }
}
void draw (GLFWwindow* window)
{
    glfwGetCursorPos(window, &new_mouse_pos_x1, &new_mouse_pos_y1);
    if(right_mouse_clicked==1 && pause==0)
    {
        x_change+=new_mouse_pos_x1-mouse_pos_x;
        y_change-=new_mouse_pos_y1-mouse_pos_y;
        check_pan();
    }
    Matrices.projection = glm::ortho((float)(-515.0f/zoom_camera+x_change), (float)(515.0f/zoom_camera+x_change), (float)(-300.0f/zoom_camera+y_change), (float)(300.0f/zoom_camera+y_change), 0.1f, 500.0f);
    glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);
    if(mouse_clicked==1 && pause==0)
    {
        glfwGetCursorPos(window, &new_mouse_pos_x, &new_mouse_pos_y);
        angleforlaser=0;
        if(new_mouse_pos_y<500 && new_mouse_pos_x>50)
        {    
            angleforlaser=1;   
            float dist1 = (new_mouse_pos_y)+(laserObjects["laserbroad"].y)-300;
            float dist2 = (new_mouse_pos_x)-(laserObjects["laserbroad"].x)-515;
            float dist3 = (new_mouse_pos_y)+(laserObjects["lasernarrow"].y)-300;
            float dist4 = (new_mouse_pos_x)-(laserObjects["lasernarrow"].x)-515;
            float angle_rad = -atan(dist1/dist2);
            float angle_rad1 = -atan(dist3/dist4);
            float angle = angle_rad/M_PI*180.0f;
            float angle1 = angle_rad1/M_PI*180.0f;
            if(angle>45)
            {
                angle=45;
                angle1=45;
            }
            else if(angle<-45)
            {
                angle=-45;
                angle1=-45;
            }
            laserObjects["laserbroad"].angle=angle;
            laserObjects["lasernarrow"].angle=angle1;
            lasercircle["lasercircle1"].angle=angle;
            lasercircle["lasercircle2"].angle=angle;
            lasercircle["lasercircle3"].angle=angle;
            lasercircle["lasercircle4"].angle=angle;
            lasercircle["lasercircle2"].x=laserObjects["lasernarrow"].x+(laserObjects["lasernarrow"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
            lasercircle["lasercircle2"].y=laserObjects["lasernarrow"].y+(laserObjects["lasernarrow"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
            lasercircle["lasercircle1"].x=laserObjects["lasernarrow"].x+(laserObjects["lasernarrow"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
            lasercircle["lasercircle1"].y=laserObjects["lasernarrow"].y+(laserObjects["lasernarrow"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
            lasercircle["lasercircle3"].x=laserObjects["laserbroad"].x-2+(laserObjects["laserbroad"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
            lasercircle["lasercircle3"].y=laserObjects["laserbroad"].y+(laserObjects["laserbroad"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
            lasercircle["lasercircle4"].x=laserObjects["laserbroad"].x+(laserObjects["laserbroad"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
            lasercircle["lasercircle4"].y=laserObjects["laserbroad"].y+(laserObjects["laserbroad"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
        }
        else if(new_mouse_pos_y<500 && new_mouse_pos_x<=50)
        {
            float w = (laserObjects["laserbroad"].width)/2;
            float h = (laserObjects["laserbroad"].height)/2;
            if(laserObjects["laserbroad"].x+515<new_mouse_pos_x && laserObjects["laserbroad"].x+w+515>new_mouse_pos_x && -laserObjects["laserbroad"].y-h+300<new_mouse_pos_y && -laserObjects["laserbroad"].y+h+300>new_mouse_pos_y)
            {
                laserObjects["laserbroad"].y=300-new_mouse_pos_y;
                laserObjects["lasernarrow"].y=300-new_mouse_pos_y;
                laserObjects["lasercircle"].y=300-new_mouse_pos_y;
                lasercircle["lasercircle1"].y=300-new_mouse_pos_y+laserObjects["lasernarrow"].width*0.5*sin(laserObjects["lasernarrow"].angle/180*M_PI);
            	lasercircle["lasercircle2"].y=300-new_mouse_pos_y+laserObjects["lasernarrow"].width*0.5*sin(laserObjects["lasernarrow"].angle/180*M_PI);
            	lasercircle["lasercircle3"].y=300-new_mouse_pos_y+laserObjects["laserbroad"].width*0.5*sin(laserObjects["laserbroad"].angle/180*M_PI);
            	lasercircle["lasercircle4"].y=300-new_mouse_pos_y+laserObjects["laserbroad"].width*0.5*sin(laserObjects["laserbroad"].angle/180*M_PI);
                if(laserObjects["laserbroad"].y<-160)
                {
                    laserObjects["laserbroad"].y=-160;
                    laserObjects["lasernarrow"].y=-160;
                    laserObjects["lasercircle"].y=-160;
                    lasercircle["lasercircle1"].y=-160+laserObjects["lasernarrow"].width*0.5*sin(laserObjects["lasernarrow"].angle/180*M_PI);
            		lasercircle["lasercircle2"].y=-160+laserObjects["lasernarrow"].width*0.5*sin(laserObjects["lasernarrow"].angle/180*M_PI);
            		lasercircle["lasercircle3"].y=-160+laserObjects["laserbroad"].width*0.5*sin(laserObjects["laserbroad"].angle/180*M_PI);
            		lasercircle["lasercircle4"].y=-160+laserObjects["laserbroad"].width*0.5*sin(laserObjects["laserbroad"].angle/180*M_PI);
                }
                if(laserObjects["laserbroad"].y>260)
                {
                    laserObjects["laserbroad"].y=260;
                    laserObjects["lasernarrow"].y=260;
                    laserObjects["lasercircle"].y=260;
                    lasercircle["lasercircle1"].y=260+laserObjects["lasernarrow"].width*0.5*sin(laserObjects["lasernarrow"].angle/180*M_PI);
            		lasercircle["lasercircle2"].y=260+laserObjects["lasernarrow"].width*0.5*sin(laserObjects["lasernarrow"].angle/180*M_PI);
            		lasercircle["lasercircle3"].y=260+laserObjects["laserbroad"].width*0.5*sin(laserObjects["laserbroad"].angle/180*M_PI);
            		lasercircle["lasercircle4"].y=260+laserObjects["laserbroad"].width*0.5*sin(laserObjects["laserbroad"].angle/180*M_PI);
                }
            }         
        }
        else if(new_mouse_pos_y>515)
        {
            float w = (drum["reddrum"].width)/2;
            float h = (drum["reddrum"].height)/2;
            if(selectgreendrum==0 && drum["reddrum"].x-w+515<new_mouse_pos_x && drum["reddrum"].x+w+515>new_mouse_pos_x && -drum["reddrum"].y-h+300<new_mouse_pos_y && -drum["reddrum"].y+h+300>new_mouse_pos_y)
            {
                selectreddrum=1;
                selectgreendrum=0;
                drum["reddrum"].x=new_mouse_pos_x-515;
                drumcap["drumcap1"].x=new_mouse_pos_x-515;
                drumcap["drumcap2"].x=new_mouse_pos_x-515;
                if(drum["reddrum"].x-w<-485)
                {
                    drum["reddrum"].x=-485+w;
                    drumcap["drumcap1"].x=-485+w;
                    drumcap["drumcap2"].x=-485+w;
                }
                else if(drum["reddrum"].x+w>315)
                {
                    drum["reddrum"].x=315-w;
                    drumcap["drumcap1"].x=315-w;
                    drumcap["drumcap2"].x=315-w;
                }
            }
            else if(selectreddrum==0 && drum["greendrum"].x-w+515<new_mouse_pos_x && drum["greendrum"].x+w+515>new_mouse_pos_x && -drum["greendrum"].y-h+300<new_mouse_pos_y && -drum["greendrum"].y+h+300>new_mouse_pos_y)
            {
                selectgreendrum=1;
                selectreddrum=0;
                drum["greendrum"].x=new_mouse_pos_x-515;
                drumcap["drumcap3"].x=new_mouse_pos_x-515;
                drumcap["drumcap4"].x=new_mouse_pos_x-515;
                if(drum["greendrum"].x-w<-485)
                {
                    drum["greendrum"].x=-485+w;
                    drumcap["drumcap3"].x=-485+w;
                    drumcap["drumcap4"].x=-485+w;
                }
                else if(drum["greendrum"].x+w>315)
                {
                    drum["greendrum"].x=315-w;
                    drumcap["drumcap3"].x=315-w;
                    drumcap["drumcap4"].x=315-w;
                }
            }
        }
    }
    if(fkeypress==1)
    {
        if(laserObjects["laserbroad"].y>-160 && pause==0)
                    {
                        laserObjects["laserbroad"].y-=2;
                        laserObjects["lasernarrow"].y-=2;
                        laserObjects["lasercircle"].y-=2;
                        lasercircle["lasercircle1"].y=laserObjects["lasernarrow"].y+laserObjects["lasernarrow"].width*0.5*sin(laserObjects["lasernarrow"].angle/180*M_PI);
                        lasercircle["lasercircle2"].y=laserObjects["lasernarrow"].y+laserObjects["lasernarrow"].width*0.5*sin(laserObjects["lasernarrow"].angle/180*M_PI);
                        lasercircle["lasercircle3"].y=laserObjects["laserbroad"].y+laserObjects["laserbroad"].width*0.5*sin(laserObjects["laserbroad"].angle/180*M_PI);
                        lasercircle["lasercircle4"].y=laserObjects["laserbroad"].y+laserObjects["laserbroad"].width*0.5*sin(laserObjects["laserbroad"].angle/180*M_PI);
                    }
    }
    if(skeypress==1)
    {
        if(laserObjects["laserbroad"].y<260 && pause==0)
                    {
                        laserObjects["laserbroad"].y+=2;
                        laserObjects["lasernarrow"].y+=2;
                        laserObjects["lasercircle"].y+=2;
                        lasercircle["lasercircle1"].y=laserObjects["lasernarrow"].y+laserObjects["lasernarrow"].width*0.5*sin(laserObjects["lasernarrow"].angle/180*M_PI);
                        lasercircle["lasercircle2"].y=laserObjects["lasernarrow"].y+laserObjects["lasernarrow"].width*0.5*sin(laserObjects["lasernarrow"].angle/180*M_PI);
                        lasercircle["lasercircle3"].y=laserObjects["laserbroad"].y+laserObjects["laserbroad"].width*0.5*sin(laserObjects["laserbroad"].angle/180*M_PI);
                        lasercircle["lasercircle4"].y=laserObjects["laserbroad"].y+laserObjects["laserbroad"].width*0.5*sin(laserObjects["laserbroad"].angle/180*M_PI);
                    }
    }
    if(akeypress==1)
    {
        if(laserObjects["laserbroad"].angle<45 && pause==0)
                    {
                        laserObjects["laserbroad"].angle+=1;
                        laserObjects["lasernarrow"].angle+=1;
                        float angle=laserObjects["lasernarrow"].angle;
                        lasercircle["lasercircle1"].angle=angle;
                        lasercircle["lasercircle2"].angle=angle;
                        lasercircle["lasercircle3"].angle=angle;
                        lasercircle["lasercircle4"].angle=angle;
                        lasercircle["lasercircle2"].x=laserObjects["lasernarrow"].x+(laserObjects["lasernarrow"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle2"].y=laserObjects["lasernarrow"].y+(laserObjects["lasernarrow"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle1"].x=laserObjects["lasernarrow"].x+(laserObjects["lasernarrow"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle1"].y=laserObjects["lasernarrow"].y+(laserObjects["lasernarrow"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle3"].x=laserObjects["laserbroad"].x-2+(laserObjects["laserbroad"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle3"].y=laserObjects["laserbroad"].y+(laserObjects["laserbroad"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle4"].x=laserObjects["laserbroad"].x+(laserObjects["laserbroad"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle4"].y=laserObjects["laserbroad"].y+(laserObjects["laserbroad"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                    }
    }
    if(dkeypress==1)
    {
         if(laserObjects["laserbroad"].angle>-45 && pause==0)
                    {
                        laserObjects["laserbroad"].angle-=1;
                        laserObjects["lasernarrow"].angle-=1;
                        float angle=laserObjects["lasernarrow"].angle;
                        lasercircle["lasercircle1"].angle=angle;
                        lasercircle["lasercircle2"].angle=angle;
                        lasercircle["lasercircle3"].angle=angle;
                        lasercircle["lasercircle4"].angle=angle;
                        lasercircle["lasercircle2"].x=laserObjects["lasernarrow"].x+(laserObjects["lasernarrow"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle2"].y=laserObjects["lasernarrow"].y+(laserObjects["lasernarrow"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle1"].x=laserObjects["lasernarrow"].x+(laserObjects["lasernarrow"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle1"].y=laserObjects["lasernarrow"].y+(laserObjects["lasernarrow"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle3"].x=laserObjects["laserbroad"].x-2+(laserObjects["laserbroad"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle3"].y=laserObjects["laserbroad"].y+(laserObjects["laserbroad"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle4"].x=laserObjects["laserbroad"].x+(laserObjects["laserbroad"].width)*0.5*cos(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        lasercircle["lasercircle4"].y=laserObjects["laserbroad"].y+(laserObjects["laserbroad"].width)*0.5*sin(laserObjects["lasernarrow"].angle*M_PI/180.0f);
                        
                    }
    }
    if(ctrlleftpress==1)
    {
        if(drum["reddrum"].x>-400)
        {
            drum["reddrum"].x-=2;
            drumcap["drumcap1"].x-=2;
            drumcap["drumcap2"].x-=2;
        }
    }
    if(ctrlrightpress==1)
    {
        if(drum["reddrum"].x<255)
        {
            drum["reddrum"].x+=2;
            drumcap["drumcap1"].x+=2;
            drumcap["drumcap2"].x+=2;
        }
    }
    if(altleftpress==1)
    {
        if(drum["greendrum"].x>-400)
        {
            drum["greendrum"].x-=2;
            drumcap["drumcap3"].x-=2;
            drumcap["drumcap4"].x-=2;
        }
    }
    if(altrightpress==1)
    {
        if(drum["greendrum"].x<225)
        {
            drum["greendrum"].x+=2;
            drumcap["drumcap3"].x+=2;
            drumcap["drumcap4"].x+=2;
        }
    }
    if(wkeypress==1)
    {
        if(backgroundObjectsmirror["mirror1"].y<200 && pause==0)
        {
            backgroundObjectsmirror["mirror1b"].y+=2;
            backgroundObjectsmirror["mirror1"].y+=2;
            mirrorshade["mirrorshade1"].y+=2;
            mirrorshade["mirrorshade2"].y+=2;
            mirrorshade["mirrorshade3"].y+=2;
            mirrorshade["mirrorshade4"].y+=2;
            mirrorshade["mirrorshade5"].y+=2;
            mirrorshade["mirrorshade6"].y+=2;
        }
    }
    if(ekeypress==1)
    {
        if(backgroundObjectsmirror["mirror1"].y>-170 && pause==0)
        {
            backgroundObjectsmirror["mirror1b"].y-=2;
            backgroundObjectsmirror["mirror1"].y-=2;
            mirrorshade["mirrorshade1"].y-=2;
            mirrorshade["mirrorshade2"].y-=2;
            mirrorshade["mirrorshade3"].y-=2;
            mirrorshade["mirrorshade4"].y-=2;
            mirrorshade["mirrorshade5"].y-=2;
            mirrorshade["mirrorshade6"].y-=2;
        }
    }
    if(keyboard_pressed==1 && pause==0)
    { 
        int i;
        string c="laserbeamobject";
        char e;
        COLOR blue = laserObjects["laserbroad"].color;
        float angle = laserObjects["laserbroad"].angle;
        createRectangle("laserbroad",angle,blue,blue,laserObjects["laserbroad"].x,laserObjects["laserbroad"].y,40,40,"laser",0);
        createRectangle("lasernarrow",angle,blue,blue,laserObjects["lasernarrow"].x,laserObjects["lasernarrow"].y,20,110,"laser",0);
        createCircle("lasercircle",0,blue,laserObjects["lasercircle"].x,laserObjects["lasercircle"].y,30,30,10,"laser",0);
    }
    if(start==0)
	{
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram (programID);
		// Eye - Location of camera. Don't change unless you are sure!!
        glm::vec3 eye(eye_x,eye_y,eye_z);
		//glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
		// Target - Where is the camera looking at.  Don't change unless you are sure!!
		glm::vec3 target (0, 0, 0);
		// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
		glm::vec3 up (0, 1, 0);

		// Compute Camera matrix (view)
		//Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
		//  Don't change unless you are sure!!
		Matrices.view = glm::lookAt(glm::vec3(0,0,1), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

		// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
		//  Don't change unless you are sure!!
		//glm::mat4 VP = Matrices.projection * Matrices.view;

			glm::mat4 VP = Matrices.projection * Matrices.view;
	    	COLOR white={0,0,0};
	    	glm::mat4 MVP;  // MVP = Projection * View * Model
		    Matrices.model = glm::mat4(1.0f);
		    glm::mat4 ObjectTransform;
		    glm::mat4 translateObject = glm::translate (glm::vec3(backgroundObjects["backgroundobj1"].x, backgroundObjects["backgroundobj1"].y, 0.0f)); 
		    glm::mat4 rotateTriangle = glm::rotate((float)((backgroundObjects["backgroundobj1"].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
		    ObjectTransform=translateObject*rotateTriangle;
		    Matrices.model *= ObjectTransform;
		    MVP = VP * Matrices.model; // MVP = p * V * M		    
		    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);
	    	draw3DObject(backgroundObjects["backgroundobj1"].object);
	    	for(map<string,Game>::iterator it=prestart.begin();it!=prestart.end();it++){
	        	string current = it->first; 
	        
	            glm::mat4 MVP;  // MVP = Projection * View * Model

	            Matrices.model = glm::mat4(1.0f);

	            /* Render your scene */
	            glm::mat4 ObjectTransform;
	            glm::mat4 translateObject = glm::translate (glm::vec3(prestart[current].x,prestart[current].y,0.0f)); // glTranslatef
	            glm::mat4 rotateTriangle = glm::rotate((float)((prestart[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
	            ObjectTransform=translateObject*rotateTriangle;
	            Matrices.model *= ObjectTransform;
	            MVP = VP * Matrices.model; // MVP = p * V * M

	            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	            draw3DObject(prestart[current].object);
	    }
    	int k;
    	for(k=1;k<=7;k++)
		{
		    float translation;
		    float translation1=0;
		    if(k==1)
		    {
		        statusforscore('W');
		        translation=-90;
		    }
		    else if(k==2)
		    {
		        statusforscore('E');
		        translation=-60;
		    }
		    else if(k==3)
		    {
		        statusforscore('L');
		        translation=-30;
		    }
		    else if(k==4)
		    {
		        statusforscore('C');
		        translation=0;
		    }
		    else if(k==5)
		    {
		        statusforscore('O');
		        translation=30;
		    }
		    else if(k==6)
		    {
		        statusforscore('M');
		        translation=60;
		    }
		    else if(k==7)
		    {
		    	statusforscore('E');
		    	translation=90;
		    }
		    for(map<string,Game>::iterator it=Score.begin();it!=Score.end();it++){
		        string current = it->first; 
		        if(Score[current].status==1)
		        {
		            glm::mat4 MVP;  // MVP = Projection * View * Model

		            Matrices.model = glm::mat4(1.0f);

		            /* Render your scene */
		            glm::mat4 ObjectTransform;
		            glm::mat4 translateObject = glm::translate (glm::vec3(Score[current].x+translation,Score[current].y+translation1,0.0f)); // glTranslatef
		            glm::mat4 rotateTriangle = glm::rotate((float)((Score[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
		            ObjectTransform=translateObject*rotateTriangle;
		            Matrices.model *= ObjectTransform;
		            MVP = VP * Matrices.model; // MVP = p * V * M

		            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		            draw3DObject(Score[current].object);
		        }
		    }
		}
	}
    if(life>=0 && points<=100 && pause==0 && start==1)
    {
				int side= rand()%2;
				if(fl==1)
				{
				    fl=0;
				    objectnum++;
				    char e;
				    string c="fallingobject";
				    e=objectnum+'0';
				    d=c+e;
				    int LO,HI,color;
				    COLOR red = {255.0/255.0,51.0/255.0,51.0/255.0};
				    COLOR lightgreen = {57/255.0,230/255.0,0/255.0};
				    COLOR black = {30/255.0,30/255.0,21/255.0};
				    COLOR objectcolor;
				    color = rand()%3;
				    float r3;
				    if(color==0)
				    {
				        objectcolor=red;
				    }
				    else if(color==1)
				    {
				        objectcolor=lightgreen;
				    }
				    else
				    {
				        objectcolor=black;
				    }
				    if(side==0)
				    {
				        LO = -405;
				        HI = -125;
				        r3 = LO + rand()%(HI-LO);
				    }
				    else
				    {
				        LO = -35;
				        HI = 215;
				        r3 = LO + rand()%(HI-LO);
				    }
				    createRectangle(d,0,objectcolor,objectcolor,r3,300,20,20,"object",0);
				}
				glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glUseProgram (programID);
				// Eye - Location of camera. Don't change unless you are sure!!
				glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
				// Target - Where is the camera looking at.  Don't change unless you are sure!!
				glm::vec3 target (0, 0, 0);
				// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
				glm::vec3 up (0, 1, 0);

				// Compute Camera matrix (view)
				//Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
				//  Don't change unless you are sure!!
				Matrices.view = glm::lookAt(glm::vec3(0,0,1), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

				// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
				//  Don't change unless you are sure!!
				glm::mat4 VP = Matrices.projection * Matrices.view;

				//Draw the background

				for(map<string,Game>::iterator it=backgroundObjects.begin();it!=backgroundObjects.end();it++){
				    string current = it->first; //The name of the current object
				    glm::mat4 MVP;  // MVP = Projection * View * Model

				    Matrices.model = glm::mat4(1.0f);

				    glm::mat4 ObjectTransform;
				    glm::mat4 translateObject = glm::translate (glm::vec3(backgroundObjects[current].x, backgroundObjects[current].y, 0.0f)); 
				    glm::mat4 rotateTriangle = glm::rotate((float)((backgroundObjects[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
				    ObjectTransform=translateObject*rotateTriangle;
				    Matrices.model *= ObjectTransform;
				    MVP = VP * Matrices.model; // MVP = p * V * M
				    
				    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

				    draw3DObject(backgroundObjects[current].object);
				    //glPopMatrix (); 
				}
                int j;
                float translationx=-515,translationy=-250;
                for(j=0;j<=20;j++)
                {
                    translationx+=100;
                    for(map<string,Game>::iterator it=circlebackground.begin();it!=circlebackground.end();it++){
                        string current = it->first; //The name of the current object
                        glm::mat4 MVP;  // MVP = Projection * View * Model

                        Matrices.model = glm::mat4(1.0f);

                        glm::mat4 ObjectTransform;
                        glm::mat4 translateObject = glm::translate (glm::vec3(circlebackground[current].x+translationx, circlebackground[current].y+translationy, 0.0f)); 
                        glm::mat4 rotateTriangle = glm::rotate((float)((circlebackground[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
                        ObjectTransform=translateObject*rotateTriangle;
                        Matrices.model *= ObjectTransform;
                        MVP = VP * Matrices.model; // MVP = p * V * M
                        
                        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

                        draw3DObject(circlebackground[current].object);
                        //glPopMatrix ();
                    }
                }
                translationx=-565,translationy=-260;
                for(j=0;j<=20;j++)
                {
                    translationx+=100;
                    for(map<string,Game>::iterator it=circlebackground1.begin();it!=circlebackground1.end();it++){
                        string current = it->first; //The name of the current object
                        glm::mat4 MVP;  // MVP = Projection * View * Model

                        Matrices.model = glm::mat4(1.0f);

                        glm::mat4 ObjectTransform;
                        glm::mat4 translateObject = glm::translate (glm::vec3(circlebackground1[current].x+translationx, circlebackground1[current].y+translationy, 0.0f)); 
                        glm::mat4 rotateTriangle = glm::rotate((float)((circlebackground1[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
                        ObjectTransform=translateObject*rotateTriangle;
                        Matrices.model *= ObjectTransform;
                        MVP = VP * Matrices.model; // MVP = p * V * M
                        
                        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

                        draw3DObject(circlebackground1[current].object);
                        //glPopMatrix (); 
                    }
                }
                translationx=-595,translationy=-280;
                for(j=0;j<=20;j++)
                {
                    translationx+=100;
                    for(map<string,Game>::iterator it=circlebackground2.begin();it!=circlebackground2.end();it++){
                        string current = it->first; //The name of the current object
                        glm::mat4 MVP;  // MVP = Projection * View * Model

                        Matrices.model = glm::mat4(1.0f);

                        glm::mat4 ObjectTransform;
                        glm::mat4 translateObject = glm::translate (glm::vec3(circlebackground2[current].x+translationx, circlebackground2[current].y+translationy, 0.0f)); 
                        glm::mat4 rotateTriangle = glm::rotate((float)((circlebackground2[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
                        ObjectTransform=translateObject*rotateTriangle;
                        Matrices.model *= ObjectTransform;
                        MVP = VP * Matrices.model; // MVP = p * V * M
                        
                        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

                        draw3DObject(circlebackground2[current].object);
                        //glPopMatrix (); 
                    }
                }
                translationx=-515,translationy=-300;
                for(j=0;j<=20;j++)
                {
                    translationx+=100;
                    for(map<string,Game>::iterator it=circlebackground3.begin();it!=circlebackground3.end();it++){
                        string current = it->first; //The name of the current object
                        glm::mat4 MVP;  // MVP = Projection * View * Model

                        Matrices.model = glm::mat4(1.0f);

                        glm::mat4 ObjectTransform;
                        glm::mat4 translateObject = glm::translate (glm::vec3(circlebackground3[current].x+translationx, circlebackground3[current].y+translationy, 0.0f)); 
                        glm::mat4 rotateTriangle = glm::rotate((float)((circlebackground3[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
                        ObjectTransform=translateObject*rotateTriangle;
                        Matrices.model *= ObjectTransform;
                        MVP = VP * Matrices.model; // MVP = p * V * M
                        
                        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

                        draw3DObject(circlebackground3[current].object);
                        //glPopMatrix (); 
                    }
                }
				for(map<string,Game>::iterator it=mirrorshade.begin();it!=mirrorshade.end();it++){
				    string current = it->first; //The name of the current object
				    glm::mat4 MVP;  // MVP = Projection * View * Model

				    Matrices.model = glm::mat4(1.0f);

				    glm::mat4 ObjectTransform;
				    glm::mat4 translateObject = glm::translate (glm::vec3(mirrorshade[current].x, mirrorshade[current].y, 0.0f)); 
				    glm::mat4 rotateTriangle = glm::rotate((float)((mirrorshade[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
				    ObjectTransform=translateObject*rotateTriangle;
				    Matrices.model *= ObjectTransform;
				    MVP = VP * Matrices.model; // MVP = p * V * M
				    
				    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

				    draw3DObject(mirrorshade[current].object);
				    //glPopMatrix (); 
				}
				for(map<string,Game>::iterator it=backgroundObjectsmirror.begin();it!=backgroundObjectsmirror.end();it++){
				    string current = it->first; //The name of the current object
				    glm::mat4 MVP;  // MVP = Projection * View * Model

				    Matrices.model = glm::mat4(1.0f);

				    glm::mat4 ObjectTransform;
				    glm::mat4 translateObject = glm::translate (glm::vec3(backgroundObjectsmirror[current].x, backgroundObjectsmirror[current].y, 0.0f)); 
				    glm::mat4 rotateTriangle = glm::rotate((float)((backgroundObjectsmirror[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
				    ObjectTransform=translateObject*rotateTriangle;
				    Matrices.model *= ObjectTransform;
				    MVP = VP * Matrices.model; // MVP = p * V * M
				    
				    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

				    draw3DObject(backgroundObjectsmirror[current].object);
				    //glPopMatrix (); 
				}
				for(map<string,Game>::iterator it=laserbeam.begin();it!=laserbeam.end();){
				    string current = it->first;
				    if(laserbeam[current].x>345 || laserbeam[current].x<-515 || laserbeam[current].y>345 || laserbeam[current].y<-190)
				    {
				        laserbeam.erase(it++);
				    }
				    else
				    {
				        ++it;
				    }
				}
				for(map<string,Game>::iterator it=laserbeam.begin();it!=laserbeam.end();){
				    string current = it->first;
				    laserbeam[current].x+=5*cos(laserbeam[current].angle*M_PI/180.0f);
				    laserbeam[current].y+=5*sin(laserbeam[current].angle*M_PI/180.0f);
				    createRectangle(current,laserbeam[current].angle,laserbeam[current].color,laserbeam[current].color,laserbeam[current].x,laserbeam[current].y,10,20,"laserobj",0);
				    
				    
				    glm::mat4 MVP;  // MVP = Projection * View * Model

				    Matrices.model = glm::mat4(1.0f);

				    /* Render your scene */
				    glm::mat4 ObjectTransform;
				     // glTranslatef
				    glm::mat4 rotateTriangle = glm::rotate((float)((laserbeam[current].angle)*M_PI/180.0f), glm::vec3(0,0,1)); 
				    
				    glm::mat4 translateObject = glm::translate (glm::vec3(laserbeam[current].x, laserbeam[current].y, 0.0f)); // rotate about vector (1,0,0)
				    //glm::mat4 translateObject2 = glm::translate (glm::vec3(x_diff, y_diff, 0.0f)); // glTranslatef*/
				    ObjectTransform=translateObject*rotateTriangle;
				    Matrices.model *= ObjectTransform;
				    MVP = VP * Matrices.model; // MVP = p * V * M

				    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				    draw3DObject(laserbeam[current].object);
				    bcollide=0;
				    lasercollision=0;
				    collisionoflaser(current);
				    if(lasercollision==1)
				    {
				    	if(hitcolor==1)
				    	{
                            points-=3;
                            if(points<0)
                            {
                                points=0;
                            }
				    		hitcolor=0;
				    		life--;
				    	}
				        laserbeam.erase(it++);
				    }
                    if(goldenhit==1 && goldenflag==0)
                    {
                        goldenflag=1;
                    }
				    if(lasercollision==0)
				    {
				        collisionofmirror(current);
				        if(bcollide==1)
				        {
				            laserbeam.erase(it++);
				        }
				        else
				        {
				            ++it;
				        }
				    }
				}
				for(map<string,Game>::iterator it=drum.begin();it!=drum.end();it++){
				    string current = it->first; 
				    glm::mat4 MVP;  // MVP = Projection * View * Model

				    Matrices.model = glm::mat4(1.0f);

				    /* Render your scene */
				    glm::mat4 ObjectTransform;
				    glm::mat4 translateObject = glm::translate (glm::vec3(drum[current].x, drum[current].y, 0.0f)); // glTranslatef
				    glm::mat4 rotateTriangle = glm::rotate((float)((drum[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				    ObjectTransform=translateObject*rotateTriangle;
				    Matrices.model *= ObjectTransform;
				    MVP = VP * Matrices.model; // MVP = p * V * M

				    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				    draw3DObject(drum[current].object);
				    brickcollection(current,drum[current].color);
				}
				for(map<string,Game>::iterator it=drumcap.begin();it!=drumcap.end();it++){
				    string current = it->first; 
				    glm::mat4 MVP;  // MVP = Projection * View * Model

				    Matrices.model = glm::mat4(1.0f);

				    /* Render your scene */
				    glm::mat4 ObjectTransform;
				    glm::mat4 translateObject = glm::translate (glm::vec3(drumcap[current].x, drumcap[current].y, 0.0f)); // glTranslatef
				    glm::mat4 rotateTriangle = glm::rotate((float)((drumcap[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				    ObjectTransform=translateObject*rotateTriangle;
				    Matrices.model *= ObjectTransform;
				    MVP = VP * Matrices.model; // MVP = p * V * M

				    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				    draw3DObject(drumcap[current].object);
				}
				for(map<string,Game>::iterator it=object.begin();it!=object.end();){
				    string current = it->first; 
				    if(object[current].y<-300)
				    {
				        object.erase(it++);
				    }
				    else
				    {
				        ++it;
				        glm::mat4 MVP;  // MVP = Projection * View * Model

				        Matrices.model = glm::mat4(1.0f);

				        /* Render your scene */
				        glm::mat4 ObjectTransform;
				        glm::mat4 translateObject = glm::translate (glm::vec3(object[current].x, object[current].y, 0.0f)); // glTranslatef
				        glm::mat4 rotateTriangle = glm::rotate((float)((object[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				        ObjectTransform=translateObject*rotateTriangle;
				        Matrices.model *= ObjectTransform;
				        MVP = VP * Matrices.model; // MVP = p * V * M

				        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				        draw3DObject(object[current].object);
				        object[current].y-=speed;
				    }
				}

				for(map<string,Game>::iterator it=laserObjects.begin();it!=laserObjects.end();it++){
				    string current = it->first; 
				    glm::mat4 MVP;  // MVP = Projection * View * Model

				    Matrices.model = glm::mat4(1.0f);

				    /* Render your scene */
				    glm::mat4 ObjectTransform;
				    glm::mat4 translateObject = glm::translate (glm::vec3(laserObjects[current].x, laserObjects[current].y, 0.0f)); // glTranslatef
				    glm::mat4 rotateTriangle = glm::rotate((float)((laserObjects[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				    ObjectTransform=translateObject*rotateTriangle;
				    Matrices.model *= ObjectTransform;
				    MVP = VP * Matrices.model; // MVP = p * V * M

				    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				    draw3DObject(laserObjects[current].object);
				}
				for(map<string,Game>::iterator it=Scorewall.begin();it!=Scorewall.end();it++){
				    string current = it->first; //The name of the current object
				    glm::mat4 MVP;  // MVP = Projection * View * Model

				    Matrices.model = glm::mat4(1.0f);

				    glm::mat4 ObjectTransform;
				    glm::mat4 translateObject = glm::translate (glm::vec3(Scorewall[current].x, Scorewall[current].y, 0.0f)); 
				    glm::mat4 rotateTriangle = glm::rotate((float)((Scorewall[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
				    ObjectTransform=translateObject*rotateTriangle;
				    Matrices.model *= ObjectTransform;
				    MVP = VP * Matrices.model; // MVP = p * V * M
				    
				    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

				    draw3DObject(Scorewall[current].object);
				    //glPopMatrix (); 
				}
				for(map<string,Game>::iterator it=Brickwall.begin();it!=Brickwall.end();it++){
				    string current = it->first; 
				    glm::mat4 MVP;  // MVP = Projection * View * Model

				    Matrices.model = glm::mat4(1.0f);

				    /* Render your scene */
				    glm::mat4 ObjectTransform;
				    glm::mat4 translateObject = glm::translate (glm::vec3(Brickwall[current].x, Brickwall[current].y, 0.0f)); // glTranslatef
				    glm::mat4 rotateTriangle = glm::rotate((float)((Brickwall[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				    ObjectTransform=translateObject*rotateTriangle;
				    Matrices.model *= ObjectTransform;
				    MVP = VP * Matrices.model; // MVP = p * V * M

				    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				    draw3DObject(Brickwall[current].object);
				}
				for(map<string,Game>::iterator it=lasercircle.begin();it!=lasercircle.end();it++){
				    string current = it->first; 
				    glm::mat4 MVP;  // MVP = Projection * View * Model

				    Matrices.model = glm::mat4(1.0f);

				    /* Render your scene */
				    glm::mat4 ObjectTransform;
				    glm::mat4 translateObject = glm::translate (glm::vec3(lasercircle[current].x, lasercircle[current].y, 0.0f)); // glTranslatef
				    glm::mat4 rotateTriangle = glm::rotate((float)((lasercircle[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				    ObjectTransform=translateObject*rotateTriangle;
				    Matrices.model *= ObjectTransform;
				    MVP = VP * Matrices.model; // MVP = p * V * M

				    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				    draw3DObject(lasercircle[current].object);
				}
				int k;
				for(k=1;k<=4;k++)
				{
				    float translation;
				    float translation1=250;
				    if(k==1)
				    {
				        statusforscore('L');
				        translation=365;
				    }
				    else if(k==2)
				    {
				        statusforscore('I');
				        translation=390;
				    }
				    else if(k==3)
				    {
				        statusforscore('F');
				        translation=415;
				    }
				    else if(k==4)
				    {
				        statusforscore('E');
				        translation=440;
				    }

				    for(map<string,Game>::iterator it=Score.begin();it!=Score.end();it++){
				        string current = it->first; 
				        if(Score[current].status==1)
				        {
				            glm::mat4 MVP;  // MVP = Projection * View * Model

				            Matrices.model = glm::mat4(1.0f);

				            /* Render your scene */
				            glm::mat4 ObjectTransform;
				            glm::mat4 translateObject = glm::translate (glm::vec3(Score[current].x+translation,Score[current].y+translation1,0.0f)); // glTranslatef
				            glm::mat4 rotateTriangle = glm::rotate((float)((Score[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				            ObjectTransform=translateObject*rotateTriangle;
				            Matrices.model *= ObjectTransform;
				            MVP = VP * Matrices.model; // MVP = p * V * M

				            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				            draw3DObject(Score[current].object);
				        }
				    }
				}
				int num=0;
				for(num=1;num<=life;num++)
				{
					float translation,translation1=200;
					if(num==1)
						translation=365;
					if(num==2)
						translation=395;
					if(num==3)
						translation=425;
					if(num==4)
						translation=455;
					if(num==5)
						translation=485;
					for(map<string,Game>::iterator it=heart.begin();it!=heart.end();it++){
					        string current = it->first;
					            glm::mat4 MVP;  // MVP = Projection * View * Model

					            Matrices.model = glm::mat4(1.0f);

					            /* Render your scene */
					            glm::mat4 ObjectTransform;
					            glm::mat4 translateObject = glm::translate (glm::vec3(heart[current].x+translation,heart[current].y+translation1,0.0f)); // glTranslatef
					            glm::mat4 rotateTriangle = glm::rotate((float)((heart[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
					            ObjectTransform=translateObject*rotateTriangle;
					            Matrices.model *= ObjectTransform;
					            MVP = VP * Matrices.model; // MVP = p * V * M

					            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
					            draw3DObject(heart[current].object);
					    }
				}
				for(k=1;k<=6;k++)
				{
				    float translation;
				    float translation1=50;
				    if(k==1)
				    {
				        statusforscore('P');
				        translation=365;
				    }
				    else if(k==2)
				    {
				        statusforscore('O');
				        translation=390;
				    }
				    else if(k==3)
				    {
				        statusforscore('I');
				        translation=415;
				    }
				    else if(k==4)
				    {
				        statusforscore('N');
				        translation=440;
				    }
				    else if(k==5)
				    {
				        statusforscore('T');
				        translation=465;
				    }
				    else if(k==6)
				    {
				        statusforscore('S');
				        translation=490;
				    }
				    for(map<string,Game>::iterator it=Score.begin();it!=Score.end();it++){
				        string current = it->first; 
				        if(Score[current].status==1)
				        {
				            glm::mat4 MVP;  // MVP = Projection * View * Model

				            Matrices.model = glm::mat4(1.0f);

				            /* Render your scene */
				            glm::mat4 ObjectTransform;
				            glm::mat4 translateObject = glm::translate (glm::vec3(Score[current].x+translation,Score[current].y+translation1,0.0f)); // glTranslatef
				            glm::mat4 rotateTriangle = glm::rotate((float)((Score[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				            ObjectTransform=translateObject*rotateTriangle;
				            Matrices.model *= ObjectTransform;
				            MVP = VP * Matrices.model; // MVP = p * V * M

				            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				            draw3DObject(Score[current].object);
				        }
				    }
				}
				int total=points;
				int ones,tens,hundreds;
				ones=total%10;
				total=total/10;
				tens=total%10;
				total=total/10;
				hundreds=total%10;
				char val1,val2,val3;
				val1=ones+'0';
				val2=tens+'0';
				val3=hundreds+'0';
				for(k=1;k<=3;k++)
				{
				    float translation;
				    if(k==1)
				    {
				        statusforscore(val1);
				        translation=445;
				    }
				    else if(k==2)
				    {
				        statusforscore(val2);
				        translation=415;
				    }
				    else if(k==3)
				    {
				        statusforscore(val3);
				        translation=385;
				    }
				    for(map<string,Game>::iterator it=Score.begin();it!=Score.end();it++){
				        string current = it->first; 
				        if(Score[current].status==1)
				        {
				            glm::mat4 MVP;  // MVP = Projection * View * Model

				            Matrices.model = glm::mat4(1.0f);

				            /* Render your scene */
				            glm::mat4 ObjectTransform;
				            glm::mat4 translateObject = glm::translate (glm::vec3(Score[current].x+translation,Score[current].y,0.0f)); // glTranslatef
				            glm::mat4 rotateTriangle = glm::rotate((float)((Score[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				            ObjectTransform=translateObject*rotateTriangle;
				            Matrices.model *= ObjectTransform;
				            MVP = VP * Matrices.model; // MVP = p * V * M

				            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				            draw3DObject(Score[current].object);
				        }
				    }
				}
				for(k=1;k<=5;k++)
				{
				    float translation;
				    float translation1=-200;
				    if(k==1)
				    {
				        statusforscore('S');
				        translation=365;
				    }
				    else if(k==2)
				    {
				        statusforscore('T');
				        translation=395;
				    }
				    else if(k==3)
				    {
				        statusforscore('A');
				        translation=425;
				    }
				    else if(k==4)
				    {
				        statusforscore('G');
				        translation=455;
				    }
				    else if(k==5)
				    {
				    	statusforscore('E');
				    	translation=485;
				    }

				    for(map<string,Game>::iterator it=Score.begin();it!=Score.end();it++){
				        string current = it->first; 
				        if(Score[current].status==1)
				        {
				            glm::mat4 MVP;  // MVP = Projection * View * Model

				            Matrices.model = glm::mat4(1.0f);

				            /* Render your scene */
				            glm::mat4 ObjectTransform;
				            glm::mat4 translateObject = glm::translate (glm::vec3(Score[current].x+translation,Score[current].y+translation1,0.0f)); // glTranslatef
				            glm::mat4 rotateTriangle = glm::rotate((float)((Score[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
				            ObjectTransform=translateObject*rotateTriangle;
				            Matrices.model *= ObjectTransform;
				            MVP = VP * Matrices.model; // MVP = p * V * M

				            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				            draw3DObject(Score[current].object);
				        }
				    }
				}
				for(k=1;k<=stagelevel;k++)
				{
					float translation1=-250,translation;
					COLOR skyblue4 = {30/255.0,144/255.0,255/255};
				    COLOR skyblue3 = {135/255.0,206/255.0,250/255};
				    COLOR skyblue2 = {0/255.0,0/255.0,125/255.0};
				    COLOR skyblue1 = {123/255.0,201/255.0,227/255.0};
				    COLOR skyblue = {132/255.0,217/255.0,245/255.0};
					if(stagelevel==1 || stagelevel==2 || stagelevel==3 || stagelevel==4)
					{
						translation=365;
						createRectangle("stage1",0,skyblue,skyblue,365,0,20,40,"stage",0);
						//stage["stage1"].color=skyblue;
					}
					if(stagelevel==2 || stagelevel==3 || stagelevel==4)
					{
						translation=375;
						createRectangle("stage2",0,skyblue4,skyblue4,405,0,20,40,"stage",0);
						//stage["stage1"].color=skyblue1;
					}
					if(stagelevel==3 || stagelevel==4)
					{
						translation=385;
						createRectangle("stage3",0,skyblue2,skyblue2,445,0,20,40,"stage",0);
						//stage["stage1"].color=skyblue3;
					}
					if(stagelevel==4)
					{
						translation=395;
						createRectangle("stage4",0,skyblue3,skyblue3,485,0,20,40,"stage",0);
						//stage["stage1"].color=skyblue4;
					}
					for(map<string,Game>::iterator it=stage.begin();it!=stage.end();it++){
				        string current = it->first;
			            glm::mat4 MVP;  // MVP = Projection * View * Model

			            Matrices.model = glm::mat4(1.0f);

			            /* Render your scene */
			            glm::mat4 ObjectTransform;
			            glm::mat4 translateObject = glm::translate (glm::vec3(stage[current].x,stage[current].y+translation1,0.0f)); // glTranslatef
			            glm::mat4 rotateTriangle = glm::rotate((float)((stage[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
			            ObjectTransform=translateObject*rotateTriangle;
			            Matrices.model *= ObjectTransform;
			            MVP = VP * Matrices.model; // MVP = p * V * M

			            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			            draw3DObject(stage[current].object);
				    }
				}
                //glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);
		}
        //glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);
		if(points>100 && start==1)
		{
			glm::mat4 VP = Matrices.projection * Matrices.view;
        	COLOR white={0,0,0};
        	glm::mat4 MVP;  // MVP = Projection * View * Model
		    Matrices.model = glm::mat4(1.0f);
		    glm::mat4 ObjectTransform;
		    glm::mat4 translateObject = glm::translate (glm::vec3(backgroundObjects["backgroundobj1"].x, backgroundObjects["backgroundobj1"].y, 0.0f)); 
		    glm::mat4 rotateTriangle = glm::rotate((float)((backgroundObjects["backgroundobj1"].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
		    ObjectTransform=translateObject*rotateTriangle;
		    Matrices.model *= ObjectTransform;
		    MVP = VP * Matrices.model; // MVP = p * V * M		    
		    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        	draw3DObject(backgroundObjects["backgroundobj1"].object);
            COLOR skyblue3 = {135/255.0,206/255.0,250/255};
            COLOR gold = {218.0/255.0,165.0/255.0,32.0/255.0};
            COLOR black = {255/255.0,255/255.0,255/255.0};
            createCircle("prestart2",0,skyblue3,0,0,450,450,200,"prestart",1);
            createCircle("prestart1",0,skyblue3,0,0,600,600,200,"prestart",1);
            createCircle("prestart5",0,gold,-400,0,50,50,200,"prestart",1);
            createCircle("prestart6",0,black,-425,20,10,10,200,"prestart",1);
            createCircle("prestart7",0,black,-375,20,10,10,200,"prestart",1);
            createCircle("prestart8",0,black,-400,-20,10,10,200,"prestart",1);
            createCircle("prestart9",0,gold,-400,-10,10,10,200,"prestart",1);
            createCircle("prestart51",0,gold,400,0,50,50,200,"prestart",1);
            createCircle("prestart61",0,black,425,20,10,10,200,"prestart",1);
            createCircle("prestart71",0,black,375,20,10,10,200,"prestart",1);
            createCircle("prestart81",0,black,400,-20,10,10,200,"prestart",1);
            createCircle("prestart91",0,gold,400,-10,10,10,200,"prestart",1);
            for(map<string,Game>::iterator it=prestart.begin();it!=prestart.end();it++){
                string current = it->first; 
            
                glm::mat4 MVP;  // MVP = Projection * View * Model

                Matrices.model = glm::mat4(1.0f);

                /* Render your scene */
                glm::mat4 ObjectTransform;
                glm::mat4 translateObject = glm::translate (glm::vec3(prestart[current].x,prestart[current].y,0.0f)); // glTranslatef
                glm::mat4 rotateTriangle = glm::rotate((float)((prestart[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
                ObjectTransform=translateObject*rotateTriangle;
                Matrices.model *= ObjectTransform;
                MVP = VP * Matrices.model; // MVP = p * V * M

                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(prestart[current].object);
        }
        	int k;
        	for(k=1;k<=6;k++)
    		{
			    float translation;
			    float translation1=0;
			    if(k==1)
			    {
			        statusforscore('Y');
			        translation=-100;
			    }
			    else if(k==2)
			    {
			        statusforscore('O');
			        translation=-70;
			    }
			    else if(k==3)
			    {
			        statusforscore('U');
			        translation=-40;
			    }
			    else if(k==4)
			    {
			        statusforscore('W');
			        translation=15;
			    }
			    else if(k==5)
			    {
			        statusforscore('I');
			        translation=45;
			    }
			    else if(k==6)
			    {
			        statusforscore('N');
			        translation=75;
			    }
			    for(map<string,Game>::iterator it=Score.begin();it!=Score.end();it++){
			        string current = it->first; 
			        if(Score[current].status==1)
			        {
			            glm::mat4 MVP;  // MVP = Projection * View * Model

			            Matrices.model = glm::mat4(1.0f);

			            /* Render your scene */
			            glm::mat4 ObjectTransform;
			            glm::mat4 translateObject = glm::translate (glm::vec3(Score[current].x+translation,Score[current].y+translation1,0.0f)); // glTranslatef
			            glm::mat4 rotateTriangle = glm::rotate((float)((Score[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
			            ObjectTransform=translateObject*rotateTriangle;
			            Matrices.model *= ObjectTransform;
			            MVP = VP * Matrices.model; // MVP = p * V * M

			            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			            draw3DObject(Score[current].object);
			        }
			    }
    		}	
		}
		if(life<0 && start==1)
        {
  			glm::mat4 VP = Matrices.projection * Matrices.view;
        	COLOR white={0,0,0};
        	glm::mat4 MVP;  // MVP = Projection * View * Model
		    Matrices.model = glm::mat4(1.0f);
		    glm::mat4 ObjectTransform;
		    glm::mat4 translateObject = glm::translate (glm::vec3(backgroundObjects["backgroundobj1"].x, backgroundObjects["backgroundobj1"].y, 0.0f)); 
		    glm::mat4 rotateTriangle = glm::rotate((float)((backgroundObjects["backgroundobj1"].angle)*M_PI/180.0f), glm::vec3(0,0,1));// glTranslatef
		    ObjectTransform=translateObject*rotateTriangle;
		    Matrices.model *= ObjectTransform;
		    MVP = VP * Matrices.model; // MVP = p * V * M		    
		    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        	draw3DObject(backgroundObjects["backgroundobj1"].object);
            COLOR otherred = {178/255.0,34/255.0,34/255.0};
            COLOR gold = {218.0/255.0,165.0/255.0,32.0/255.0};
            COLOR black = {255/255.0,255/255.0,255/255.0};
            //COLOR white = {255/255.0,255/255.0,255/255.0};
            createCircle("prestart2",0,otherred,0,0,450,450,200,"prestart",1);
            createCircle("prestart1",0,otherred,0,0,600,600,200,"prestart",1);
            createCircle("prestart5",0,gold,-400,0,50,50,200,"prestart",1);
            createCircle("prestart6",0,black,-425,20,10,10,200,"prestart",1);
            createCircle("prestart7",0,black,-375,20,10,10,200,"prestart",1);
            createCircle("prestart8",0,black,-400,-20,10,10,200,"prestart",1);
            createCircle("prestart9",0,gold,-400,-30,10,10,200,"prestart",1);
            createCircle("prestart51",0,gold,400,0,50,50,200,"prestart",1);
            createCircle("prestart61",0,black,425,20,10,10,200,"prestart",1);
            createCircle("prestart71",0,black,375,20,10,10,200,"prestart",1);
            createCircle("prestart81",0,black,400,-20,10,10,200,"prestart",1);
            createCircle("prestart91",0,gold,400,-30,10,10,200,"prestart",1);
            for(map<string,Game>::iterator it=prestart.begin();it!=prestart.end();it++){
                string current = it->first; 
            
                glm::mat4 MVP;  // MVP = Projection * View * Model

                Matrices.model = glm::mat4(1.0f);

                /* Render your scene */
                glm::mat4 ObjectTransform;
                glm::mat4 translateObject = glm::translate (glm::vec3(prestart[current].x,prestart[current].y,0.0f)); // glTranslatef
                glm::mat4 rotateTriangle = glm::rotate((float)((prestart[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
                ObjectTransform=translateObject*rotateTriangle;
                Matrices.model *= ObjectTransform;
                MVP = VP * Matrices.model; // MVP = p * V * M

                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(prestart[current].object);
            }
        	int k;
        	for(k=1;k<=7;k++)
    		{
			    float translation;
			    float translation1=0;
			    if(k==1)
			    {
			        statusforscore('Y');
			        translation=-100;
			    }
			    else if(k==2)
			    {
			        statusforscore('O');
			        translation=-70;
			    }
			    else if(k==3)
			    {
			        statusforscore('U');
			        translation=-40;
			    }
			    else if(k==4)
			    {
			        statusforscore('L');
			        translation=15;
			    }
			    else if(k==5)
			    {
			        statusforscore('O');
			        translation=45;
			    }
			    else if(k==6)
			    {
			        statusforscore('S');
			        translation=75;
			    }
			    else if(k==7)
			    {
			    	statusforscore('E');
			    	translation=105;
			    }
			    for(map<string,Game>::iterator it=Score.begin();it!=Score.end();it++){
			        string current = it->first; 
			        if(Score[current].status==1)
			        {
			            glm::mat4 MVP;  // MVP = Projection * View * Model

			            Matrices.model = glm::mat4(1.0f);

			          
			            glm::mat4 ObjectTransform;
			            glm::mat4 translateObject = glm::translate (glm::vec3(Score[current].x+translation,Score[current].y+translation1,0.0f)); // glTranslatef
			            glm::mat4 rotateTriangle = glm::rotate((float)((Score[current].angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
			            ObjectTransform=translateObject*rotateTriangle;
			            Matrices.model *= ObjectTransform;
			            MVP = VP * Matrices.model; // MVP = p * V * M

			            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			            draw3DObject(Score[current].object);
			        }
			    }
    		}
    	}

}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
       is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, mousescroll); // mouse scroll

    return window;
}
/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
GLuint textureProgramID;
void initGL (GLFWwindow* window, int width, int height)
{


    COLOR grey = {168.0/255.0,168.0/255.0,168.0/255.0};
    COLOR gold = {218.0/255.0,165.0/255.0,32.0/255.0};
    COLOR red = {255.0/255.0,51.0/255.0,51.0/255.0};
    COLOR coingold = {255.0/255.0,223.0/255.0,0.0/255.0};
    COLOR lightgreen = {57/255.0,230/255.0,0/255.0};
    COLOR darkgreen = {51/255.0,102/255.0,0/255.0};
    COLOR black = {30/255.0,30/255.0,21/255.0};
    COLOR blue = {0,0,1};
    COLOR ball = {255/255.0,228/225.0,181/255.0};
    COLOR darkbrown = {46/255.0,46/255.0,31/255.0};
    COLOR lightbrown = {95/255.0,63/255.0,32/255.0};
    COLOR brown1 = {117/255.0,78/255.0,40/255.0};
    COLOR brown2 = {134/255.0,89/255.0,40/255.0};
    COLOR brown3 = {46/255.0,46/255.0,31/255.0};
    COLOR cratebrown = {153/255.0,102/255.0,0/255.0};
    COLOR cratebrown1 = {121/255.0,85/255.0,0/255.0};
    COLOR cratebrown2 = {102/255.0,68/255.0,0/255.0};
    COLOR skyblue7 = {95/255.0,158/255.0,160/255.0};
    COLOR skyblue6 = {175/255.0,238/255.0,238/255.0};
    COLOR skyblue5 = {70/255.0,130/255.0,180/255.0};
    COLOR skyblue4 = {30/255.0,144/255.0,255/255};
    COLOR skyblue3 = {135/255.0,206/255.0,250/255};
    COLOR skyblue2 = {0/255.0,0/255.0,125/255.0};
    COLOR skyblue1 = {123/255.0,201/255.0,227/255.0};
    COLOR skyblue = {132/255.0,217/255.0,245/255.0};
    COLOR cloudwhite = {229/255.0,255/255.0,255/255.0};
    COLOR cloudwhite1 = {204/255.0,255/255.0,255/255.0};
    COLOR lightpink = {255/255.0,122/255.0,173/255.0};
    COLOR darkpink = {255/255.0,51/255.0,119/255.0};
    COLOR white = {255/255.0,255/255.0,255/255.0};
    COLOR score = {117/255.0,78/255.0,40/255.0};
    COLOR lightred = {117/255.0,78/255.0,71/255.0};
    COLOR otherred = {178/255.0,34/255.0,34/255.0};

    int k;
    createRectangle("backgroundobj1",0,white,white,0,0,600,1030,"background",0);
    createRectangle("backgroundobj2",0,white,skyblue6,15,0,600,1000,"background",0);
    createRectangle("backgroundobj3",0,gold,gold,0,-200,100,1030,"background",0);
    createRectangle("backgroundobjtree1",0,brown1,brown1,315,-100,100,20,"background",0);
    createCircle("backgroundobjtree2",0,darkgreen,315,0,40,100,200,"background",1);
    createRectangle("backgroundobjtree3",0,gold,gold,300,0,15,15,"background",0);
    createCircle("backgroundobj4",0,skyblue7,-300,200,100,15,200,"background",0);
    createCircle("backgroundobj5",0,skyblue7,-300,210,80,15,200,"background",0);
    createCircle("backgroundobj8",0,skyblue7,-300,220,30,10,200,"background",0);
    createCircle("backgroundobj9",0,skyblue7,-300,190,80,15,200,"background",0);
    createCircle("backgroundobj93",0,skyblue7,-300,180,30,10,200,"background",0);
    createCircle("backgroundobj41",0,skyblue7,150,200,100,15,200,"background",0);
    createCircle("backgroundobj51",0,skyblue7,150,210,80,15,200,"background",0);
    createCircle("backgroundobj81",0,skyblue7,150,220,30,10,200,"background",0);
    createCircle("backgroundobj91",0,skyblue7,150,190,80,15,200,"background",0);
    createCircle("backgroundobj931",0,skyblue7,150,180,30,10,200,"background",0);
    createCircle("backgroundobj31",0,otherred,-440,-160,30,30,200,"background",1);
    createCircle("backgroundobjcrow1",0,black,150,200,10,10,200,"background",0);
    createCircle("backgroundobjcrow2",0,skyblue7,147,197,10,10,200,"background",0);
    createCircle("backgroundobjcrow3",0,black,165,200,10,10,200,"background",0);
    createCircle("backgroundobjcrow4",0,skyblue7,168,197,10,10,200,"background",0);
    createCircle("backgroundobjcrow5",0,black,160,188,10,10,200,"background",0);
    createCircle("backgroundobjcrow6",0,skyblue7,157,185,10,10,200,"background",0);
    createCircle("backgroundobjcrow7",0,black,175,188,10,10,200,"background",0);
    createCircle("backgroundobjcrow8",0,skyblue7,178,185,10,10,200,"background",0);
    /*createCircle("backgroundobj33",0,white,-460,-160,6,6,200,"background",1);
    createCircle("backgroundobj34",0,white,-440,-140,6,6,200,"background",1);
    createCircle("backgroundobj35",0,white,-440,-180,6,6,200,"background",1);
    createCircle("backgroundobj36",0,white,-440,-160,6,6,200,"background",1);*/
    createCircle("backgorundcircle1",0,skyblue,0,0,100,50,200,"circlebackground",1);
    createCircle("backgorundcircle2",0,skyblue1,0,0,100,50,200,"circlebackground1",1);
    createCircle("backgorundcircle3",0,skyblue4,0,0,100,50,200,"circlebackground2",1);
    createCircle("backgorundcircle4",0,skyblue5,0,0,100,50,200,"circlebackground3",1);
    createRectangle("reddrum",0,red,red,-185,-260,80,150,"drum",0);
    createRectangle("greendrum",0,lightgreen,lightgreen,15,-260,80,150,"drum",0);
    createRectangle("laserbroad",0,skyblue2,skyblue2,-485,0,40,40,"laser",0);
    createRectangle("blueline",0,blue,blue,-85,-200,2,800,"background",0);
    createRectangle("lasernarrow",0,skyblue2,skyblue2,-485,0,18,110,"laser",0);
    createCircle("lasercircle3",0,skyblue,-467,0,5,10,200,"laser1",1);
    createCircle("lasercircle4",0,skyblue2,-465,0,5,10,200,"laser1",1);
    createCircle("lasercircle1",0,skyblue,-430,0,5,10,200,"laser1",1);
    createCircle("lasercircle2",0,skyblue2,-430,0,3,8,200,"laser1",1);

    createRectangle("mirror1",60,skyblue,skyblue,-85,0,3,60,"backgroundmirror",0);
    createRectangle("mirror2",150,skyblue,skyblue,-80,280,3,60,"backgroundmirror",0);
    createRectangle("mirror3",120,skyblue,skyblue,265,200,3,60,"backgroundmirror",0);
    createRectangle("mirror4",45,skyblue,skyblue,265,-120,3,60,"backgroundmirror",0);
    createRectangle("mirror1b",60,skyblue1,skyblue1,-85,-1.5,3,60,"backgroundmirror",0);
    createRectangle("mirror2b",150,skyblue1,skyblue1,-80,281.5,3,60,"backgroundmirror",0);
    createRectangle("mirror3b",120,skyblue1,skyblue1,265,201.5,3,60,"backgroundmirror",0);
    createRectangle("mirror4b",45,skyblue1,skyblue1,265,-121.5,3,60,"backgroundmirror",0);
    createRectangle("Scoreside",0,cratebrown2,cratebrown2,415,0,600,200,"scoreside",0);
    createRectangle("mirrorshade1",0,skyblue1,skyblue1,-15+backgroundObjectsmirror["mirror1"].x+25*cos(M_PI/3)+25,25*sin(M_PI/3),2,20,"mirrorback",0);
    createRectangle("mirrorshade2",0,skyblue1,skyblue1,-15+backgroundObjectsmirror["mirror1"].x+15*cos(M_PI/3)+25,15*sin(M_PI/3),2,20,"mirrorback",0);
    createRectangle("mirrorshade3",0,skyblue1,skyblue1,-15+backgroundObjectsmirror["mirror1"].x-25*cos(M_PI/3)+25,-25*sin(M_PI/3),2,20,"mirrorback",0);
    createRectangle("mirrorshade4",0,skyblue1,skyblue1,-15+backgroundObjectsmirror["mirror1"].x+5*cos(M_PI/3)+25,5*sin(M_PI/3),2,20,"mirrorback",0);
    createRectangle("mirrorshade5",0,skyblue1,skyblue1,-15+backgroundObjectsmirror["mirror1"].x-15*cos(M_PI/3)+25,-15*sin(M_PI/3),2,20,"mirrorback",0);
    createRectangle("mirrorshade6",0,skyblue1,skyblue1,-15+backgroundObjectsmirror["mirror1"].x-5*cos(M_PI/3)+25,-5*sin(M_PI/3),2,20,"mirrorback",0);
    createRectangle("mirrorshade7",0,skyblue1,skyblue1,-95+25*cos(5*M_PI/6)+25,280+25*sin(5*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade8",0,skyblue1,skyblue1,-95+15*cos(5*M_PI/6)+25,280+15*sin(5*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade9",0,skyblue1,skyblue1,-95-25*cos(5*M_PI/6)+25,280+-25*sin(5*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade10",0,skyblue1,skyblue1,-95+5*cos(5*M_PI/6)+25,280+5*sin(5*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade11",0,skyblue1,skyblue1,-95-15*cos(5*M_PI/6)+25,280-15*sin(5*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade12",0,skyblue1,skyblue1,-95-5*cos(5*M_PI/6)+25,280-5*sin(5*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade13",0,skyblue1,skyblue1,250+25*cos(4*M_PI/6)+25,200+25*sin(4*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade14",0,skyblue1,skyblue1,250+15*cos(4*M_PI/6)+25,200+15*sin(4*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade15",0,skyblue1,skyblue1,250-25*cos(4*M_PI/6)+25,200+-25*sin(4*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade16",0,skyblue1,skyblue1,250+5*cos(4*M_PI/6)+25,200+5*sin(4*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade17",0,skyblue1,skyblue1,250-15*cos(4*M_PI/6)+25,200-15*sin(4*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade18",0,skyblue1,skyblue1,250-5*cos(4*M_PI/6)+25,200-5*sin(4*M_PI/6),2,20,"mirrorback",0);
    createRectangle("mirrorshade19",0,skyblue1,skyblue1,250+25*cos(M_PI/4)+25,-120+25*sin(M_PI/4),2,20,"mirrorback",0);
    createRectangle("mirrorshade20",0,skyblue1,skyblue1,250+15*cos(M_PI/4)+25,-120+15*sin(M_PI/4),2,20,"mirrorback",0);
    createRectangle("mirrorshade21",0,skyblue1,skyblue1,250-25*cos(M_PI/4)+25,-120+-25*sin(M_PI/4),2,20,"mirrorback",0);
    createRectangle("mirrorshade22",0,skyblue1,skyblue1,250+5*cos(M_PI/4)+25,-120+5*sin(M_PI/4),2,20,"mirrorback",0);
    createRectangle("mirrorshade23",0,skyblue1,skyblue1,250-15*cos(M_PI/4)+25,-120-15*sin(M_PI/4),2,20,"mirrorback",0);
    createRectangle("mirrorshade24",0,skyblue1,skyblue1,250-5*cos(M_PI/4)+25,-120-5*sin(M_PI/4),2,20,"mirrorback",0);
    createCircle("lasercircle",0,skyblue2,-495,0,30,30,10,"laser",0);
    createCircle("drumcap1",0,blue,-185,-220,75,10,200,"drumcap",1);
    createCircle("drumcap2",0,red,-185,-220,73,8,200,"drumcap",1);
    createCircle("drumcap3",0,blue,15,-220,75,10,200,"drumcap",1);
    createCircle("drumcap4",0,lightgreen,15,-220,73,8,200,"drumcap",1);
    createCircle("prestart1",0,skyblue4,0,0,600,600,200,"prestart",1);
    createCircle("prestart2",0,skyblue3,0,0,450,450,200,"prestart",1);
    createCircle("prestart3",0,skyblue1,0,0,300,300,200,"prestart",1);
    createCircle("prestart4",0,skyblue,0,0,150,150,200,"prestart",1);

    float x[3],y[3];
    x[0]=0;
    x[1]=10;
    x[2]=-10;
    y[0]=0;
    y[1]=10;
    y[2]=10;
    createCircle("heartcircle1",0,red,5,10,4,4,200,"heart",1);
    //heart["heartcircle1"].status=1;
    createCircle("heartcircle2",0,red,-5,10,4,4,200,"heart",1);
    //heart["heartcircle2"].status=1;
    createTriangle ("hearttriangle",0,red,x,y,"heart",1);
    //createRectangle("stage1",0,skyblue1,skyblue1,0,0,20,10,"stage",0);
    int ycoord=-285;
    for(k=1;k<=20;k++)
    {
        char t=k+'0';
        string u="Wall";
        string v=u+t;
        COLOR wallcolor;
        if(k%2==0)
        {
            wallcolor.r=cratebrown.r;
            wallcolor.g=cratebrown.g;
            wallcolor.b=cratebrown.b;
        }
        else
        {
            wallcolor.r=cratebrown1.r;
            wallcolor.g=cratebrown1.g;
            wallcolor.b=cratebrown1.b;            
        }
        createRectangle(v,0,wallcolor,wallcolor,330,ycoord,30,30,"wall",0);
        ycoord+=30;
    }
    ycoord=-285;
    for(k=21;k<=40;k++)
    {
        char t=k+'0';
        string u="Wall";
        string v=u+t;
        COLOR wallcolor;
        if(k%2==0)
        {
            wallcolor.r=cratebrown.r;
            wallcolor.g=cratebrown.g;
            wallcolor.b=cratebrown.b;
        }
        else
        {
            wallcolor.r=cratebrown1.r;
            wallcolor.g=cratebrown1.g;
            wallcolor.b=cratebrown1.b;            
        }
        createRectangle(v,0,wallcolor,wallcolor,-500,ycoord,30,30,"wall",0);
        ycoord+=30;
    }
    COLOR color;
    color.r=gold.r;
    color.b=gold.b;
    color.g=gold.g;
    float width1=20;
    float height1=3;
    createRectangle("top",0,color,color,0,20,height1,width1,"score",0);
    createRectangle("bottom",0,color,color,0,-20,height1,width1,"score",0);
    createRectangle("middle",0,color,color,0,0,height1,width1,"score",0);
    createRectangle("left1",0,color,color,-10,10,width1,height1,"score",0);
    createRectangle("left2",0,color,color,-10,-10,width1,height1,"score",0);
    createRectangle("right1",0,color,color,10,10,width1,height1,"score",0);
    createRectangle("right2",0,color,color,10,-10,width1,height1,"score",0);
    createRectangle("middle1",0,color,color,0,10,width1,height1,"score",0);
    createRectangle("middle2",0,color,color,0,-10,width1,height1,"score",0);
    createRectangle("diagonal1",(atan(0.5)*180/M_PI),color,color,-5,10,width1/2+13,height1,"score",0);
    createRectangle("diagonal2",-(atan(0.5)*180/M_PI),color,color,5,10,width1/2+13,height1,"score",0);
    createRectangle("diagonal3",-(atan(0.5)*180/M_PI),color,color,-5,-10,width1/2+13,height1,"score",0);
    createRectangle("diagonal4",(atan(0.5)*180/M_PI),color,color,5,-10,width1/2+13,height1,"score",0);

   /*createCircle("lasercircle2",0,skyblue,-345,0,10,10,10,"laser",0);
    createCircle("lasercircle1",0,skyblue2,-345,0,10,10,100,"laser1",1);*/
    // Create and compile our GLSL program from the shaders
    glActiveTexture(GL_TEXTURE0);
    // load an image file directly as a new OpenGL texture
    // GLuint texID = SOIL_load_OGL_texture ("Images/beach.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS); // Buggy for OpenGL3
    // check for an error during the load process
    //if(textureID == 0 )
    //  cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;

    // Create and compile our GLSL program from the texture shaders
    textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");
    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


    reshapeWindow (window, width, height);

    // Background color of the scene
    glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
    glClearDepth (1.0f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}
int main (int argc, char** argv)
{
    camera_disable_rotation=1;
    camera_follow=0;
    camera_fps=0;
    camera_radius=1; //Top view
    camera_top=1;
    eye_x = 500+camera_radius*cos(angle*M_PI/180);
    eye_z = 500+camera_radius*sin(angle*M_PI/180);
    eye_y=1100;
    int width = 1030;
    int height = 600;
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = 3000;
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
    mpg123_open(mh, "e.mp3");
    mpg123_getformat(mh, &rate, &channels, &encoding);

    /* set the output format and open the output device */
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);
    /* open the file and get the decoding format */
    
    GLFWwindow* window = initGLFW(width, height);

    initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time,current_time1,last_cannon_time = glfwGetTime();

    glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);

    game_start_timer=glfwGetTime();
    old_time = glfwGetTime();
                 
    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {
            if(mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
            {
                ao_play(dev, (char *)buffer, done);
            } 
            else mpg123_seek(mh, 0, SEEK_SET);
        cur_time = glfwGetTime(); // Time in seconds
        // OpenGL Draw commands
        draw(window);
        old_time=cur_time;

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); 
        current_time1 = glfwGetTime();// Time in seconds
        if ((current_time - last_update_time) >= 1/speed) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            fl=1;
            last_update_time = current_time;
        }
        if((current_time1 - last_cannon_time >= stagelevel)) {
            fl1=1;
            last_cannon_time = current_time1;
        }
        if(ckeypress==1)
        {
        		ckeypress=0;
		 		initGL (window, width, height);
		 		life=5;
		 		points=0;
		 		speed=0.5;
                stagelevel=1;
		 		for(map<string,Game>::iterator it=object.begin();it!=object.end();){
	    		string current = it->first; 
	        	object.erase(it++);
	        	}
	    		for(map<string,Game>::iterator it=laserbeam.begin();it!=laserbeam.end();){
	    		string current = it->first; 
	        	laserbeam.erase(it++);
	    		}
                for(map<string,Game>::iterator it=stage.begin();it!=stage.end();){
                string current = it->first; 
                stage.erase(it++);
                }
        }
        if(points>25*stagelevel)
        {
        	stagelevel++;
        }
    }
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
