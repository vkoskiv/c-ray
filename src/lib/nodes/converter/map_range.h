//
//  map_range.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 11/08/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

const struct valueNode *newMapRange(const struct node_storage *s,
									const struct valueNode *input_value,
									const struct valueNode *from_min,
									const struct valueNode *from_max,
									const struct valueNode *to_min,
									const struct valueNode *to_max);
