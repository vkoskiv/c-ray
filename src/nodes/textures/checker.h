//
//  checker.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct world;

const struct colorNode *newCheckerBoardTexture(const struct node_storage *s, const struct colorNode *A, const struct colorNode *B, const struct valueNode *scale);
