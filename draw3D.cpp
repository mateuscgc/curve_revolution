#include <bits/stdc++.h>
#include <cassert>
#include <GL/glew.h>
#include <glm/vec3.hpp> 
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 


#if defined (__APPLE__) || defined(MACOSX)
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include "myDataStructures.h"
#include "initShaders.h"
					
using namespace std;

#define X first
#define Y second
typedef pair<double, double> point2D;

double delta = 0.2;
list <point2D> points;

GLuint 	axisShader;

GLuint 	axisVBO[3];

int		winWdth 	= 800,
		winHeight	= 800,
		total_faces,

		faces_used_for_controls,
		num_segments_for_triple,
		faces_used_for_curve;


			
bool	drawRef		= true;

int event_type;
list<point2D>::iterator event_point;

const int EVENT_INVALID = 0;
const int EVENT_ADD_POINT = (1 << 0);
const int EVENT_MOVE_POINT = (1 << 1);
const int EVENT_REMOVE_POINT = (1 << 2);

const double POINT_SELECTION_ERROR_MARGIN = 0.1;

inline bool operator==(const point2D& a, const point2D& b) {
	return (hypot(abs(a.X-b.X), abs(a.Y-b.Y)) <= POINT_SELECTION_ERROR_MARGIN);
}

// Geometric

double sx(list<point2D>::iterator it) { return abs(prev(it)->X - it->X); }
double sy(list<point2D>::iterator it) { return abs(prev(it)->Y - it->Y); }
double sx(list<point2D>::iterator it, double x) { return abs(it->X - x); }
double sy(list<point2D>::iterator it, double y) { return abs(it->Y - y); }

double cross_product(list<point2D>::iterator a, list<point2D>::iterator b) {
    return sx(a)*sy(b) - sx(b)*sy(a);
}

double cross_product(list<point2D>::iterator it, point2D pt) {
    return sx(it)*sy(prev(it), pt.Y) - sx(prev(it), pt.X)*sy(it);  
}

bool in_interval(double a, double l1, double l2) {
    return (min(l1, l2) <= a && a <= max(l1, l2));
}

bool point_in_segment(list<point2D>::iterator it, point2D pt) {
	cout << "cross " << cross_product(it, pt) << endl;
    return (abs(cross_product(it, pt)) < 1 && in_interval(pt.X, prev(it)->X, it->X) && in_interval(pt.Y, prev(it)->Y, it->Y));
}

list<point2D>::iterator get_segment(point2D pt) {
    for(list<point2D>::iterator it = next(points.begin()); it != points.end(); it++)
        if(point_in_segment(it, pt))
            return it;
    return points.end();
}

list<point2D>::iterator add_point_before(point2D pt, list<point2D>::iterator it) {
    return points.insert(it, pt);
}

void move_point(list<point2D>::iterator it, point2D pt) {
    *it = pt;
}

void remove_point(list<point2D>::iterator pt) {
	//cout << points.size();
    //cout << " # removed point: (" << pt->X << ", " << pt->Y << ") # ";
    points.erase(pt);
    //cout << points.size() << endl;
}

list<point2D>::iterator point_exists(point2D pt) {
    for(list<point2D>::iterator it = points.begin(); it != points.end(); it++)
        if(*it == pt)
            return it;
    return points.end();   
}
		
/// ***********************************************************************
/// **
/// ***********************************************************************

// OPENGL

void criaVBO() {
	int np = points.size();

ObjectVA	axis_VA;

	// ========== FACE ============

	faces_used_for_controls = (np-1)*2;
	num_segments_for_triple = 1/delta;
	faces_used_for_curve = 2*((np-1)/2)*num_segments_for_triple;
	total_faces = faces_used_for_controls + faces_used_for_curve;
	//cout << faces_used_for_controls << endl;
	//cout << faces_used_for_curve << endl;

	axis_VA.vFace = (unsigned int *) malloc((faces_used_for_controls + faces_used_for_curve)*sizeof(unsigned int));
	if (!axis_VA.vFace)
		exit(-1);

	
	int i;
	for(i = 0; i < np-1; i++) { // Faces from control segments
		//cout << 2*i << endl;
		//cout << 2*i+1 << endl;
		axis_VA.vFace[2*i] = i;
		axis_VA.vFace[2*i+1] = i+1;
	}

    for(int i = 0; i < (np-1)/2; i++) { // Faces from curve
    	for(int t = 0; t < 2*num_segments_for_triple; t += 2) {
    		//cout << faces_used_for_controls+2*num_segments_for_triple*i+t << " " << np+num_segments_for_triple*i+t/2 << endl;
    		//cout << faces_used_for_controls+2*num_segments_for_triple*i+t+1 << " " << np+num_segments_for_triple*i+t/2+1 << endl;
    		axis_VA.vFace[faces_used_for_controls+2*num_segments_for_triple*i+t] = np+num_segments_for_triple*i+t/2;
    		axis_VA.vFace[faces_used_for_controls+2*num_segments_for_triple*i+t+1] = np+num_segments_for_triple*i+t/2+1;
    	}
    }
    //cout << endl;

	

	// ========== POINT ============

    int used_for_control_points = 3*np;
	int num_points_for_triple = 1/delta+1;
    int used_for_curve_points = 3*((np-1)/2)*num_points_for_triple;
    //cout << used_for_control_points << endl;
    //cout << num_points_for_triple << endl;
	//cout << used_for_curve_points << endl;


	axis_VA.vPoint 	= (float *) malloc((used_for_control_points + used_for_curve_points)*sizeof(float));
	if (!axis_VA.vPoint)
		exit(-1);

	i = 0;
	for(list<point2D>::iterator pt = points.begin();
		pt != points.end(); pt++, i++) {
		//cout << 3*i << " " << pt->X << endl;
		//cout << 3*i+1 << " " << pt->Y << endl;
		//cout << 3*i+2 << endl;
		axis_VA.vPoint[3*i] 	= pt->X;
		axis_VA.vPoint[3*i+1] 	= pt->Y;
		axis_VA.vPoint[3*i+2] 	= 0;
	} 

	i = 0;
    double t = 0;
    for(list<point2D>::iterator pt = next(points.begin()); next(pt) != points.end(); pt++, pt++, i++) { // Points from curve
    	t = 0;
    	for(int j = 0; j < 3*num_points_for_triple; j += 3, t += delta) {
    		//cout << t << endl;
    		
    		axis_VA.vPoint[used_for_control_points+3*num_points_for_triple*i+j] = (1-t)*(1-t)*prev(pt)->X + 2*(1-t)*t*pt->X + t*t*next(pt)->X;
    		axis_VA.vPoint[used_for_control_points+3*num_points_for_triple*i+j+1] = (1-t)*(1-t)*prev(pt)->Y + 2*(1-t)*t*pt->Y + t*t*next(pt)->Y;
    		axis_VA.vPoint[used_for_control_points+3*num_points_for_triple*i+j+2] = 0.0;
    	
    		//cout << used_for_control_points+3*num_points_for_triple*i+j << " " <<  axis_VA.vPoint[used_for_control_points+3*num_points_for_triple*i+j] << endl;
    		//cout << used_for_control_points+3*num_points_for_triple*i+j+1 << " " << axis_VA.vPoint[used_for_control_points+3*num_points_for_triple*i+j+1] << endl;
    		//cout << used_for_control_points+3*num_points_for_triple*i+j+2 << " " << axis_VA.vPoint[used_for_control_points+3*num_points_for_triple*i+j+2] << endl;
    	}
    	if(next(next(pt)) == points.end())
    		break;
    }

    int used_for_control_colors = 4*np;
    int used_for_curve_colors = 4*((np-1)/2)*num_points_for_triple;

	
	axis_VA.vColor 	= (float *) malloc((used_for_control_colors + used_for_curve_colors)*sizeof(float));
	if (!axis_VA.vColor) 
		exit(-1);

	for(i = 0; i < used_for_control_colors + used_for_curve_colors; i++) {
		if(i >= used_for_control_colors || i % 4 == 3 || i % 4 == 1)
			axis_VA.vColor[i] = 1.0;
		else
			axis_VA.vColor[i] = 0.0;


		//cout << i << " " << axis_VA.vColor[i] << endl;
	}
	
	axis_VA.vTextCoord 	= NULL;
	axis_VA.vNormal 	= NULL;

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[0]);

	glBufferData(	GL_ARRAY_BUFFER, (used_for_control_points + used_for_curve_points)*sizeof(float), 
					axis_VA.vPoint, GL_DYNAMIC_DRAW);

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[1]);

	glBufferData(	GL_ARRAY_BUFFER, (used_for_control_colors + used_for_curve_colors)*sizeof(float), 
					axis_VA.vColor, GL_DYNAMIC_DRAW);

	glBindBuffer(	GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);

	glBufferData(	GL_ELEMENT_ARRAY_BUFFER, (faces_used_for_controls + faces_used_for_curve)*sizeof(unsigned int), 
					axis_VA.vFace, GL_DYNAMIC_DRAW);
	
	free(axis_VA.vPoint);
	free(axis_VA.vColor);
	free(axis_VA.vFace);	
}
		
/// ***********************************************************************
/// **
/// ***********************************************************************

void drawAxis() {

int attrV, attrC; 
	
	glBindBuffer(GL_ARRAY_BUFFER, axisVBO[0]); 		
	attrV = glGetAttribLocation(axisShader, "aPosition");
	glVertexAttribPointer(attrV, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrV);

	glBindBuffer(GL_ARRAY_BUFFER, axisVBO[1]); 		
	attrC = glGetAttribLocation(axisShader, "aColor");
	glVertexAttribPointer(attrC, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrC);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);
	glDrawElements(GL_POINTS, total_faces, GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(attrV);
	glDisableVertexAttribArray(attrC);

	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void reshape(int w, int h) {

	winWdth 	= w;
	winHeight	= h;
	glViewport(0, 0, winWdth, winHeight);      
	glutPostRedisplay();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void keyboard (unsigned char key, int x, int y) {

	switch (key) {	
		case 27  	: 	exit(0);
						break;
		case 'A'	: 	
		case 'a'	: 	drawRef = !drawRef;
						break;
		}
	glutPostRedisplay();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

point2D get_real_coords(double x, double y) {
	 cout << x/(double)winWdth*20-10 << " " << -(y/(double)winHeight*20-10) << endl;
	return { x/(double)winWdth*20-10, -(y/(double)winHeight*20-10) };
}

void handle_initialization(int button, point2D real_coords) {
	switch(button) {
		case GLUT_LEFT_BUTTON:
			if(glutGetModifiers() && GLUT_ACTIVE_CTRL) {
				event_type = EVENT_REMOVE_POINT;
				event_point = point_exists(real_coords);
			} else {
				//...
				event_point = point_exists(real_coords);
				if(event_point != points.end())
					event_type = EVENT_MOVE_POINT;
				else {
					event_type = EVENT_ADD_POINT;
					list<point2D>::iterator it = get_segment(real_coords);
					event_point = add_point_before(real_coords, it);					
				}
			}
		break;
		default:
			event_type = EVENT_INVALID;
		break;
	}
}

void handle_closing(int button, point2D real_coords) {
	switch(button) {
		case GLUT_LEFT_BUTTON:
			switch(event_type) {
				case EVENT_REMOVE_POINT:
					if(event_point != points.end() && point_exists(real_coords) == event_point)
						remove_point(event_point);
				break;
				case EVENT_MOVE_POINT:
				case EVENT_ADD_POINT:
					move_point(event_point, real_coords);
				break;
			}
		break;
	}
}

void mouse(int button, int state, int x, int y) {
	cout << button << " " << state << " " << x << " " << y << endl;
	if(!state)
		handle_initialization(button, get_real_coords(x, y));
	else
		handle_closing(button, get_real_coords(x, y));
	criaVBO();
	glutPostRedisplay();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void display(void) { 
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	glm::mat4 MVP 	= glm::ortho(	-10.0, 10.0, -10.0, 10.0, 0.0, 10.0); 
	
    if (drawRef) {
    	glUseProgram(axisShader);
		int loc = glGetUniformLocation( axisShader, "uMVP" );
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(MVP));
    	drawAxis();
    	}
	
	glutSwapBuffers();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void initGL(void) {

	glClearColor(0.0, 0.0, 0.0, 0.0);	
	glPointSize(3.0);
	
	if (glewInit()) {
		cout << "Unable to initialize GLEW ... exiting" << endl;
		exit(EXIT_FAILURE);
		}
		
	cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;	
	
	cout << "Opengl Version: " << glGetString(GL_VERSION) << endl;
	cout << "Opengl Vendor : " << glGetString(GL_VENDOR) << endl;
	cout << "Opengl Render : " << glGetString(GL_RENDERER) << endl;
	cout << "Opengl Shading Language Version : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void initShaders(void) {

    // Load shaders and use the resulting shader program
    axisShader = InitShader( "axisShader.vert", "axisShader.frag" );
}

/* ************************************************************************* */
/* ************************************************************************* */
/* *****                                                               ***** */
/* ************************************************************************* */
/* ************************************************************************* */

int main(int argc, char *argv[]) {
	cin >> delta;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow("draw3D");
	glutReshapeWindow(winWdth, winHeight);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);

	int n;
    cin >> n;

    double x, y;
    for(int i = 0; i < n; i++) {
        cin >> x >> y;
        points.push_back({x,y});
    }

	initGL();

	glGenBuffers(3, axisVBO);
	
	criaVBO();
	
	initShaders();
	
	glutMainLoop();

	return(0);
}
    
   
