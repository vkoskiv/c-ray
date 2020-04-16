//
//  hdr.h
//  C-ray
//
//  Created by Valtteri on 16.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct hdr {
	struct texture *t;
	float offset; //radians
};

struct hdr *newHDRI(void);

void destroyHDRI(struct hdr *hdr);
