//
//  logo.h
//  C-ray
//
//  Created by Valtteri on 2.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

// The C-ray logo, encoded as a PNG.
// This will be decoded and passed to SDL2 in initDisplay
// This source file is 227kB, but the final PNG embedded in the
// binary is only 37kB
// Decoding and setting this logo adds about ~130ms to the initial setup run-time.
#ifndef NO_LOGO
unsigned int logo_png_data_len = 37566;
unsigned char logo_png_data[] = {
#include "logo_bytes.txt"
};
#endif
