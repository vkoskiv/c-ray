//
//  colormix.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 17/12/2023.
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

const struct colorNode *new_color_mix(const struct node_storage *s, const struct colorNode *A, const struct colorNode *B, const struct valueNode *factor);

