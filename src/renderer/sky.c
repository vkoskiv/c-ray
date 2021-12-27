//
//  sky.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/06/2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "sky.h"

#include "../datatypes/vector.h"
#include "../datatypes/color.h"
#include "../datatypes/lightray.h"

/*
 This implementation here is adapted from the CUDA implementation found at this URL:
 https://github.com/stijnherfst/Tyrant/blob/master/PathTracer/sunsky.cuh
 That page also has more information about the source of this implementation.
 The original code is under the MIT license.
 */

//static const float sunSize = 1.5f;
static const float cutoffAngle = PI / 1.95f;
static const float steepness = 1.5f;
static const float skyFactor = 10.0f;
static const float turbidity = 1.0f;
static const float mieCoefficient = 0.005f;
static const float mieDirectionalG = 0.80f;

static const float rayleighZenithLength = 8.4E3f;
static const float mieZenithLength = 1.25E3f;
static const float sunIntensity = 1000.0f;
static const struct color primaryWavelengths = {680E-9f, 550E-9f, 450E-9f, 1.0f};
static const struct vector up = {0.0f, 1.0f, 0.0f};

static const struct color K = {0.686f, 0.678f, 0.666f, 1.0f};

static float getSunIntensity(const float zenithAngleCos) {
	return sunIntensity * max(0.0f, 1.0f - expf(-((cutoffAngle - acosf(zenithAngleCos)) / steepness)));
}

static struct color colorPow(struct color c1, struct color c2) {
	return (struct color){
		powf(c1.red, c2.red),
		powf(c1.green, c2.green),
		powf(c1.blue, c2.blue),
		powf(c1.alpha, c2.alpha)
	};
}

static struct color invertColor(float invert, struct color c) {
	return (struct color){invert / c.red, invert/ c.green, invert / primaryWavelengths.blue, invert / primaryWavelengths.alpha};
}

static struct color totalMie(struct color primaryWavelengths, struct color K, float T) {
	float c = (0.2f * T) * 10E-18;
	return colorCoef(0.434f * c * PI, colorMul(colorPow(invertColor((2.0f * PI), primaryWavelengths),(struct color){2.0f, 2.0f, 2.0f, 1.0f}), K));
}

static float rayleighPhase(float cosViewSunAngle) {
	return (3.0f / (16.0f * PI)) * (1.0f * powf(cosViewSunAngle, 2.0f));
}

static float hgPhase(float cosViewSunAngle, float g) {
	return (1.0f / (4.0f * PI)) * ((1.0f - powf(g, 2.0f)) / powf(1.0f - 2.0f * g * cosViewSunAngle + powf(g, 2.0f), 1.0f));
}

static struct color divideColors(struct color c1, struct color c2) {
	return (struct color){c1.red / c2.red, c1.green / c2.green, c1.blue / c2.blue, c1.alpha / c2.alpha};
}

static const struct vector sunDirection = {0.0f, 0.2f, 1.0f};
struct color sky(struct lightRay incidentRay) {
	float cosViewSunAngle = vecDot(incidentRay.direction, sunDirection);
	float cosSunUpAngle = vecDot(sunDirection, up);
	float cosUpViewAngle = vecDot(up, incidentRay.direction);
	
	float sunE = getSunIntensity(cosSunUpAngle);
	struct color rayleighAtX = (struct color){5.176821E-6f, 1.2785348E-5f, 2.8530756E-5f, 1.0f};
	struct color mieAtX = colorCoef(mieCoefficient, totalMie(primaryWavelengths, K, turbidity));
	float zenithAngle = max(0.0f, cosUpViewAngle);
	float rayleighOpticalLength = rayleighZenithLength / zenithAngle;
	float mieOpticalLength = mieZenithLength / zenithAngle;
	
	struct color Fex = (struct color){expf(-(rayleighAtX.red * rayleighOpticalLength + mieAtX.red * mieOpticalLength)), expf(-(rayleighAtX.green * rayleighOpticalLength + mieAtX.green * mieOpticalLength)), expf(-(rayleighAtX.blue * rayleighOpticalLength + mieAtX.blue * mieOpticalLength)), expf(-(rayleighAtX.alpha * rayleighOpticalLength + mieAtX.alpha * mieOpticalLength))};
	
	struct color rayleighXtoEye = colorCoef(rayleighPhase(cosViewSunAngle), rayleighAtX);
	struct color mieXtoEye = colorCoef(hgPhase(cosViewSunAngle, mieDirectionalG), mieAtX);
	
	struct color totalLightAtX = colorAdd(rayleighAtX, mieAtX);
	struct color lightFromXtoEye = colorAdd(rayleighXtoEye, mieXtoEye);
	struct color somethingElse = colorCoef(sunE, divideColors(lightFromXtoEye, totalLightAtX));
	struct color sky = colorMul(somethingElse, (struct color){1.0f - Fex.red, 1.0f - Fex.green, 1.0f - Fex.blue, 1.0f - Fex.alpha});
	
	sky = colorMul(sky, colorLerp((struct color){1.0f, 1.0f, 1.0f, 1.0f}, colorPow(colorMul(somethingElse, Fex), (struct color){0.5f, 0.5f, 0.5f, 0.5f}), clamp(powf(1.0f - vecDot(up, sunDirection), 5.0f), 0.0f, 1.0f)));
	
	return colorCoef(skyFactor * 0.01f, sky);
	
}
