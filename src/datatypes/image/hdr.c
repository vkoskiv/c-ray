//
//  hdr.c
//  C-ray
//
//  Created by Valtteri on 16.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "texture.h"
#include "hdr.h"

struct hdr *newHDRI() {
	struct hdr *hdr = calloc(1, sizeof(*hdr));
	hdr->t = newTexture(none, 0, 0, 0);
	return hdr;
}

void destroyHDRI(struct hdr *hdr) {
	if (hdr) {
		destroyTexture(hdr->t);
		free(hdr);
	}
}
