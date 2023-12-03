//
//  memory.h
//  C-ray
//
//  Created by Valtteri on 19.2.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// C99 doesn't have max_align_t, so we have to supply our own. This should do.
typedef union {char *p; double d; long double ld; long int i;} cray_max_align_t;
