#ifndef MY_DATA_STRUCTURES
#define MY_DATA_STRUCTURES 1

#include <vector>

using namespace std;
								
typedef struct	{	float* 				vPoint;
					float* 				vNormal;
					float*				vColor;
					float*				vTextCoord;
					unsigned int*		vFace;
				} 	ObjectVA;
								
#endif
