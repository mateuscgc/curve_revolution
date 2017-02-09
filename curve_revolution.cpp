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

#define PI 3.14159265
#define X first
#define Y second
typedef pair<double, double> point2D;

struct point3D {
	double x, y, z;
	point3D(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}
};

double T = 0.01;
list <point2D> points;

GLuint 	axisShader;

GLuint 	axisVBO[3];

int		winWdth 	= 800,
		winHeight	= 800,
		total_faces;

			
bool	drawRef		= true;

list<point2D>::iterator event_point;

const int EVENT_INVALID = 0;
const int EVENT_ADD_POINT = (1 << 0);
const int EVENT_MOVE_POINT = (1 << 1);
const int EVENT_REMOVE_POINT = (1 << 2);
int event_type = EVENT_INVALID;

const int REVOLUTION_STEPS = 18;
const int ANGLE_STEP = 360/REVOLUTION_STEPS;

const int MODE_CURVE = (1 << 0);
const int MODE_REVOLUTION = (1 << 1);
int mode = MODE_CURVE;

const int AXIS_Y = (1 << 0);
const int AXIS_X = (1 << 1);
int revolution_axis = AXIS_Y;

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
	cout << "added point" << endl;
    return points.insert(it, pt);
}

void move_point(list<point2D>::iterator it, point2D pt) {
    *it = pt;
}

void remove_point(list<point2D>::iterator pt) {
    points.erase(pt);
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

void curve() {
	int np = points.size();

	int curve_points = np ? 1/T : 0;
	cout << "curve_points: " << curve_points << endl;

ObjectVA	axis_VA;

	// ========== FACE ============

	int used_for_control_faces = max(0, (np-1)*2);
	int used_for_curve_faces = max(0, (curve_points-1)*2);
	total_faces = used_for_control_faces + used_for_curve_faces;
	cout << "used_for_control_faces: " << used_for_control_faces << endl;
	cout << "used_for_curve_faces: " << used_for_curve_faces << endl;

	axis_VA.vFace = (unsigned int *) malloc((used_for_control_faces + used_for_curve_faces)*sizeof(unsigned int));
	if (!axis_VA.vFace)
		exit(-1);

	
	int i;
	for(i = 0; i < np-1; i++) { // Faces from control segments
		axis_VA.vFace[2*i] = i;
		axis_VA.vFace[2*i+1] = i+1;
	}

	for(int i = 0; i < curve_points-1; i++) {
		axis_VA.vFace[used_for_control_faces + 2*i] = i;
		axis_VA.vFace[used_for_control_faces + 2*i+1] = i+1;
	}

	

	// ========== POINT ============

    int used_for_control_points = 3*np;
    int used_for_curve_points = 3*curve_points;
	cout << "used_for_control_points: " << used_for_control_points << endl;
	cout << "used_for_curve_points: " << used_for_curve_points << endl;   

	axis_VA.vPoint 	= (float *) malloc((used_for_control_points + used_for_curve_points)*sizeof(float));
	if (!axis_VA.vPoint)
		exit(-1);

	i = 0;
	for(list<point2D>::iterator pt = points.begin();
		pt != points.end(); pt++, i++) {
		axis_VA.vPoint[3*i] 	= pt->X;
		axis_VA.vPoint[3*i+1] 	= pt->Y;
		axis_VA.vPoint[3*i+2] 	= 0;
	}

	vector<point2D> interpolations;
	int p;
	double t;
	for(t = 0, p = 0; p < curve_points && t <= 1; t+=T, p++) {
		interpolations.clear();
		interpolations.reserve(points.size()); interpolations.insert(interpolations.end(), points.begin(), points.end());
		for(int j = 0; j < np-1; j++) {
			for(int i = 0; i < np-j-1; i++) {
				interpolations[i].X = interpolations[i].X*(1-t) + interpolations[i+1].X*t;
				interpolations[i].Y = interpolations[i].Y*(1-t) + interpolations[i+1].Y*t;
			}
		}
		axis_VA.vPoint[used_for_control_points + 3*p] = interpolations[0].X;
		axis_VA.vPoint[used_for_control_points + 3*p+1] = interpolations[0].Y;
		axis_VA.vPoint[used_for_control_points + 3*p+2] = 0.0;
	}


	// ========== COLORS ============

    int used_for_control_colors = 4*np;
    int used_for_curve_colors = 4*curve_points;
    cout << "used_for_control_colors: " << used_for_control_colors << endl;
	cout << "used_for_curve_colors: " << used_for_curve_colors << endl;

	
	axis_VA.vColor 	= (float *) malloc((used_for_control_colors + used_for_curve_colors)*sizeof(float));
	if (!axis_VA.vColor) 
		exit(-1);

	for(i = 0; i < used_for_control_colors + used_for_curve_colors; i++) {
		if(i >= used_for_control_colors || i % 4 == 3 || i % 4 == 1)
			axis_VA.vColor[i] = 1.0;
		else
			axis_VA.vColor[i] = 0.0;
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

	glBufferData(	GL_ELEMENT_ARRAY_BUFFER, (used_for_control_faces + used_for_curve_faces)*sizeof(unsigned int), 
					axis_VA.vFace, GL_DYNAMIC_DRAW);
	
	free(axis_VA.vPoint);
	free(axis_VA.vColor);
	free(axis_VA.vFace);
}

point3D rotate(point2D pt, double angle) {
	if(revolution_axis == AXIS_Y)
		return point3D(pt.X*cos(angle), pt.Y, pt.X*sin(angle));
	else
		return point3D(pt.X, pt.Y*sin(angle), pt.Y*cos(angle));
}

void revolution() {
	int np = points.size();

	//T = 0.1;

	int curve_points = np ? 1/T : 0;
	cout << "curve_points: " << curve_points << endl;

ObjectVA	axis_VA;

	// ========== FACE ============

	int used_for_surface_faces = max(0, 6*REVOLUTION_STEPS*(curve_points-1));
	total_faces = used_for_surface_faces;

	cout << "used_for_surface_faces: " << used_for_surface_faces << endl;

	axis_VA.vFace = (unsigned int *) malloc(used_for_surface_faces*sizeof(unsigned int));
	if (!axis_VA.vFace)
		exit(-1);

	for(int r = 0; r < REVOLUTION_STEPS; r++) {
		for(int i = 0; i < curve_points-1; i++) {
			// TRIANGLE 1
			// cout << 6*curve_points*r + 6*i << endl;
			axis_VA.vFace[6*(curve_points-1)*r + 6*i] = (curve_points-1)*r + i;
			axis_VA.vFace[6*(curve_points-1)*r + 6*i+1] = (curve_points-1)*r + i+1;
			axis_VA.vFace[6*(curve_points-1)*r + 6*i+2] = ((curve_points-1)*(r+1))%((curve_points-1)*REVOLUTION_STEPS) + i;

			// TRIANGLE 2
			axis_VA.vFace[6*(curve_points-1)*r + 6*i+3] = ((curve_points-1)*(r+1))%((curve_points-1)*REVOLUTION_STEPS) + i+1;
			axis_VA.vFace[6*(curve_points-1)*r + 6*i+4] = ((curve_points-1)*(r+1))%((curve_points-1)*REVOLUTION_STEPS) + i;
			axis_VA.vFace[6*(curve_points-1)*r + 6*i+5] = (curve_points-1)*r + i+1;
		}
	}


	// ========== POINT ============

    int used_for_surface_points = 3*REVOLUTION_STEPS*curve_points;
	cout << "used_for_surface_points: " << used_for_surface_points << endl;   

	axis_VA.vPoint 	= (float *) malloc(used_for_surface_points*sizeof(float));
	if (!axis_VA.vPoint)
		exit(-1);

	vector<point3D> controls[REVOLUTION_STEPS];
	double angle;
	for(int r = 0; r < REVOLUTION_STEPS; r++) {
		angle = r*ANGLE_STEP*PI/180;
		for(list<point2D>::iterator pt = points.begin(); pt != points.end(); pt++) {
			controls[r].push_back(rotate(*pt, angle));
		}
	}

	vector<point3D> interpolations;
	int p;
	double t;
	for(int r = 0; r < REVOLUTION_STEPS; r++) {
		for(t = 0, p = 0; p < curve_points && t <= 1; t+=T, p++) {
			interpolations.clear();
			interpolations.reserve(controls[r].size()); interpolations.insert(interpolations.end(), controls[r].begin(), controls[r].end());
			for(int j = 0; j < np-1; j++) {
				for(int i = 0; i < np-j-1; i++) {
					interpolations[i].x = interpolations[i].x*(1-t) + interpolations[i+1].x*t;
					interpolations[i].y = interpolations[i].y*(1-t) + interpolations[i+1].y*t;
					interpolations[i].z = interpolations[i].z*(1-t) + interpolations[i+1].z*t;
				}
			}
			axis_VA.vPoint[r*3*curve_points + 3*p] = interpolations[0].x;
			axis_VA.vPoint[r*3*curve_points + 3*p+1] = interpolations[0].y;
			axis_VA.vPoint[r*3*curve_points + 3*p+2] = interpolations[0].z;
		}
	}

	// ========== COLORS ============

    int used_for_surface_colors = 4*REVOLUTION_STEPS*curve_points;
	cout << "used_for_surface_colors: " << used_for_surface_colors << endl;

	
	axis_VA.vColor 	= (float *) malloc(used_for_surface_colors*sizeof(float));
	if (!axis_VA.vColor) 
		exit(-1);

	for(int i = 0; i < used_for_surface_colors; i++) {
			axis_VA.vColor[i] = 1.0;
	}

	axis_VA.vTextCoord 	= NULL;
	axis_VA.vNormal 	= NULL;

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[0]);

	glBufferData(	GL_ARRAY_BUFFER, used_for_surface_points*sizeof(float), 
					axis_VA.vPoint, GL_DYNAMIC_DRAW);

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[1]);

	glBufferData(	GL_ARRAY_BUFFER, used_for_surface_colors*sizeof(float), 
					axis_VA.vColor, GL_DYNAMIC_DRAW);

	glBindBuffer(	GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);

	glBufferData(	GL_ELEMENT_ARRAY_BUFFER, used_for_surface_faces*sizeof(unsigned int), 
					axis_VA.vFace, GL_DYNAMIC_DRAW);
	
	free(axis_VA.vPoint);
	free(axis_VA.vColor);
	free(axis_VA.vFace);

}

void axis() {
		
ObjectVA	axis_VA;

	axis_VA.vFace = (unsigned int *) malloc(3*2*sizeof(unsigned int));
	if (!axis_VA.vFace) 
		exit(-1);
	
	axis_VA.vFace[0] = 0;
	axis_VA.vFace[1] = 3;
	axis_VA.vFace[2] = 0;
	axis_VA.vFace[3] = 2;
	axis_VA.vFace[4] = 0;
	axis_VA.vFace[5] = 1;
			
	total_faces = 6;
	
	axis_VA.vPoint 	= (float *) malloc(4*3*sizeof(float));
	if (!axis_VA.vPoint) 
		exit(-1);
	
	axis_VA.vPoint[0] = 0.0;
	axis_VA.vPoint[1] = 0.0;
	axis_VA.vPoint[2] = 0.0;

	axis_VA.vPoint[3] = 5.0;
	axis_VA.vPoint[4] = 0.0;
	axis_VA.vPoint[5] = 0.0;

	axis_VA.vPoint[6] = 0.0;
	axis_VA.vPoint[7] = 5.0;
	axis_VA.vPoint[8] = 0.0;

	axis_VA.vPoint[9] = 0.0;
	axis_VA.vPoint[10] = 0.0;
	axis_VA.vPoint[11] = 5.0;
	
	axis_VA.vColor 	= (float *) malloc(4*4*sizeof(float));
	if (!axis_VA.vColor) 
		exit(-1);
	
	axis_VA.vColor[0] = 1.0;
	axis_VA.vColor[1] = 1.0;
	axis_VA.vColor[2] = 1.0;
	axis_VA.vColor[3] = 1.0;

	axis_VA.vColor[4] = 1.0;
	axis_VA.vColor[5] = 0.0;
	axis_VA.vColor[6] = 0.0;
	axis_VA.vColor[7] = 1.0;

	axis_VA.vColor[8] = 0.0;
	axis_VA.vColor[9] = 1.0;
	axis_VA.vColor[10] = 0.0;
	axis_VA.vColor[11] = 1.0;

	axis_VA.vColor[12] = 0.0;
	axis_VA.vColor[13] = 0.0;
	axis_VA.vColor[14] = 1.0;
	axis_VA.vColor[15] = 1.0;
	
	axis_VA.vTextCoord 	= NULL;
	axis_VA.vNormal 	= NULL;

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[0]);

	glBufferData(	GL_ARRAY_BUFFER, 4*3*sizeof(float), 
					axis_VA.vPoint, GL_STATIC_DRAW);

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[1]);

	glBufferData(	GL_ARRAY_BUFFER, 4*4*sizeof(float), 
					axis_VA.vColor, GL_STATIC_DRAW);

	glBindBuffer(	GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);

	glBufferData(	GL_ELEMENT_ARRAY_BUFFER, 3*2*sizeof(unsigned int), 
					axis_VA.vFace, GL_STATIC_DRAW);
	
	free(axis_VA.vPoint);
	free(axis_VA.vColor);
	free(axis_VA.vFace);	
}
		
/// ***********************************************************************
/// **
/// ***********************************************************************

void draw() {

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
	glDrawElements(GL_LINE_STRIP, total_faces, GL_UNSIGNED_INT, 0);

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

		case 'R'	:
		case 'r'	:
						mode = mode^MODE_CURVE^MODE_REVOLUTION;
						break;

		case 'X'	:
		case 'x'	:
						revolution_axis = AXIS_X;
						break;

		case 'Y'	:
		case 'y'	:
						revolution_axis = AXIS_Y;
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
			if(mode & MODE_CURVE) {
				if(glutGetModifiers() & GLUT_ACTIVE_CTRL) {
					event_type = EVENT_REMOVE_POINT;
					event_point = point_exists(real_coords);
				} else {
					event_point = point_exists(real_coords);
					if(event_point != points.end())
						event_type = EVENT_MOVE_POINT;
					else {
						event_type = EVENT_ADD_POINT;
						list<point2D>::iterator it = get_segment(real_coords);
						event_point = add_point_before(real_coords, it);
					}
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
					if(event_point != points.end() && point_exists(real_coords) == event_point) {
						remove_point(event_point);
					}
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
	if(!state)
		handle_initialization(button, get_real_coords(x, y));
	else
		handle_closing(button, get_real_coords(x, y));
	if(mode & MODE_CURVE) glutPostRedisplay();
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
		if(mode & MODE_CURVE)
			curve();
		else
			revolution();
    	draw();
		axis();
    	draw();
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
	//cin >> T;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow("Curve Revolution");
	glutReshapeWindow(winWdth, winHeight);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);

	//int n;
    //cin >> n;

    //double x, y;
    //for(int i = 0; i < n; i++) {
    //    cin >> x >> y;
    //    points.push_back({x,y});
    //}

	initGL();

	glGenBuffers(3, axisVBO);
	
	initShaders();
	
	glutMainLoop();

	return(0);
}
