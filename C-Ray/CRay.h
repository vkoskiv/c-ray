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

//Image dimensions. Eventually get this from the input file
#define kImgWidth 1920
#define kImgHeight 1080
#define kFrameCount 1
#define bounces 2
#define contrast 1.0

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

//material
typedef struct {
	color diffuse;
	float reflectivity;
}material;

//Light source
typedef struct {
	vector pos;
	float radius;
	color intensity;
}lightSource;

//World
typedef struct {
	color *ambientColor;
	lightSource *lights;
	material *materials;
	sphereObject *spheres;
	polygonObject *polys;
	
	int height, width;
	
	int sphereAmount;
	int polygonAmount;
	int materialAmount;
	int lightAmount;
}world;

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

bool rayIntersectsWithLight(lightRay *ray, lightSource *light, double *t) {
	bool intersects = false;
	
	//Vector dot product of the direction
	float A = scalarProduct(&ray->direction, &ray->direction);
	//Distance between start of a bounced ray and the light pos
	vector distance = subtractVectors(&ray->start, &light->pos);
	float B = 2 * scalarProduct(&ray->direction, &distance);
	float C = scalarProduct(&distance, &distance) - (light->radius * light->radius);
	float trigDiscriminant = B * B - 4 * A * C;
	
	//If trigDiscriminant is negative, ray has missed the lightSource
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
		if ((t0 > 0.001f) && (t0 < *t)) {
			*t = t0;
			intersects = true;
		} else {
			intersects = false;
		}
	}
	return intersects;
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

float randRange(float a, float b)
{
	return ((b-a)*((float)rand()/RAND_MAX))+a;
}

//TODO: Turn this into a proper tokenizer
int buildScene(bool randomGenerator, world *scene) {
	if (!randomGenerator) {
		
		scene->ambientColor = (color * )calloc(3, sizeof(color));
		scene->ambientColor->red =   0.41;
		scene->ambientColor->green = 0.41;
		scene->ambientColor->blue =  0.41;
		
		//define materials to an array
		scene->materialAmount = 6;
		
		scene->materials = (material *)calloc(scene->materialAmount, sizeof(material));
		scene->materials[0].diffuse.red = 1;
		scene->materials[0].diffuse.green = 0;
		scene->materials[0].diffuse.blue = 0;
		scene->materials[0].reflectivity = 0.2;
		
		scene->materials[1].diffuse.red = 0;
		scene->materials[1].diffuse.green = 1;
		scene->materials[1].diffuse.blue = 0;
		scene->materials[1].reflectivity = 0.5;
		
		scene->materials[2].diffuse.red = 0;
		scene->materials[2].diffuse.green = 0;
		scene->materials[2].diffuse.blue = 1;
		scene->materials[2].reflectivity = 1;
		
		scene->materials[3].diffuse.red = 0.3;
		scene->materials[3].diffuse.green = 0.3;
		scene->materials[3].diffuse.blue = 0.3;
		scene->materials[3].reflectivity = 1;
		
		scene->materials[4].diffuse.red = 0;
		scene->materials[4].diffuse.green = 0.517647;
		scene->materials[4].diffuse.blue = 1;
		scene->materials[4].reflectivity = 1;
		
		scene->materials[5].diffuse.red = 0.3;
		scene->materials[5].diffuse.green = 0.3;
		scene->materials[5].diffuse.blue = 0.3;
		scene->materials[5].reflectivity = 0.5;
		
		//Define polygons to an array
		scene->polygonAmount = 4;
		
		scene->polys = (polygonObject *)calloc(scene->polygonAmount, sizeof(polygonObject));
		//Bottom plane
		scene->polys[0].v1.x = 200;
		scene->polys[0].v1.y = 50;
		scene->polys[0].v1.z = 0;
		
		scene->polys[0].v2.x = kImgWidth-200;
		scene->polys[0].v2.y = 50;
		scene->polys[0].v2.z = 0;
		
		scene->polys[0].v3.x = 400;
		scene->polys[0].v3.y = kImgHeight/2;
		scene->polys[0].v3.z = 2000;
		scene->polys[0].material = 3;
		
		scene->polys[1].v1.x = kImgWidth-200;
		scene->polys[1].v1.y = 50;
		scene->polys[1].v1.z = 0;
		
		scene->polys[1].v2.x = kImgWidth-400;
		scene->polys[1].v2.y = kImgHeight/2;
		scene->polys[1].v2.z = 2000;
		
		scene->polys[1].v3.x = 400;
		scene->polys[1].v3.y = kImgHeight/2;
		scene->polys[1].v3.z = 2000;
		scene->polys[1].material = 3;
		
		//Background plane
		//First poly
		//bottom left
		scene->polys[2].v1.x = 400;
		scene->polys[2].v1.y = kImgHeight/2;
		scene->polys[2].v1.z = 2020;
		//Bottom right
		scene->polys[2].v2.x = kImgWidth-400;
		scene->polys[2].v2.y = kImgHeight/2;
		scene->polys[2].v2.z = 2020;
		//top left
		scene->polys[2].v3.x = 400;
		scene->polys[2].v3.y = kImgHeight;
		scene->polys[2].v3.z = 1500;
		scene->polys[2].material = 5;
		//Second poly
		//top right
		scene->polys[3].v1.x = kImgWidth-400;
		scene->polys[3].v1.y = kImgHeight;
		scene->polys[3].v1.z = 1500;
		//top left
		scene->polys[3].v2.x = 400;
		scene->polys[3].v2.y = kImgHeight;
		scene->polys[3].v2.z = 1500;
		//bottom right
		scene->polys[3].v3.x = kImgWidth-400;
		scene->polys[3].v3.y = kImgHeight/2;
		scene->polys[3].v3.z = 2020;
		scene->polys[3].material = 5;
		
		//define spheres to an array
		//Red sphere
		scene->sphereAmount = 4;
		
		scene->spheres = (sphereObject *)calloc(scene->sphereAmount, sizeof(sphereObject));
		scene->spheres[0].pos.x = 400;
		scene->spheres[0].pos.y = 260;
		scene->spheres[0].pos.z = 0;
		scene->spheres[0].radius = 200;
		scene->spheres[0].material = 0;
		
		//green sphere
		scene->spheres[1].pos.x = 650;
		scene->spheres[1].pos.y = 630;
		scene->spheres[1].pos.z = 1750;
		scene->spheres[1].radius = 150;
		scene->spheres[1].material = 1;
		
		//blue sphere
		scene->spheres[2].pos.x = 1300;
		scene->spheres[2].pos.y = 520;
		scene->spheres[2].pos.z = 1000;
		scene->spheres[2].radius = 220;
		scene->spheres[2].material = 2;
		
		//grey sphere
		scene->spheres[3].pos.x = 970;
		scene->spheres[3].pos.y = 250;
		scene->spheres[3].pos.z = 400;
		scene->spheres[3].radius = 100;
		scene->spheres[3].material = 3;
		
		//Define lights to an array
		scene->lightAmount = 1;
		
		scene->lights = (lightSource *)calloc(scene->lightAmount, sizeof(lightSource));
		scene->lights[0].pos.x = kImgWidth/2;
		scene->lights[0].pos.y = 1080-100;
		scene->lights[0].pos.z = 0;
		scene->lights[0].intensity.red = 0.9;
		scene->lights[0].intensity.green = 0.9;
		scene->lights[0].intensity.blue = 0.9;
		scene->lights[0].radius = 1.0;
		
		/*lights[1].pos.x = 1280;
		 lights[1].pos.y = 3000;
		 lights[1].pos.z = -1000;
		 lights[1].intensity.red = 0.4;
		 lights[1].intensity.green = 0.4;
		 lights[1].intensity.blue = 0.4;
		 
		 lights[2].pos.x = 1600;
		 lights[2].pos.y = 1000;
		 lights[2].pos.z = -1000;
		 lights[2].intensity.red = 0.4;
		 lights[2].intensity.green = 0.4;
		 lights[2].intensity.blue = 0.4;*/
		
		return 0;
	} else {
		//Define random scene
		int amount = rand()%50+50;
		printf("Spheres: %i\n",amount);
		int i;
		
		for (i = 0; i < amount; i++) {
			scene->materials[i].diffuse.red = randRange(0,1);
			scene->materials[i].diffuse.green = randRange(0,1);
			scene->materials[i].diffuse.blue = randRange(0,1);
			scene->materials[i].reflectivity = randRange(0,1);
			
			scene->spheres[i].pos.x = rand()%kImgWidth;
			scene->spheres[i].pos.y = rand()%kImgWidth;
			scene->spheres[i].pos.z = rand()&4000-2000;
			scene->spheres[i].radius = rand()%100+50;
			scene->spheres[i].material = i;
		}
		return 0;
	}
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

float FastInvSqrt(float x) {
	float xhalf = 0.5f * x;
	int i = *(int*)&x;         // evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);  // what the fuck?
	x = *(float*)&i;
	x = x*(1.5f-(xhalf*x*x));
	return x;
}

#endif /* defined(____CRay__) */
