//
//  encoder.h
//  c-ray
//
//  Created by Valtteri on 8.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct imageFile;
struct renderInfo;

//Writes image data to file
void writeImage(struct imageFile *image);
