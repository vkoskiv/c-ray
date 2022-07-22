//
//  gradient.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

const struct colorNode *newGradientTexture(const struct node_storage *s, struct color down, struct color up);

