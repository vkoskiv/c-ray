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

void saveImageFromArray(const char *filename, const unsigned char *imgdata, unsigned width, unsigned height) {
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

void saveBmpFromArray(const char *filename, const unsigned char *imgData, unsigned width, unsigned height) {
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

void encodePNGFromArray(const char *filename, const unsigned char *imgData, unsigned width, unsigned height) {
    unsigned char *flippedData = NULL;
    flippedData = (unsigned char*)malloc(4 * width * height);
    memset(flippedData, 0, 4 * width * height);
    
    int fy = height;
    for (int y = 0; y < height; y++) {
        if (fy > 0) fy--;
        for (int x = 0; x < width; x++) {
            //Note, PNG is big-endian, so we flip the color byte values (...*3+ X)
            flippedData[(x + fy*width)*3 + 2] = imgData[(x + y*width)*3 + 0];
            flippedData[(x + fy*width)*3 + 1] = imgData[(x + y*width)*3 + 1];
            flippedData[(x + fy*width)*3 + 0] = imgData[(x + y*width)*3 + 2];
        }
    }
    
    unsigned error = lodepng_encode24_file(filename, flippedData, width, height);
    free(flippedData);
    if (error) printf("error %u: %s\n", error, lodepng_error_text(error));
}

char *writeImage(int currentFrame, world *worldScene, unsigned char *imgData) {
	//Save image data to a file
	int bufSize;
	if (currentFrame < 100) {
		bufSize = 26;
	} else if (currentFrame < 1000) {
		bufSize = 27;
	} else {
		bufSize = 28;
	}
	char *buf = (char*)calloc(sizeof(char), bufSize);
	
	if (worldScene->camera.outputFileType == ppm) {
		sprintf(buf, "../output/rendered_%d.ppm", currentFrame);
		printf("Saving result in \"%s\"\n", buf);
		saveImageFromArray(buf, imgData, worldScene->camera.width, worldScene->camera.height);
	} else if (worldScene->camera.outputFileType == bmp){
		sprintf(buf, "../output/rendered_%d.bmp", currentFrame);
		printf("Saving result in \"%s\"\n", buf);
		saveBmpFromArray(buf, imgData, worldScene->camera.width, worldScene->camera.height);
	} else {
		sprintf(buf, "../output/rendered_%d.png", currentFrame);
		printf("Saving result in \"%s\"\n", buf);
		encodePNGFromArray(buf, imgData, worldScene->camera.width, worldScene->camera.height);
	}
	return buf;
}

void printFileSize(char *fileName) {
	//We determine the file size after saving, because the lodePNG library doesn't have a way to tell the compressed file size
	//This will work for all three image formats
	long bytes, kilobytes, megabytes, gigabytes, terabytes; // <- Futureproofing?!
	bytes = getFileSize(fileName);
	free(fileName);
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

int getFileSize(char *fileName) {
	FILE *file;
	file = fopen(fileName, "r");
	if (!file) return 0;
	fseek(file, 0L, SEEK_END);
	int size = (int)ftell(file);
	fclose(file);
	return size;
}
