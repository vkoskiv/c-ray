//
//  learn.c
//  C-Ray
//
//  Created by Valtteri on 16/10/2018.
//  Copyright © 2018 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "../libraries/Tinn.h"
#include "../datatypes/tile.h"
#include "../libraries/cJSON.h"
#include "../utils/logging.h"
#include "../utils/filehandler.h"
#include "../datatypes/texture.h"
#include "../utils/loaders/textureloader.h"

#include "learn.h"

void createModel(char *filePath);

/*
 TODO:
 - Render a bunch of CLEAN high-sample images and pair them with their JSONs
 - Load reference image from file
 - Training mode
 - Model loading and saving
 - Channel splitting
 - Prediction combining
 
 Feed in tile after rendering it, and LEARN!
 
 For the denoiser bit, we just load up the models, predict and combine to get result
 
 */

//Load metadata for model from JSON file
int loadMetaData(struct model *model, char *filePath) {
	size_t bytes = 0;
	char *buf = loadFile(filePath, &bytes);
	logr(info, "%zi bytes of input loaded from file, parsing.\n", bytes);
	cJSON *json = cJSON_Parse(buf);
	free(buf);
	if (json == NULL) {
		logr(warning, "Failed to parse JSON\n");
		const char *errptr = cJSON_GetErrorPtr();
		if (errptr != NULL) {
			free(buf);
			cJSON_Delete(json);
			logr(warning, "Error before: %s\n", errptr);
			return -2;
		}
	}
	
	const cJSON *trainIterations = NULL;
	const cJSON *sessions = NULL;
	const cJSON *modelPath = NULL;
	const cJSON *nips = NULL;
	const cJSON *nhid = NULL;
	const cJSON *nops = NULL;
	
	trainIterations = cJSON_GetObjectItem(json, "iterations");
	if (cJSON_IsNumber(trainIterations)) {
		if (trainIterations->valueint > 0) {
			model->trainIterations = trainIterations->valueint;
		} else {
			model->trainIterations = 0;
		}
	} else {
		logr(warning, "Invalid trainIterations value in model JSON\n");
		return -1;
	}
	
	sessions = cJSON_GetObjectItem(json, "sessions");
	if (cJSON_IsNumber(sessions)) {
		if (sessions->valueint > 0) {
			model->sessions = sessions->valueint;
		} else {
			model->sessions = 0;
		}
	} else {
		logr(warning, "Invalid sessions value in model JSON\n");
		return -1;
	}
	
	modelPath = cJSON_GetObjectItem(json, "modelPath");
	if (cJSON_IsString(modelPath)) {
		copyString(modelPath->valuestring, &model->modelPath);
	} else {
		logr(warning, "Invalid modelPath value in model JSON\n");
		return -1;
	}
	
	nips = cJSON_GetObjectItem(json, "nips");
	if (cJSON_IsNumber(nips)) {
		if (nips->valueint > 0) {
			model->nips = nips->valueint;
		} else {
			model->nips = 0;
		}
	} else {
		logr(warning, "Invalid nips value in model JSON\n");
		return -1;
	}
	
	nops = cJSON_GetObjectItem(json, "nops");
	if (cJSON_IsNumber(nops)) {
		if (nops->valueint > 0) {
			model->nops = nops->valueint;
		} else {
			model->nops = 0;
		}
	} else {
		logr(warning, "Invalid nops value in model JSON\n");
		return -1;
	}
	
	nhid = cJSON_GetObjectItem(json, "nhid");
	if (cJSON_IsNumber(nhid)) {
		if (nhid->valueint > 0) {
			model->nhid = nhid->valueint;
		} else {
			model->nhid = 0;
		}
	} else {
		logr(warning, "Invalid nhid value in model JSON\n");
		return -1;
	}
	
	cJSON_Delete(json);
	
	return 0;
}

int saveMetaData(struct model *model, char *filePath) {
	
	
	
	return 0;
}

/*struct model *loadModel(char *filePath) {
	logr(info, "Loading model\n");
	struct model *model = NULL;
	//If model exists, read it, otherwise create it and read it.
	if (access(filePath, F_OK) == -1) {
		logr(warning, "Model not found at %s! Creating new one there now.\n", filePath);
		createModel(filePath);
		model = loadModel(filePath);
	} else {
		model = malloc(sizeof(struct model));
		//model params from json.
		loadMetaData(model, filePath);
		model->network = xtload(model->modelPath);
		model->network.nips = model->nips;
		model->network.nops = model->nops;
		model->network.nhid = model->nhid;
	}
	return model;
}*/

void saveModel(struct model *model, char *filePath) {
	logr(info, "Saving model\n");
	cJSON *json = cJSON_CreateObject();
	
	cJSON *modelPath = cJSON_CreateString(model->modelPath);
	cJSON_AddItemToObject(json, "modelPath", modelPath);
	
	cJSON *nips = cJSON_CreateNumber(model->nips);
	cJSON_AddItemToObject(json, "nips", nips);
	
	cJSON *nops = cJSON_CreateNumber(model->nops);
	cJSON_AddItemToObject(json, "nops", nops);
	
	cJSON *nhid = cJSON_CreateNumber(model->nhid);
	cJSON_AddItemToObject(json, "nhid", nhid);
	
	cJSON *sessions = cJSON_CreateNumber(model->sessions);
	cJSON_AddItemToObject(json, "sessions", sessions);
	
	cJSON *iterations = cJSON_CreateNumber(model->trainIterations);
	cJSON_AddItemToObject(json, "iterations", iterations);
	
	const char *jsonString = cJSON_Print(json);
	
	FILE *fp = fopen(filePath, "w+");
	if (fp != NULL) {
		fputs(jsonString, fp);
		fclose(fp);
	} else {
		logr(warning, "Failed to open file for writing at %s\n", filePath);
	}
	
	xtsave(model->network, "./models/model.tinn");
	
	cJSON_Delete(json);
	free(model);
}

void createModel(char *filePath) {
	struct model *model = malloc(sizeof(struct model));
	copyString(filePath, &model->modelPath);
	model->network = xtbuild(256, 1024, 256);
	model->nips = 256;
	model->nops = 256;
	model->nhid = 1024;
	model->sessions = 0;
	model->trainIterations = 0;
	saveModel(model, filePath);
}

void allocMatrix(float ****data, int width, int height, int depth) {
	(*data) = (float ***)calloc(depth, sizeof(float ***));
	
	for (int d = 0; d < depth; d++) {
		(*data)[d] = (float **)calloc(width, sizeof(float **));
		for (int r = 0; r < height; r++) {
			(*data)[d][r] = (float*)calloc(height, sizeof(float *));
		}
	}
}

void deallocMatrix(float ****data, int width, int height, int depth) {
	for (int d = 0; d < depth; d++) {
		for (int r = 0; r < height; r++) {
			free((*data)[d][r]);
		}
		free((*data)[d]);
	}
	free((*data));
}

//FIXME: Temporarily here
color getPixelFromBuffer(unsigned char **data, int width, int height, int x, int y) {
	color output = {0.0, 0.0, 0.0, 0.0};
	//TODO: convert unsigned char -> float
	output.red = (float)((*data)[(x + (height - y) * (width))*4 + 0] / 255.0);
	output.green = (float)((*data)[(x + (height - y) * (width))*4 + 1] / 255.0);
	output.blue = (float)((*data)[(x + (height - y) * (width))*4 + 2] / 255.0);
	//output.alpha = (float)((*data)[(x + (height - y) * (width))*4 + 3] / 255.0);
	return output;
}

struct data *loadTrainingData(struct texture *low, struct renderTile *lowTile, struct texture *high, struct renderTile *highTile, int nips, int nops, int nhid) {
	//Loop thru tile, grab pixels, convert to floats and shove into training dataset
	struct data *new = calloc(1, sizeof(struct data));
	
	new->nips = nips;
	new->nops = nops;
	new->source = NULL;
	new->target = NULL;
	
	//3d matrices for training data
	allocMatrix(&new->source, lowTile->width, lowTile->height, 3);
	allocMatrix(&new->target, highTile->width, highTile->height, 3);
	
	//Now load image data for all color channels
	//Source
	for (int x = 0; x < lowTile->width; x++) {
		for (int y = 0; y < lowTile->height; y++) {
			color px = getPixelFromBuffer(&low->byte_data, lowTile->width, lowTile->height, x, y);
			new->source[0][x][y] = (float)px.red;
			new->source[1][x][y] = (float)px.green;
			new->source[2][x][y] = (float)px.blue;
		}
	}
	
	//Target
	for (int x = 0; x < highTile->width; x++) {
		for (int y = 0; y < highTile->height; y++) {
			color px = getPixelFromBuffer(&high->byte_data, highTile->width, highTile->height, x, y);
			new->target[0][x][y] = (float)px.red;
			new->target[1][x][y] = (float)px.green;
			new->target[2][x][y] = (float)px.blue;
		}
	}
	
	return new;
}

//TODO: Consider having an image module, image.c

void freeDataset(struct data *d) {
	deallocMatrix(&d->source, 16, 16, 3);
	deallocMatrix(&d->target, 16, 16, 3);
}

//Convert 2D array of grayscale img data to a 1D array. Append each row in.
float *matrixToArray(float ***data, int width, int height) {
	float *arr = (float *)calloc(width*height, sizeof(float));
	
	//output.red = i->data[(x + (i->size.height - y) * i->size.width)*3 + 0];
	
	/*for (int y = 0; y < height; y++) {
		for (int x = 0; x < height; x++) {
			arr[(x + (height - y) * width)] = (*data)[x][y];
		}
	}*/
	
	int k = 0;
	
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			arr[k++] = (*data)[x][y];
		}
	}
	return arr;
}

void study() {
	struct model *m = NULL;//loadModel("./models/model.json");
	
	struct texture *low = loadTexture("./models/examples/0low.png");
	struct renderTile *lowTiles = NULL;
	int lowTilesCount = quantizeImage(&lowTiles, low, 16, 16);
	
	struct texture *high = loadTexture("./models/examples/0.png");
	struct renderTile *highTiles = NULL;
	int highTilesCount = quantizeImage(&highTiles, high, 16, 16);
	
	//Sanity check. These should be the same
	if (lowTilesCount != highTilesCount) {
		logr(error, "Quantization of images resulted in tile count mismatch. Please verify the source images have matching dimensions\n");
	}
	
	
	struct data *dataset = NULL;
	//Learning params
	float rate = 1.0f;
	const float annealRate = 0.99f;
	
	for (int i = 0; i < lowTilesCount; i++) {
		dataset = loadTrainingData(low, &lowTiles[i], high, &highTiles[i], m->nips, m->nops, m->nhid);
		
		float error = 0.0f;
		
		//Train
		
		for (int channel = 0; channel < 3; channel++) {
			float *lowData = matrixToArray(&dataset->source[channel], 16, 16);
			float *highData = matrixToArray(&dataset->target[channel], 16, 16);
			
			error += xttrain(m->network, lowData, highData, rate);
			m->trainIterations++;
		}
		
		printf("Training error rate: %.12f :: Learning rate %f\n", (float)error/3, (float)rate);
		rate *= annealRate;
		
		freeDataset(dataset);
	}
	
	m->sessions++;
	saveModel(m, "./models/model.json");
	
	freeTexture(low);
	freeTexture(high);

}

//To be used while rendering
void learnTile(struct renderer *r, struct renderTile *tile) {
	/*
	 Get rendered data from current render
	 Then get rendered data from example file
	 Then run training iteration
	 */
}
