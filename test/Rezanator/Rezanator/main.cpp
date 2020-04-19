////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Syed Reza Ali
//http://www.syedrezaali.com
//MAT 594CM - Project 1
//C++/OpenGL/GLUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <stdlib.h>
#ifdef __APPLE__
#include <GLUT/GLUT.h>
#else
#include <GL/glut.h>
#endif
#include <math.h>
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

GLfloat angle = 0.0; //the rotation value
float xpos = 0, ypos = 0, zpos = 0, xrot = 0, yrot = 90;
float cRadius = 5.0f; // our radius distance from our character
float lastx, lasty;
bool rotation;
float alpha = 0.03;
float size = 0.0;
float Rcolor = 0.0, Gcolor = 0.0, Bcolor = 0.0;
int chooser;

void specialkeys(int key, int x, int y)
{
	switch ( key )
	{
		case GLUT_KEY_UP :
			cRadius-=0.25;
			break;
		case GLUT_KEY_DOWN :
			cRadius+=0.25;
			break;
		case GLUT_KEY_RIGHT :
			alpha+=0.01;
			if(alpha>1.0)
			{
				alpha = 1.0;
			}
			break;
		case GLUT_KEY_LEFT :
			alpha-=0.01;
			if(alpha<0.0)
			{
				alpha = 0.0;
			}
			break;
			
		default:
			break;
	}
}

void keyboard (unsigned char key, int x, int y) {
    
	if (key=='1')
	{
		chooser = 1;
	}
	if (key=='2')
	{
        chooser = 2;
	}
	if (key=='3')
	{
		chooser = 3;
	}
	if (key=='4')
	{
		chooser = 4;
	}
	if (key=='5')
	{
		chooser = 5;
	}
	if (key=='6')
	{
		chooser = 6;
	}
	if (key=='7')
	{
		chooser = 7;
	}
	if (key=='8')
	{
		chooser = 8;
	}
	if (key=='9')
	{
		chooser = 9;
	}
	if (key=='0')
	{
		chooser = 0;
	}
	
	if (key=='r')
	{
		rotation = !rotation;
	}
	
	if (key=='q')
	{
		xrot += 1;
		if (xrot >360) xrot -= 360;
	}
	
	if (key=='z')
	{
		xrot -= 1;
		if (xrot < -360) xrot += 360;
	}
	
	if (key=='w')
	{
		float xrotrad, yrotrad;
		yrotrad = (yrot / 180 * 3.141592654f);
		xrotrad = (xrot / 180 * 3.141592654f);
		xpos += float(sin(yrotrad));
		zpos -= float(cos(yrotrad));
		ypos -= float(sin(xrotrad));
	}
	
	if (key=='s')
	{
		float xrotrad, yrotrad;
		yrotrad = (yrot / 180 * 3.141592654f);
		xrotrad = (xrot / 180 * 3.141592654f);
		xpos -= float(sin(yrotrad));
		zpos += float(cos(yrotrad));
		ypos += float(sin(xrotrad));
	}
	
	if (key=='d')
	{
		float yrotrad;
		yrotrad = (yrot / 180 * 3.141592654f);
		xpos += float(cos(yrotrad)) * 0.2;
		zpos += float(sin(yrotrad)) * 0.2;
	}
	
	if (key=='a')
	{
		float yrotrad;
		yrotrad = (yrot / 180 * 3.141592654f);
		xpos -= float(cos(yrotrad)) * 0.2;
		zpos -= float(sin(yrotrad)) * 0.2;
	}
	if (key==27) { //27 is the ascii code for the ESC key
		//glutLeaveGameMode();
		exit (0); //end the program
	}
}

void mouseMovement(int x, int y) {
	int diffx=x-lastx; //check the difference between the current x and the last x position
	int diffy=y-lasty; //check the difference between the current y and the last y position
	lastx=x; //set lastx to the current x position
	lasty=y; //set lasty to the current y position
	xrot += (float) diffy; //set the xrot to xrot with the addition of the difference in the y position
	yrot -= (float) diffx;// set the xrot to yrot with the addition of the difference in the x position
    
}

void cube (void) {
	size = 0.0;
	glPushMatrix();
	for (int i = 0; i < 201; i++)
	{
		size+=0.025;
		glColor4f(Rcolor,Gcolor,Bcolor,alpha);
		glRotatef(angle,1.0, 0.0, 0.0);
		glRotatef(angle,0.0,1.0, 0.0);
		glRotatef(angle,0.0, 0.0, 1.0);
		
		switch(chooser)
		{
			case 1:
				glutSolidCube(size);
				break;
			case 2:
				glutWireCube(size);
				break;
			case 3:
				glutSolidCone(size, size, 10, 10);
				break;
			case 4:
				glutWireCone(size, size, 10, 10);
				break;
			case 5:
				glutSolidSphere(size, i, i);
				break;
			case 6:
				glutWireSphere(size, i, i);
				break;
			case 7:
				glPushMatrix();
				glScalef( size, size, size );
				glutSolidDodecahedron();
				glPopMatrix();
				break;
			case 8:
				glPushMatrix();
				glScalef( size, size, size );
				glutWireDodecahedron();
				glPopMatrix();
				break;
			case 9:
				glPushMatrix();
				glScalef( size, size, size );
				glutSolidIcosahedron();
				glPopMatrix();
				break;
			case 0:
				glPushMatrix();
				glScalef( size, size, size );
				glutWireIcosahedron();
				glPopMatrix();
				break;
				
			default:
				break;
		}
	}
	glPopMatrix();
}


void init (void) {
	glEnable (GL_DEPTH_TEST);	//enable the depth testing
	glEnable (GL_LIGHTING);		//enable the lighting
	glEnable (GL_LIGHT0);	//enable LIGHT0, our Diffuse Light
	glEnable (GL_COLOR_MATERIAL);
	glShadeModel (GL_SMOOTH); //set the shader to smooth shader
}

void camera (void) {
	glRotatef(xrot,1.0,0.0,0.0); //rotate our camera on teh x-axis (left and right)
	glRotatef(yrot,0.0,1.0,0.0); //rotate our camera on the y-axis (up and down)
	glTranslated(-xpos,-ypos,-zpos); //translate the screen to the position of our camera
}

void display (void) {
	glClearColor (0.0,0.0,0.0,1.0);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	glEnable(GL_BLEND); //enable the blending
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //set the blend function
	//glBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR); //set the blend function
	//glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA); //set the blend function
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); //set the blend function
	//glBlendFunc(GL_SRC_ALPHA,GL_SRC_ALPHA_SATURATE); //set the blend function
	//glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); //set the blend function
	//glBlendFunc(GL_SRC_ALPHA,GL_SRC_ALPHA_SATURATE); //set the blend function
	glLoadIdentity();
	
	glTranslatef(0.0f, 0.0f, -cRadius);
	glRotatef(xrot,1.0,0.0,0.0);
	glColor3f(1.0f, 0.0f, 0.0f);
	glRotatef(yrot,0.0,1.0,0.0);  //rotate our camera on the y-axis (up and down)
	glTranslated(-xpos,0.0f,-zpos); //translate the screen to the position of our camera
	glColor3f(1.0f, 1.0f, 1.0f);
	cube();
	glutSwapBuffers();
	Rcolor = sin(angle/50);
	Gcolor = cos(angle/50);
	Bcolor = 2*sin(angle/50)*cos(angle/50);
	if(rotation)
	{
		angle+=0.02;
	}
}

void reshape (int w, int h) {
	glViewport (0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode (GL_PROJECTION); //set it so we can play with the 'camera'
	glLoadIdentity (); //replace the current matrix with the Identity Matrix
	gluPerspective (60, (GLfloat)w / (GLfloat)h, 1.0, 100.0); //set the angle of view, the ratio of sight, the near and far factors
	glMatrixMode (GL_MODELVIEW); //switch back the the model editing mode.
}

int main (int argc, char **argv) {
    glutInit (&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	
	//glutGameModeString( "1440x900:32@60" ); //the settings for fullscreen mode
	//glutEnterGameMode(); //set glut to fullscreen using the settings in the line above
	glutInitWindowSize (1280, 720); //set the size of the window
	glutInitWindowPosition (0, 0); //set the position of the window
	glutCreateWindow ("Rezanator");
	init();
    glutDisplayFunc (display);
	glutIdleFunc (display);
	glutReshapeFunc (reshape);
	glutPassiveMotionFunc(mouseMovement); //check for mouse movement
	glutKeyboardFunc (keyboard);//the call for the keyboard function.
	glutSpecialFunc(specialkeys);
    glutMainLoop ();
    return 0;
}


