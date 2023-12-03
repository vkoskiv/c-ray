//
//  checker.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct node_storage;
struct valueNode;

const struct colorNode *newCheckerBoardTexture(const struct node_storage *s, const struct colorNode *A, const struct colorNode *B, const struct valueNode *scale);
