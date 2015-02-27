//
//  CRay.h
//  
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//

#ifndef ____CRay__
#define ____CRay__

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h> //Need this for boolean data type
#include <math.h>
#include <string.h>

//Vector
typedef struct {
	float x, y, z;
}vector;

//Object type
typedef struct {
	int type;
}type;

//Object, a sphere in this case
typedef struct {
	vector pos;
	type objectType;
	float radius;
	int material;
}sphereObject;

typedef struct {
	vector v1, v2, v3; //Three vertices
	int material;
}polygonObject;

typedef struct { //Maybe use two polygons for this
	vector v1, v2, v3, v4; //Four vertices
	int material;
}plane;

//Object, a cube (WIP)
typedef struct {
	vector pos;
	type objectType;
	float edgeLength;
	int material;
}cubeObject;

//Simulated light ray
typedef struct {
	vector start;
	vector direction;
}lightRay;

//Color
typedef struct {
	float red, green, blue;
}color;

//World
typedef struct {
	color ambientColor;
}world;

//material
typedef struct {
	color diffuse;
	float reflectivity;
}material;

//Light source
typedef struct {
	vector pos;
	color intensity;
}lightSource;

/* Vector Functions */

//Add two vectors and return the resulting vector
vector addVectors(vector *v1, vector *v2) {
	vector result = {v1->x + v2->x, v1->y + v2->y, v1->z + v2->z};
	return result;
}

//Subtract two vectors and return the resulting vector
vector subtractVectors(vector *v1, vector *v2) {
	vector result = {v1->x - v2->x, v1->y - v2->y, v1->z - v2->z };
	return result;
}

//Multiply two vectors and return the dot product
float scalarProduct(vector *v1, vector *v2) {
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

//Multiply a vector by a scalar and return the resulting vector
vector vectorScale(double c, vector *v) {
	vector result = {v->x * c, v->y * c, v->z * c};
	return result;
}

//Calculate cross product and return the resulting vector
vector vectorCross(vector *v1, vector *v2) {
	vector result;
	
	result.x = (v1->y * v2->z) - (v1->z * v2->y);
	result.y = (v1->z * v2->x) - (v1->x * v2->z);
	result.z = (v1->x * v2->y) - (v1->y * v2->x);
	
	return result;
}

//Color functions
//Multiply two colors
color multiplyColors(color *c1, color *c2) {
	color result = {c1->red * c2->red, c1->green * c2->green, c1->blue * c2->blue};
	return result;
}

//Add two colors
color addColors(color *c1, color *c2) {
	color result = {c1->red + c2->red, c1->green + c2->green, c1->blue + c2->blue};
	return result;
}

//Multiply a color with a coefficient value
color colorCoef(double coef, color *c) {
	color result = {c->red * coef, c->green * coef, c->blue * coef};
	return result;
}

//Calculates intersection with a sphere and a light ray
bool rayIntersectsWithSphere(lightRay *ray, sphereObject *sphere, double *t) {
	bool intersects = false;
	
	//Vector dot product of the direction
	float A = scalarProduct(&ray->direction, &ray->direction);
	
	//Distance between start of a lightRay and the sphere position
	vector distance = subtractVectors(&ray->start, &sphere->pos);
	
	float B = 2 * scalarProduct(&ray->direction, &distance);
	
	float C = scalarProduct(&distance, &distance) - (sphere->radius * sphere->radius);
	
	float trigDiscriminant = B * B - 4 * A * C;
	
	//If discriminant is negative, no real roots and the ray has missed the sphere
	if (trigDiscriminant < 0) {
		intersects = false;
	} else {
		float sqrtOfDiscriminant = sqrtf(trigDiscriminant);
		float t0 = (-B + sqrtOfDiscriminant)/(2);
		float t1 = (-B - sqrtOfDiscriminant)/(2);
		
		//Pick closest intersection
		if (t0 > t1) {
			t0 = t1;
		}
		
		//Verify intersection is larger than 0 and less than the original distance
		if ((t0 > 0.001f) && (t0 < *t)) {
			*t = t0;
			intersects = true;
		} else {
			intersects = false;
		}
	}
	return intersects;
}

bool rayIntersectsWithPolygon(lightRay *r, polygonObject *t, double *result, vector *normal) {
	double det, invdet;
	vector edge1 = subtractVectors(&t->v2, &t->v1);
	vector edge2 = subtractVectors(&t->v3, &t->v1);
	
	//Find the cross product of edge 2 and the current ray direction
	vector s1 = vectorCross(&r->direction, &edge2);
	
	det = scalarProduct(&edge1, &s1);
	//Prepare for floating point precision errors, find a better way to fix these!
	if (det > -0.000001 && det < 0.000001) {
		return false;
	}
	
	invdet = 1/det;
	
	vector s2 = subtractVectors(&r->start, &t->v1);
	double u = scalarProduct(&s2, &s1) * invdet;
	if (u < 0 || u > 1) {
		return false;
	}
	
	vector s3 = vectorCross(&s2, &edge1);
	double v = scalarProduct(&r->direction, &s3) * invdet;
	if (v < 0 || (u+v) > 1) {
		return false;
	}
	
	double temp = scalarProduct(&edge2, &s3) * invdet;
	
	if (((temp < 0) || (temp > *result))) {
		return false;
	}
		
		*result = temp - 0.002; //This is to fix floating point precision error artifacts
		*normal = vectorCross(&edge2, &edge1);
		
		return true;
}

void saveImageFromArray(char *filename, unsigned char *imgdata, int width, int height) {
	//File pointer
	FILE *f;
	//Open the file
	f = fopen(filename, "w");
	//Write the PPM format header info
	fprintf(f, "P6 %d %d %d\n", width, height, 255);
	//Write given image data to the file, 3 bytes/pixel
	fwrite(imgdata, 3, width*height, f);
	//Close the file
	fclose(f);
}

void saveBmpFromArray(char *filename, unsigned char *imgData, int width, int height) {
	int i;
	int error;
	FILE *f;
	int filesize = 54 + 3*width*height;
	
	unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
	unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
	unsigned char bmppadding[3] = {0,0,0};
	
	//Create header with filesize data
	bmpfileheader[2] = (unsigned char)(filesize    );
	bmpfileheader[3] = (unsigned char)(filesize>> 8);
	bmpfileheader[4] = (unsigned char)(filesize>>16);
	bmpfileheader[5] = (unsigned char)(filesize>>24);
	
	//create header width and height info
	bmpinfoheader[ 4] = (unsigned char)(width    );
	bmpinfoheader[ 5] = (unsigned char)(width>>8 );
	bmpinfoheader[ 6] = (unsigned char)(width>>16);
	bmpinfoheader[ 7] = (unsigned char)(width>>24);
	
	bmpinfoheader[ 8] = (unsigned char)(height    );
	bmpinfoheader[ 9] = (unsigned char)(height>>8 );
	bmpinfoheader[10] = (unsigned char)(height>>16);
	bmpinfoheader[11] = (unsigned char)(height>>24);
	
	f = fopen(filename,"wb");
	error = (unsigned int)fwrite(bmpfileheader,1,14,f);
	if (error != 14) {
		printf("Error writing BMP file header data\n");
	}
	error = (unsigned int)fwrite(bmpinfoheader,1,40,f);
	if (error != 40) {
		printf("Error writing BMP info header data\n");
	}
	
	for (i = 0; i < height; i++) {
		error = (unsigned int)fwrite(imgData+(width*(i)*3),3,width,f);
		if (error != width) {
			printf("Error writing image line to BMP\n");
		}
		error = (unsigned int)fwrite(bmppadding,1,(4-(width*3)%4)%4,f);
		if (error != (4-(width*3)%4)%4) {
			printf("Error writing BMP padding data\n");
		}
	}
	fclose(f);
}

float randRange(float a, float b)
{
	return ((b-a)*((float)rand()/RAND_MAX))+a;
}

float FastInvSqrt(float x) {
	float xhalf = 0.5f * x;
	int i = *(int*)&x;         // evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);  // what the fuck?
	x = *(float*)&i;
	x = x*(1.5f-(xhalf*x*x));
	return x;
}

#endif /* defined(____CRay__) */
