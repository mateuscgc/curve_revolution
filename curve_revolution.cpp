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
#include "loadpng.h"
					
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
GLuint 	phongShader;

GLuint 	axisVBO[5];

int		winWdth 	= 800,
		winHeight	= 800,
		total_faces;

			
bool	drawRef		= true;

list<point2D>::iterator event_point;

const int EVENT_INVALID = 0;
const int EVENT_ADD_POINT = (1 << 0);
const int EVENT_MOVE_POINT = (1 << 1);
const int EVENT_REMOVE_POINT = (1 << 2);
const int EVENT_MOVE_CAMERA = (1 << 3);
int event_type = EVENT_INVALID;

const int REVOLUTION_STEPS = 180;
const int ANGLE_STEP = 360/REVOLUTION_STEPS;

const int MODE_CURVE = (1 << 0);
const int MODE_REVOLUTION = (1 << 1);
int mode = MODE_CURVE;

const int AXIS_Y = (1 << 0);
const int AXIS_X = (1 << 1);
int revolution_axis = AXIS_Y;

const int SHOW_P = (1 << 0);
const int SHOW_E = (1 << 1);
const int SHOW_F = (1 << 2);
const int SHOW_T = (1 << 3);
int show = SHOW_P | SHOW_E | SHOW_F;

const int FULL_BOTTOM_LEFT = (1 << 0);
const int FULL_UPPER_LEFT = (1 << 1);
const int FULL_BOTTOM_RIGHT = (1 << 2);
const int FULL_UPPER_RIGHT = (1 << 3);
int full_screen = 0;

point2D camera_move_point;

bool waiting_double_click = false;
bool first_click = false;

const double POINT_SELECTION_ERROR_MARGIN = 0.1;

glm::mat4 Model4 = glm::lookAt(
								// glm::vec3(3.5f*cos(angle_y), 3.5f*sin(angle_x), 3.5f*sin(angle_y)*cos(angle_x)),
								glm::vec3(3.5f, 3.5f, 3.5f),
							    glm::vec3(0.0f, 0.0f, 0.0f),
							    glm::vec3(0.0f, -1.0f, 0.0f));

glm::vec3 light_pos = glm::vec3(1.5f, 3.5f, 6.5f);
glm::vec3 light_color = glm::vec3(0.8f, 0.8f, 0.8f);

//int noise3DTexSize = 128;

//const char* filename = argv[1];

// Load file and decode image.
  


void LoadTexture(const char * filename) {
	std::vector<unsigned char> image;
  	unsigned width, height;
  	unsigned error = lodepng::decode(image, width, height, filename);

	// If there's an error, display it.
	if(error != 0) {
	    std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
	}

	// Texture size must be power of two for the primitive OpenGL version this is written for. Find next power of two.
	size_t u2 = 1; while(u2 < width) u2 *= 2;
	size_t v2 = 1; while(v2 < height) v2 *= 2;
	// Ratio for power of two version compared to actual version, to render the non power of two image with proper size.
	double u3 = (double)width / u2;
	double v3 = (double)height / v2;

	// Make power of two version of the image.
	std::vector<unsigned char> image2(u2 * v2 * 4);
	for(size_t y = 0; y < height; y++)
		for(size_t x = 0; x < width; x++)
			for(size_t c = 0; c < 4; c++) {
				image2[4 * u2 * y + 4 * x + c] = image[4 * width * y + 4 * x + c];
			}

	// Enable the texture for OpenGL.
	glEnable(GL_TEXTURE_2D);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_NEAREST = no smoothing
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, u2, v2, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image2[0]);

}

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
	//cout << "cross " << cross_product(it, pt) << endl;
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

point3D rotate(point2D pt, double angle) {
	if(mode == MODE_CURVE || revolution_axis == AXIS_Y)
		return point3D(pt.X*cos(angle), pt.Y, pt.X*sin(angle));
	else
		return point3D(pt.X, pt.Y*sin(angle), pt.Y*cos(angle));
}

vector<point3D> create_curve(vector<point3D>& control_points, int num_curve_points) {
	int np = control_points.size();
	vector<point3D> interpolations;
	vector<point3D> ans;
	int p;
	double t;
	for(t = 0, p = 0; p < num_curve_points && t <= 1; t+=T, p++) {
		interpolations.clear();
		interpolations.reserve(points.size()); interpolations.insert(interpolations.end(), control_points.begin(), control_points.end());
		for(int j = 0; j < np-1; j++) {
			for(int i = 0; i < np-j-1; i++) {
				interpolations[i].x = interpolations[i].x*(1-t) + interpolations[i+1].x*t;
				interpolations[i].y = interpolations[i].y*(1-t) + interpolations[i+1].y*t;
				interpolations[i].z = interpolations[i].z*(1-t) + interpolations[i+1].z*t;
			}
		}
		ans.push_back(interpolations[0]);
	}

	return ans;
}

void create_gl_structures(int used_for_faces, int used_for_points, int used_for_colors, int revolution_steps, int num_drawing_points, vector<point3D> (*point_manipulation)(vector<point3D>&, int), glm::vec3 color) {
	int np = points.size();

	//int num_drawing_points = np ? 1/T : 0;
	//cout << "num_drawing_points: " << num_drawing_points << endl;

ObjectVA	geometry_VA;

	// ========== FACE ============

	total_faces = used_for_faces;
	//cout << "used_for_faces: " << used_for_faces << endl;

	geometry_VA.vFace = (unsigned int *) malloc(used_for_faces*sizeof(unsigned int));
	if (!geometry_VA.vFace)
		exit(-1);

	for(int r = 0; r < revolution_steps; r++) {
		for(int i = 0; i < num_drawing_points-1; i++) {
			// TRIANGLE 1
			geometry_VA.vFace[6*(num_drawing_points-1)*r + 6*i] = (num_drawing_points-1)*r + i;
			geometry_VA.vFace[6*(num_drawing_points-1)*r + 6*i+1] = (num_drawing_points-1)*r + i+1;
			//if(revolution_steps > 1) {
				geometry_VA.vFace[6*(num_drawing_points-1)*r + 6*i+2] = ((num_drawing_points-1)*(r+1))%((num_drawing_points-1)*revolution_steps) + i;
			
				// TRIANGLE 2
				geometry_VA.vFace[6*(num_drawing_points-1)*r + 6*i+3] = ((num_drawing_points-1)*(r+1))%((num_drawing_points-1)*revolution_steps) + i+1;
				geometry_VA.vFace[6*(num_drawing_points-1)*r + 6*i+4] = ((num_drawing_points-1)*(r+1))%((num_drawing_points-1)*revolution_steps) + i;
				geometry_VA.vFace[6*(num_drawing_points-1)*r + 6*i+5] = (num_drawing_points-1)*r + i+1;
			//}	
		}
	}

	// ========== POINT ============

	//cout << "used_for_points: " << used_for_points << endl;   

	geometry_VA.vPoint 	= (float *) malloc(used_for_points*sizeof(float));
	if (!geometry_VA.vPoint)
		exit(-1);

	vector<point3D> controls[revolution_steps];
	double angle;
	for(int r = 0; r < revolution_steps; r++) {
		angle = r*ANGLE_STEP*PI/180;
		for(list<point2D>::iterator pt = points.begin(); pt != points.end(); pt++) {
			controls[r].push_back(rotate(*pt, angle));
		}
	}


	vector<point3D> drawing_points;
	for(int r = 0; r < revolution_steps; r++) {
		if(point_manipulation != nullptr)
			drawing_points = (*point_manipulation)(controls[r], num_drawing_points);
		else
			drawing_points = controls[r];
		for(int p = 0; p < drawing_points.size(); p++) {
			geometry_VA.vPoint[r*3*num_drawing_points + 3*p] = drawing_points[p].x;
			geometry_VA.vPoint[r*3*num_drawing_points + 3*p+1] = drawing_points[p].y;
			geometry_VA.vPoint[r*3*num_drawing_points + 3*p+2] = drawing_points[p].z;
		}
	}

	// ========== NORMAL ============

	//cout << "used_for_points: " << used_for_points << endl;   

	geometry_VA.vNormal = (float *) malloc(used_for_points*sizeof(float));
	if (!geometry_VA.vNormal)
		exit(-1);

	glm::vec3 normal;
	for(int r = 0; r < revolution_steps; r++) {
		for(int p = 1; p < num_drawing_points-1; p++) {
			normal = normalize(glm::vec3(
						geometry_VA.vPoint[r*3*num_drawing_points + 3*p-3] +
						geometry_VA.vPoint[r*3*num_drawing_points + 3*p+3] -
						2*geometry_VA.vPoint[r*3*num_drawing_points + 3*p],

						geometry_VA.vPoint[r*3*num_drawing_points + 3*p-2] +
						geometry_VA.vPoint[r*3*num_drawing_points + 3*p+4] -
						2*geometry_VA.vPoint[r*3*num_drawing_points + 3*p+1],

						geometry_VA.vPoint[r*3*num_drawing_points + 3*p-1] +
						geometry_VA.vPoint[r*3*num_drawing_points + 3*p+5] -
						2*geometry_VA.vPoint[r*3*num_drawing_points + 3*p+2]));

			geometry_VA.vNormal[r*3*num_drawing_points + 3*p] = normal.x;

			geometry_VA.vNormal[r*3*num_drawing_points + 3*p+1] = normal.y;

			geometry_VA.vNormal[r*3*num_drawing_points + 3*p+2] = normal.z;

		}
	}

	// ========== TEXTURE COORDINATES ============


	geometry_VA.vTextCoord = (float *) malloc((used_for_points/3*2)*sizeof(float));
	if (!geometry_VA.vNormal)
		exit(-1);

	for(int r = 0; r < revolution_steps; r++) {
		for(int p = 0; p < num_drawing_points; p++) {

			geometry_VA.vTextCoord[r*2*num_drawing_points + 2*p] = (revolution_axis == AXIS_Y ? (double)r/(double)revolution_steps : (double)p/(double)num_drawing_points);
			geometry_VA.vNormal[r*2*num_drawing_points + 2*p+1] = (revolution_axis == AXIS_X ? (double)r/(double)revolution_steps : (double)p/(double)num_drawing_points);

		}
	}

	// ========== COLORS ============

	//cout << "used_for_colors: " << used_for_colors << endl;
	
	geometry_VA.vColor 	= (float *) malloc(used_for_colors*sizeof(float)) ;
	if (!geometry_VA.vColor) 
		exit(-1);

	for(int i = 0; i < used_for_colors; i++) {
			if(i % 4 == 3)
				geometry_VA.vColor[i] = 1.0;
			else
				geometry_VA.vColor[i] = color[i%4];
	}

	//geometry_VA.vTextCoord 	= NULL;
	//geometry_VA.vNormal 	= NULL;

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[0]);

	glBufferData(	GL_ARRAY_BUFFER, used_for_points*sizeof(float), 
					geometry_VA.vPoint, GL_DYNAMIC_DRAW);

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[1]);

	glBufferData(	GL_ARRAY_BUFFER, used_for_colors*sizeof(float), 
					geometry_VA.vColor, GL_DYNAMIC_DRAW);

	glBindBuffer(	GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);

	glBufferData(	GL_ELEMENT_ARRAY_BUFFER, used_for_faces*sizeof(unsigned int), 
					geometry_VA.vFace, GL_DYNAMIC_DRAW);

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[3]);

	glBufferData(	GL_ARRAY_BUFFER, used_for_points*sizeof(float), 
					geometry_VA.vNormal, GL_DYNAMIC_DRAW);

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[4]);

	glBufferData(	GL_ARRAY_BUFFER, used_for_points/3*2*sizeof(float), 
					geometry_VA.vTextCoord, GL_DYNAMIC_DRAW);
	
	free(geometry_VA.vPoint);
	free(geometry_VA.vNormal);
	free(geometry_VA.vColor);
	free(geometry_VA.vFace);
	free(geometry_VA.vTextCoord);
}

void curve_controls() {
	int np = points.size();
	int used_for_control_faces = max(0, 6*(np-1));
	int used_for_control_points = 3*np;
	int used_for_control_colors = 4*np;
	create_gl_structures(used_for_control_faces, used_for_control_points, used_for_control_colors, 1, np, nullptr, glm::vec3(0.2, 0.8, 0.4));
}

void curve() {
	int np = points.size();
	int num_curve_points = np >= 2 ? 1/T : 0;
	int used_for_curve_faces = max(0, 6*(num_curve_points-1));
    int used_for_num_curve_points = 3*num_curve_points;
    int used_for_curve_colors = 4*num_curve_points;
	create_gl_structures(used_for_curve_faces, used_for_num_curve_points, used_for_curve_colors, 1, num_curve_points, create_curve, glm::vec3(0.8, 0.2, 0.4));
}

void revolution() {
	int np = points.size();
	int num_curve_points = np >= 2 ? 1/T : 0;
	int used_for_surface_faces = max(0, 6*REVOLUTION_STEPS*(num_curve_points-1));
    int used_for_surface_points = 3*REVOLUTION_STEPS*num_curve_points;
    int used_for_surface_colors = 4*REVOLUTION_STEPS*num_curve_points;
	create_gl_structures(used_for_surface_faces, used_for_surface_points, used_for_surface_colors, REVOLUTION_STEPS, num_curve_points, create_curve, glm::vec3(0.8, 0.2, 0.4));
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

void draw(GLenum primitive, GLuint shader) {

int attrV, attrC, attrN; 
	
	glBindBuffer(GL_ARRAY_BUFFER, axisVBO[0]);
	attrV = glGetAttribLocation(shader, "aPosition");
	glVertexAttribPointer(attrV, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrV);


	if(shader == phongShader) {
		glBindBuffer(GL_ARRAY_BUFFER, axisVBO[3]);
		attrN = glGetAttribLocation(shader, "aNormal");
		glVertexAttribPointer(attrN, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(attrN);

		glBindBuffer(GL_ARRAY_BUFFER, axisVBO[4]);
		attrN = glGetAttribLocation(shader, "aTexCoord");
		glVertexAttribPointer(attrN, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(attrN);

	}

	glBindBuffer(GL_ARRAY_BUFFER, axisVBO[1]);
	attrC = glGetAttribLocation(shader, "aColor");
	glVertexAttribPointer(attrC, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrC);

	int poly_loc = glGetUniformLocation( phongShader, "uPoly" );
	int fill_loc = glGetUniformLocation( shader, "uFill" );

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);
	if(mode & MODE_REVOLUTION && show & SHOW_T) {
		glUniform1i(fill_loc, 0);	
		glUniform1f(poly_loc, 1.f);
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glDrawElements(primitive, total_faces, GL_UNSIGNED_INT, 0);
	} else {
		glUniform1i(fill_loc, 1);
		if(mode & MODE_REVOLUTION && show & (SHOW_F | SHOW_T)) {
			glUniform1f(poly_loc, 1.f);
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			glDrawElements(primitive, total_faces, GL_UNSIGNED_INT, 0);
		}
		if(mode & MODE_CURVE || show & SHOW_E) {
			glUniform1f(poly_loc, 0.7f);
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			glDrawElements(primitive, total_faces, GL_UNSIGNED_INT, 0);
		}
		if(mode & MODE_REVOLUTION && show & SHOW_P) {
			glUniform1f(poly_loc, 0.3f);
			glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
			glDrawElements(primitive, total_faces, GL_UNSIGNED_INT, 0);
		}
	}

	glDisableVertexAttribArray(attrV);
	glDisableVertexAttribArray(attrN);
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
						glutPostRedisplay();
						break;

		case 'R'	:
		case 'r'	:
						mode = mode^MODE_CURVE^MODE_REVOLUTION;
						glutPostRedisplay();
						break;

		case 'X'	:
		case 'x'	:
						revolution_axis = AXIS_X;
						glutPostRedisplay();
						break;

		case 'Y'	:
		case 'y'	:
						revolution_axis = AXIS_Y;
						glutPostRedisplay();
						break;

		case 'F'	:
		case 'f'	:
						show = show^SHOW_F;
						show = show & ~SHOW_T;
						glutPostRedisplay();
						break;

		case 'E'	:
		case 'e'	:
						show = show^SHOW_E;
						glutPostRedisplay();
						break;

		case 'P'	:
		case 'p'	:
						show = show^SHOW_P;
						glutPostRedisplay();
						break;

		case 'T'	:
		case 't'	:
						show = show^SHOW_T;
						show = show & ~SHOW_F;
						glutPostRedisplay();
						break;
		}
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

point2D get_real_coords(double x, double y) {
	//cout << x/(double)winWdth*20-10 << " " << -(y/(double)winHeight*20-10) << endl;
	return { x/(double)winWdth*20-10, -(y/(double)winHeight*20-10) };
}

void close_double_click(int value) {
	waiting_double_click = false;
}

void handle_initialization(int button, int x, int y) {
	point2D real_coords = get_real_coords(x, y);
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
			} else if(mode & MODE_REVOLUTION) {
				if(full_screen & FULL_UPPER_RIGHT || (x > winWdth/2 && y < winHeight/2 && !full_screen)) {
					event_type = EVENT_MOVE_CAMERA;
					camera_move_point = point2D(x, y);
				} else
					event_type = EVENT_INVALID;
				
				if(!waiting_double_click) {
					waiting_double_click = true;
					first_click = true;
    				glutTimerFunc(250, close_double_click, 1);
    			}
			}
		break;
		default:
			event_type = EVENT_INVALID;
		break;
	}
}

void handle_closing(int button, int x, int y) {
	point2D real_coords = get_real_coords(x, y);
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
				case EVENT_MOVE_CAMERA:
					//cout << (y-camera_move_point.Y)/100 << endl;
					//cout << (x-camera_move_point.X)/100 << endl;

					//camera_angle_y += x-camera_move_point.X;
					//camera_angle_x += y-camera_move_point.Y;
					//while((abs(camera_angle_y) > 180)) camera_angle_y += -(camera_angle_y/abs(camera_angle_y))*360;
					//while((abs(camera_angle_x) > 180)) camera_angle_x += -(camera_angle_x/abs(camera_angle_x))*360;

    				Model4 *= glm::rotate(glm::mat4(), (glm::mediump_float)(y-camera_move_point.Y)/100, glm::vec3(1, 0, 0));
    				Model4 *= glm::rotate(glm::mat4(), (glm::mediump_float)(x-camera_move_point.X)/100, glm::vec3(0, 1, 0));
				
    			default:
    				if(!first_click) {
    					if(waiting_double_click) {
    						waiting_double_click = false;
    						if(full_screen & FULL_BOTTOM_LEFT || (x < winWdth/2 && y > winHeight/2 && !full_screen))
    							full_screen ^= FULL_BOTTOM_LEFT;
    						else if(full_screen & FULL_UPPER_LEFT || (x < winWdth/2 && y < winHeight/2 && !full_screen))
    							full_screen ^= FULL_UPPER_LEFT;
    						else if(full_screen & FULL_BOTTOM_RIGHT || (x > winWdth/2 && y > winHeight/2 && !full_screen))
    							full_screen ^= FULL_BOTTOM_RIGHT;
    						else if(full_screen & FULL_UPPER_RIGHT || (x > winWdth/2 && y < winHeight/2 && !full_screen))
    							full_screen ^= FULL_UPPER_RIGHT;
    					}
    				} else
    					first_click = false;
				break;

			}
		break;
	}
}

void mouse(int button, int state, int x, int y) {
	if(!state)
		handle_initialization(button, x, y);
	else
		handle_closing(button, x, y);
	glutPostRedisplay();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void display(void) { 
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	//PROJECTION
	glm::mat4 orthogonal = glm::ortho(	-10.0, 10.0, -10.0, 10.0, -10.0, 10.0);
	glm::mat4 perspective = glm::perspective(9.0f, 1.0f, 0.1f, 30.0f);
    
    //VIEW
    glm::mat4 View = glm::mat4(1.);
    //View = glm::translate(View, glm::vec3(2.0f,4.0f, -25.0f));

    //MODEL
    glm::mat4 Model1 = glm::rotate(glm::mat4(1.0), 0.f, glm::vec3(1, 0, 0));
    glm::mat4 Model2 = glm::rotate(glm::mat4(1.0), -1.58f, glm::vec3((revolution_axis == AXIS_Y), (revolution_axis == AXIS_X), 0));
    glm::mat4 Model3 = glm::rotate(glm::mat4(1.0), 1.58f, glm::vec3((revolution_axis == AXIS_Y), (revolution_axis == AXIS_X), 0));
    // glm::mat4 Model4 = glm::rotate(glm::mat4(1.0), -1.f, glm::vec3(1, 1, 0));

    //glm::mat4 MVP1 = orthogonal * View * Model1;
    //glm::mat4 MVP2 = orthogonal * View * Model2;
    //glm::mat4 MVP3 = orthogonal * View * Model3;
    
    //double angle_y = camera_angle_y*PI/180;
    //double angle_x = camera_angle_x*PI/180;
    //glm::mat4 MVP4 = perspective * View * Model4;

    if (drawRef) {
    	
		if(mode & MODE_CURVE) {

			glUseProgram(axisShader);
			int projection_loc = glGetUniformLocation( axisShader, "uProjectionMatrix" );
			int view_loc = glGetUniformLocation( axisShader, "uViewMatrix" );
			int model_loc = glGetUniformLocation( axisShader, "uModelMatrix" );
			int normalMatrix_loc = glGetUniformLocation( axisShader, "uNormalMatrix" );
			glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(orthogonal));
			glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(View));
			
			glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(Model1));
			glViewport(0, 0, winWdth, winHeight);

			curve_controls();
    		draw(GL_LINE_STRIP, axisShader);
			
			curve();
	    	draw(GL_LINE_STRIP, axisShader);

			axis();
    		draw(GL_LINE_STRIP, axisShader);

		} else {
			glUseProgram(phongShader);
			int projection_loc = glGetUniformLocation( phongShader, "uProjectionMatrix" );
			int view_loc = glGetUniformLocation( phongShader, "uViewMatrix" );
			int model_loc = glGetUniformLocation( phongShader, "uModelMatrix" );
			int normalMatrix_loc = glGetUniformLocation( phongShader, "uNormalMatrix" );
			int texture_loc = glGetUniformLocation( phongShader, "Noise" );
			glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(orthogonal));
			glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(View));
			revolution();

			int light_loc = glGetUniformLocation(phongShader, "uLPos");
			glUniform3fv(light_loc, 1, glm::value_ptr(light_pos));

			light_loc = glGetUniformLocation(phongShader, "uLColor");
			glUniform3fv(light_loc, 1, glm::value_ptr(light_color));
		
			if(!full_screen || full_screen & FULL_BOTTOM_LEFT) {
				if(!full_screen)
					glViewport(0, 0, winWdth/2, winHeight/2);
				else
					glViewport(0, 0, winWdth, winHeight);

				glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(Model1));
				glUniformMatrix3fv(normalMatrix_loc, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(Model1*View)))));
    			draw(GL_TRIANGLES, phongShader);
    		}

			if(!full_screen || full_screen & FULL_BOTTOM_RIGHT) {
				if(!full_screen)
	    			glViewport(winWdth/2, 0, winWdth/2, winHeight/2);
				else
					glViewport(0, 0, winWdth, winHeight);

				glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(Model2));
				glUniformMatrix3fv(normalMatrix_loc, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(Model2*View)))));
	    		draw(GL_TRIANGLES, phongShader);
	    	}

	    	if(!full_screen || full_screen & FULL_UPPER_LEFT) {
				if(!full_screen)
		    		glViewport(0, winHeight/2, winWdth/2, winHeight/2);
				else
					glViewport(0, 0, winWdth, winHeight);
				
				glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(Model3));
				glUniformMatrix3fv(normalMatrix_loc, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(Model3*View)))));
	    		draw(GL_TRIANGLES, phongShader);
	    	}

	    	if(!full_screen || full_screen & FULL_UPPER_RIGHT) {
				if(!full_screen)
		    		glViewport(winWdth/2, winHeight/2, winWdth/2, winHeight/2);
				else
					glViewport(0, 0, winWdth, winHeight);
				
				glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(perspective));
				glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(Model4));
				glUniformMatrix3fv(normalMatrix_loc, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(Model4*View)))));
	    		draw(GL_TRIANGLES, phongShader);
    		}

    		glUseProgram(axisShader);
			projection_loc = glGetUniformLocation( axisShader, "uProjectionMatrix" );
			view_loc = glGetUniformLocation( axisShader, "uViewMatrix" );
			model_loc = glGetUniformLocation( axisShader, "uModelMatrix" );
			normalMatrix_loc = glGetUniformLocation( axisShader, "uNormalMatrix" );
			glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(orthogonal));
			glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(View));

			axis();

			if(!full_screen || full_screen & FULL_BOTTOM_LEFT) {
				if(!full_screen)
					glViewport(0, 0, winWdth/2, winHeight/2);
				else
					glViewport(0, 0, winWdth, winHeight);

				glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(Model1));
    			draw(GL_LINE_STRIP, axisShader);
    		}

			if(!full_screen || full_screen & FULL_BOTTOM_RIGHT) {
				if(!full_screen)
	    			glViewport(winWdth/2, 0, winWdth/2, winHeight/2);
				else
					glViewport(0, 0, winWdth, winHeight);

				glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(Model2));
	    		draw(GL_LINE_STRIP, axisShader);
	    	}

	    	if(!full_screen || full_screen & FULL_UPPER_LEFT) {
				if(!full_screen)
		    		glViewport(0, winHeight/2, winWdth/2, winHeight/2);
				else
					glViewport(0, 0, winWdth, winHeight);
				
				glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(Model3));
	    		draw(GL_LINE_STRIP, axisShader);
	    	}

	    	if(!full_screen || full_screen & FULL_UPPER_RIGHT) {
				if(!full_screen)
		    		glViewport(winWdth/2, winHeight/2, winWdth/2, winHeight/2);
				else
					glViewport(0, 0, winWdth, winHeight);
				
				glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(perspective));
				glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(Model4));
	    		draw(GL_LINE_STRIP, axisShader);
    		}
		}
    }
	
	glutSwapBuffers();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void initGL(void) {

	glClearColor(0.0, 0.0, 0.0, 0.0);	
	glPointSize(2.0);
	
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
    phongShader = InitShader( "phongShader.vert", "phongShader.frag" );
}

/* ************************************************************************* */
/* ************************************************************************* */
/* *****                                                               ***** */
/* ************************************************************************* */
/* ************************************************************************* */

int main(int argc, char *argv[]) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow("Curve Revolution");
	glutReshapeWindow(winWdth, winHeight);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);


	initGL();

	LoadTexture("wood-texture.png");

	glGenBuffers(5, axisVBO);
	
	initShaders();
	
	glutMainLoop();

	return(0);
}
