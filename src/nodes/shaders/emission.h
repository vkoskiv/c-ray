//
//  emission.h
//  C-ray
//
//  Created by Valtteri on 2.1.2021.
//  Copyright © 2021-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

const struct bsdfNode *newEmission(const struct node_storage *s, const struct colorNode *tex, const struct valueNode *strength);
