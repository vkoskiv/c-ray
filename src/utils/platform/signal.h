//
//  signal.h
//  C-ray
//
//  Created by Valtteri on 7.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum sigtype {
	sigint,
	sigabrt,
};

int registerHandler(enum sigtype, void (*handler)(int));
