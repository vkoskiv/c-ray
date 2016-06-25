//
//  filehandler.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "filehandler.h"

//Prototypes for internal functions
int getFileSize(char *fileName);

void saveBmpFromArray(const char *filename, world *worldScene) {
	int i;
	int error;
	FILE *f;
	int filesize = 54 + 3*worldScene->camera->width*worldScene->camera->height;
	
	unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
	unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
	unsigned char bmppadding[3] = {0,0,0};
	
	//Create header with filesize data
	bmpfileheader[2] = (unsigned char)(filesize    );
	bmpfileheader[3] = (unsigned char)(filesize>> 8);
	bmpfileheader[4] = (unsigned char)(filesize>>16);
	bmpfileheader[5] = (unsigned char)(filesize>>24);
	
	//create header width and height info
	bmpinfoheader[ 4] = (unsigned char)(worldScene->camera->width    );
	bmpinfoheader[ 5] = (unsigned char)(worldScene->camera->width>>8 );
	bmpinfoheader[ 6] = (unsigned char)(worldScene->camera->width>>16);
	bmpinfoheader[ 7] = (unsigned char)(worldScene->camera->width>>24);
	
	bmpinfoheader[ 8] = (unsigned char)(worldScene->camera->height    );
	bmpinfoheader[ 9] = (unsigned char)(worldScene->camera->height>>8 );
	bmpinfoheader[10] = (unsigned char)(worldScene->camera->height>>16);
	bmpinfoheader[11] = (unsigned char)(worldScene->camera->height>>24);
	
	f = fopen(filename,"wb");
	error = (unsigned int)fwrite(bmpfileheader,1,14,f);
	if (error != 14) {
		printf("Error writing BMP file header data\n");
	}
	error = (unsigned int)fwrite(bmpinfoheader,1,40,f);
	if (error != 40) {
		printf("Error writing BMP info header data\n");
	}
	
	for (i = 0; i < worldScene->camera->height; i++) {
		error = (unsigned int)fwrite(worldScene->camera->imgData+(worldScene->camera->width*(i)*3),3,worldScene->camera->width,f);
		if (error != worldScene->camera->width) {
			printf("Error writing image line to BMP\n");
		}
		error = (unsigned int)fwrite(bmppadding,1,(4-(worldScene->camera->width*3)%4)%4,f);
		if (error != (4-(worldScene->camera->width*3)%4)%4) {
			printf("Error writing BMP padding data\n");
		}
	}
	fclose(f);
}

void encodePNGFromArray(const char *filename, world *worldScene) {
	//C-Ray saves image data into the matrix top-down, PNG renders it down-up, so we flip each
	//vertical line before encoding it to file.
	unsigned char *flippedData = NULL;
    flippedData = (unsigned char*)malloc(4 * worldScene->camera->width * worldScene->camera->height);
    memset(flippedData, 0, 4 * worldScene->camera->width	* worldScene->camera->height);
    
    int fy = worldScene->camera->height;
    for (int y = 0; y < worldScene->camera->height; y++) {
        if (fy > 0) fy--;
        for (int x = 0; x < worldScene->camera->width; x++) {
            //Note, PNG is big-endian, so we flip the color byte values (...*3+ X)
            flippedData[(x + fy*worldScene->camera->width)*3 + 2] = worldScene->camera->imgData[(x + y*worldScene->camera->width)*3 + 0];
            flippedData[(x + fy*worldScene->camera->width)*3 + 1] = worldScene->camera->imgData[(x + y*worldScene->camera->width)*3 + 1];
            flippedData[(x + fy*worldScene->camera->width)*3 + 0] = worldScene->camera->imgData[(x + y*worldScene->camera->width)*3 + 2];
        }
    }
    unsigned error = lodepng_encode24_file(filename, flippedData, worldScene->camera->width, worldScene->camera->height);
    free(flippedData);
    if (error) printf("error %u: %s\n", error, lodepng_error_text(error));
}

void printFileSize(char *fileName) {
	//We determine the file size after saving, because the lodePNG library doesn't have a way to tell the compressed file size
	//This will work for all three image formats
	long bytes, kilobytes, megabytes, gigabytes, terabytes; // <- Futureproofing?!
	bytes = getFileSize(fileName);
	if (fileName) free(fileName);
	kilobytes = bytes / 1000;
	megabytes = kilobytes / 1000;
	gigabytes = megabytes / 1000;
	terabytes = gigabytes / 1000;
	
	if (gigabytes > 1000) {
		printf("Wrote %ldTB to file.\n", terabytes);
	} else if (megabytes > 1000) {
		printf("Wrote %ldGB to file.\n", gigabytes);
	} else if (kilobytes > 1000) {
		printf("Wrote %ldMB to file.\n", megabytes);
	} else if (bytes > 1000) {
		printf("Wrote %ldKB to file.\n", kilobytes);
	} else {
		printf("Wrote %ldB to file.\n", bytes);
	}
	
}

void writeImage(world *worldScene) {
	//Save image data to a file
	int bufSize;
	if (worldScene->camera->currentFrame < 100) {
		bufSize = 26;
	} else if (worldScene->camera->currentFrame < 1000) {
		bufSize = 27;
	} else {
		bufSize = 28;
	}
	char *buf = (char*)calloc(sizeof(char), bufSize);
	
	if (worldScene->camera->fileType == bmp){
		sprintf(buf, "../output/rendered_%d.bmp", worldScene->camera->currentFrame);
		printf("Saving result in \"%s\"\n", buf);
		saveBmpFromArray(buf, worldScene);
	} else  if (worldScene->camera->fileType == png){
		sprintf(buf, "../output/rendered_%d.png", worldScene->camera->currentFrame);
		printf("Saving result in \"%s\"\n", buf);
		encodePNGFromArray(buf, worldScene);
	}
	printFileSize(buf);
}

int getFileSize(char *fileName) {
	FILE *file;
	file = fopen(fileName, "r");
	if (!file) return 0;
	fseek(file, 0L, SEEK_END);
	int size = (int)ftell(file);
	fclose(file);
	return size;
}
