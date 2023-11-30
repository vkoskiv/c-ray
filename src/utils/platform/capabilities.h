//
//  capabilities.h
//  C-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

/// Get amount of logical processing cores on the system
/// @remark Is unaware of NUMA nodes on high core count systems
/// @return Amount of logical processing cores
int sys_get_cores(void);
