//========================================================================
// This is a small test application for GLFW.
// The program shows texture loading with mipmap generation and trilienar
// filtering.
// Note: For OpenGL 1.0 compability, we do not use texture objects (this
// is no issue, since we only have one texture).
//========================================================================


#include <stdlib.h>    // For malloc() etc.
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <glfw3.h>
#include <GL/glu.h>
#include <iostream>
#include <math.h>
#include <tinythread.h>
#include "FreeImage.h"
#include "tiles.h"
#include "sequence.h"

  using namespace std;
  using namespace tthread;
//  using namespace cimg_library;


/// Max function
template <class T> T MAX(T a, T b) {
	return (a > b) ? a: b;
}

/// Min function
template <class T> T MIN(T a, T b) {
	return (a < b) ? a: b;
}

float lum;
float gam = 1;
bool run = true;
unsigned int curTime=0;
int sens = 1;
bool pingpong = false;
bool pause = false;
bool repeat = true;
unsigned int FirstFrame;
unsigned int LastFrame;
unsigned int StepFrame;
float displayScale = 1;
Sequence* sequence;
BYTE LUT[256];		// Lookup table
int mouseX, mouseY, mouseXInit = -1;

bool FileExists( const char* FileName )
{
    FILE* fp = NULL;

    fp = fopen( FileName, "rb" );
    if( fp != NULL )
    {
        fclose( fp );
        return true;
    }
    else
        return false;
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

//void GLFWCALL My_Key_Callback(int key, int action)
static void My_Key_Callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        //exit
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, GL_TRUE);
        //pause
        if (key == GLFW_KEY_SPACE)
            pause = !pause;
        //frame by frame and playing
        if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)
        {
            repeat = (key == GLFW_KEY_DOWN);
            curTime = 0;
            pause = false;
        }
        if (key == GLFW_KEY_RIGHT)
        {
            if(curTime<LastFrame-FirstFrame) curTime++;
            pause = true;
        }

        if (key == GLFW_KEY_LEFT)
        {
            if(curTime>0) curTime--;
            pause = true;
        }

        // scale
        if (key == GLFW_KEY_KP_ADD)
            displayScale *= 2;

        if (key == GLFW_KEY_KP_SUBTRACT)
            displayScale /= 2;

        if (key == GLFW_KEY_KP_ENTER)
            displayScale = 1;

        // lut modifications
        if ((key == 'A')||(key == 'Z')||(key == 'Q')||(key == 'S'))
        {
            if (key == 'A')
                lum += 10;
            if (key == 'Z')
                lum -= 10;

            if (key == 'Q')
                gam -= 0.1;
            if (key == 'S')
                gam += 0.1;

        	// Build the lookup table
        	double exponent = 1 / gam;
        	double v = 255.0 * (double)pow((double)255, -exponent);
        	for(int i = 0; i < 256; i++) {
        		double color = (double)pow((double)i, exponent) * v;
                color = MAX(0.0, MIN(color-lum, 255.0));
        		LUT[i] = (BYTE)floor(color + 0.5);
        	}
        }
        if (key == 'P')
            pingpong = !pingpong;
        if (key == 'I')
            sens = -sens;

        if (key == GLFW_KEY_PAGE_UP)
            sequence->SetProxy((sequence->GetProxy())+1);

        if (key == GLFW_KEY_PAGE_DOWN)
            if ((sequence->GetProxy())>1) sequence->SetProxy((sequence->GetProxy())-1);

    }
}

static void My_Mouse_Pos_Callback(GLFWwindow* window, double x, double y)
//void GLFWCALL My_Mouse_Pos_Callback(int x, int y)
{
    mouseX = x;
    mouseY = y;
}

static void My_Mouse_Button_Callback(GLFWwindow* window, int button, int action, int mods)
//void GLFWCALL My_Mouse_Button_Callback(int button, int action)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
            mouseXInit = mouseX;
        else
            mouseXInit = -1;
    }
}
//========================================================================
// main()
//========================================================================

int main( int argc,char *argv[])
{

//    typedef unsigned char uchar;
    int     width, height;
    unsigned int frames, seqLen;
    double  t, t0, tfr;
    double fps = 0.0;
    char    titlestr[ 200 ];
    char    InputName[ 200 ];
    int i;
    //int slideTimeInit;

    FreeImage_Initialise();

    //const char *InputName   = cimg_option("-i","E:/projets/court_metrage/JPG/la_derniere_pelle_DVn%04d.jpg","Nom generique des images");
    strcpy(InputName, "toto.%04d.jpg");//cimg_option("-i","E:/projets/dev/opengl/viewer_freeimage/lena.n%04d.jpg","Nom generique des images");
    FirstFrame    = 1;//cimg_option("-f",1,"Number of the first image");
    LastFrame     = 1;//cimg_option("-l",1,"Number of the last image"); //249 normalement
    StepFrame     = 1;

    bool argOk = false;
    for(i=1; i < argc; i++) {
        if ((strcmp(argv[i],"--help")==0)||(strcmp(argv[i],"-help")==0))
        {
           printf("exemple: viewer_freeimage toto.001.tga\n\n");
           printf("commandline:\n");
           printf("  --help  this help\n");
           printf("  -s force start frame (default 1)\n");
           printf("  -e force end frame (default 1)\n");
           printf("  -n force read every n frame (default 1)\n");
           printf("  -f filename (default toto.%%04d.jpg)\n\n");
           printf("keyboard:\n");
           printf("  a z  change luminosity\n");
           printf("  q s  change gamma");
           printf("  right  next frame\n");
           printf("  left  previous frame\n");
           printf("  top  play once\n");
           printf("  bottom  play repeat\n");
           printf("  p  pingpong mode\n");
           printf("  i  inverted play\n");
           printf("  pg_up/pg_down  increase/decrease proxy\n");
           printf("  +/-  zoom in/out\n");
           printf("  numpad_enter  zoom init\n");
           printf("mouse:\n");
           printf("  left clic & move slide in frames");
           return 0;
        }

        if ((strcmp(argv[i],"-s")==0)&&(argc>i+1)) { FirstFrame = atoi(argv[++i]); argOk = true; }
        if ((strcmp(argv[i],"-e")==0)&&(argc>i+1)) { LastFrame = atoi(argv[++i]); argOk = true;}
        if ((strcmp(argv[i],"-n")==0)&&(argc>i+1)) { StepFrame = atoi(argv[++i]); argOk = true;}
        if ((strcmp(argv[i],"-f")==0)&&(argc>i+1)) { strcpy(InputName,argv[++i]); argOk = true;}
    }

    // automatic frame detection
    if (!argOk)
    {
        //we are looking for numbers from the end of the filename
        char concatName[255];
        strcpy(InputName,argv[argc-1]);

        strcpy(concatName,InputName);

        int begin =0;
        int end = 0;
        int pos = strlen(concatName)-1;

        do
        {
            if ((concatName[pos]<=57)&&(concatName[pos]>=48))
            {
                if (end==0)end = pos;
            }
            else
                if (end!=0)
                {
                    begin = pos;
                    concatName[pos+1]=0;
                }
            pos--;
        }while ((begin==0)&&(pos>0));

        //if there is no number in the file name first frame and last frame are equal to 0
        if ((begin == 0) && (end == 0)){
            FirstFrame = LastFrame = 0;
        }
        else{
                //else we are searching for a sequence
            char startFramNum[end-begin+1];
            strncpy(startFramNum,InputName+begin+1,end+1);
            startFramNum[end-begin]='\0';
            FirstFrame = atoi(startFramNum);

            if (end-begin==1)
                sprintf(InputName,"%s%%d%s",concatName,concatName+end+1);
            else
                sprintf(InputName,"%s%%0%dd%s",concatName,end-begin,concatName+end+1);

            sprintf(concatName,InputName,FirstFrame);
            int curFrame = FirstFrame;
            for (i=1;i<50;i++)
            {
                sprintf(concatName,InputName,curFrame+i);
                if (FileExists(concatName))
                {
                    //cout<<concatName<<"\n";
                    StepFrame = i;
                    break;
                }
            }

            sprintf(concatName,InputName,FirstFrame);

            do{
                curFrame += StepFrame;
                sprintf(concatName,InputName,curFrame);

            }while(FileExists(concatName));

            LastFrame = curFrame-StepFrame;
            cout<<"automatic frame detection:\n";
            cout<<"first: "<<FirstFrame<<"\n";
            cout<<"last: "<<LastFrame<<"\n";
            cout<<"step: "<<StepFrame<<"\n";
        }
    }

    sequence = new Sequence(InputName, FirstFrame, LastFrame, StepFrame);
    seqLen = sequence->GetFrameCount();

    GLFWwindow* window;
    glfwSetErrorCallback(error_callback);
    // Initialise GLFW
    if (!glfwInit())
        exit(EXIT_FAILURE);

    // Open OpenGL window
    window = glfwCreateWindow(800, 600, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
        return 0;
    }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, My_Key_Callback);
    glfwSetCursorPosCallback(window, My_Mouse_Pos_Callback);
    glfwSetMouseButtonCallback(window, My_Mouse_Button_Callback);

    // Enable sticky keys
    glfwSetInputMode(window, GLFW_STICKY_KEYS, true);

    // Disable vertical sync (on cards that support it)
    glfwSwapInterval( 1 );

    // Enable texturing
    glEnable( GL_TEXTURE_2D );

    // Main loop
    //running = GL_TRUE;
    frames = 0;
    curTime = 0;
    glfwSetTime(0);
    t = t0 = 0;//glfwGetTime();
    tfr = 0;

    //init lookup table
	for(int i = 0; i < 256; i++) {
		LUT[i] = (BYTE)i;
	}

    //char * name = (char *)malloc((int) BUFSIZ*sizeof(char));
    char * name = new char[BUFSIZ];

    while( !glfwWindowShouldClose(window) )
    {

        if (mouseXInit == -1)
        {
            if (!pause)curTime += sens;
        }
        else
        {
            int offsetFrame = (mouseX-mouseXInit)/5;
            curTime += offsetFrame;
            mouseXInit += offsetFrame*5;
        }

        if (curTime>=seqLen)
        {
            if (pingpong)
            {
                curTime = (seqLen-1) + (seqLen-1) - curTime;
                sens = -1;
            }
            else if (repeat)
            {
                curTime = 0;
            }
            else
            {
                curTime = seqLen-1;
                pause = true;
            }
        }

        if (curTime<0)
        {
            if (pingpong)
            {
                curTime = - curTime;
                sens = 1;
            }
            else if (repeat)
            {
                curTime = seqLen-1;
            }
            else
            {
                curTime = 0;
                pause = true;
            }
        }
        tfr += t;
                // Calculate and display FPS (frames per second)
        if( tfr > 1.0f || frames == 0)
        {
            fps = (double)frames / (double)tfr;
            tfr = 0;
            //t0 = tfr;
            frames = 0;
        }

        sprintf( titlestr, "Sequence viewer frame %s (%.1f FPS) L=%.1f G=%.1f", (sequence->Get_image_name(curTime)).c_str(), fps, lum, gam );
        glfwSetWindowTitle(window, titlestr );

        frames ++;

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        // Get window size (may be different than the requested size)
        glfwGetFramebufferSize(window,  &width, &height );
        height = height > 0 ? height : 1;

        // Set viewport
        glViewport( 0, 0, width, height );

        // Clear color buffer
        glClearColor( 0.0f, 0.0f, 0.0f, 0.0f);
        glClear( GL_COLOR_BUFFER_BIT );

        // Select and setup the projection matrix
        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();

        //gluPerspective( 65.0f, (GLfloat)width/(GLfloat)height, 1.0f, 50.0f );
        glOrtho(-width/2, width/2, -height/2, height/2, -1.0, 10.0);

        // Select and setup the modelview matrix
        glMatrixMode( GL_MODELVIEW );
        glLoadIdentity();

        gluLookAt( 0.0f,  0.0f, 10.0f,    // Eye-position
                   0.0f, 0.0f, 0.0f,    // View-point
                   0.0f,  1.0f,   0.0f );  // Up-vector

        // Draw a textured quad
        glScalef(displayScale,displayScale,displayScale);

        sequence->Display(curTime, LUT);
        glfwSwapBuffers(window);

        t = glfwGetTime();
        glfwSetTime(0);
        tthread::chrono::milliseconds dura( (int)(40 - t));
        this_thread::sleep_for( dura );

        //tfr = glfwGetTime();

        // Swap buffers
        glfwPollEvents();

    }
    delete name;

    FreeImage_DeInitialise();

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}
