//
//  emission.h
//  C-ray
//
//  Created by Valtteri on 2.1.2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

const struct bsdfNode *newEmission(const struct world *world, const struct colorNode *tex, const struct valueNode *strength);
