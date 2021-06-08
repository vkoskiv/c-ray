//
//  isotropic.h
//  C-ray
//
//  Created by Valtteri on 27.5.2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// A uniform scatter direction, used for volumes

const struct bsdfNode *newIsotropic(const struct world *world, const struct colorNode *tex);
