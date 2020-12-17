//
//  grayscale.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 15/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// TODO: Move this to a 'converter' group once more converters are added.

struct valueNode *newGrayscaleConverter(struct world *world, const struct colorNode *node);
