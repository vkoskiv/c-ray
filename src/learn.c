//
//  learn.c
//  C-Ray
//
//  Created by Valtteri on 16/10/2018.
//  Copyright © 2018 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "Tinn.h"
#include "tile.h"
#include "cJSON.h"
#include "logging.h"
#include "filehandler.h"

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
	char *buf = loadFile(filePath);
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

struct model *loadModel(char *filePath) {
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
	}
	return model;
}

void saveModel(struct model *model, char *filePath) {
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
	
	FILE *fp = fopen(filePath, "ab");
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

void tileToTrainingData(struct renderTile *tile) {
	//Loop thru tile, grab pixels, convert to floats and shove into training dataset
}

void study() {
	struct model *m = loadModel("./models/model.json");
	
	struct image *low = loadImage("./models/examples/0low.png");
	struct renderTile *lowTiles;
	
	struct image *high = loadImage("./models/examples/0.png");
	struct renderTile *highTiles;
	
}

//To be used while rendering
void learnTile(struct renderer *r, struct renderTile *tile) {
	/*
	 Get rendered data from current render
	 Then get rendered data from example file
	 Then run training iteration
	 */
}
