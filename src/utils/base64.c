//
//  base64.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 18/03/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#include "base64.h"

#include <stdlib.h>

// Neither of these implementations are mine, but they are
// way faster than anything I could come up with, so we're using
// them here.

/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

// Ported to C here, lifted from:
// https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c

static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *b64encode(const void *input, const size_t dataLength) {
	unsigned char *out, *pos;
	const unsigned char *end, *in;
	const unsigned char *data = (const unsigned char *)input;
	size_t outputLength = 4 * ((dataLength + 2) / 3); // 3-byte blocks to 4-byte
	if (outputLength < dataLength)
		return NULL; // integer overflow
	char *outStr = calloc(outputLength + 1, sizeof(*outStr));
	out = (unsigned char *)&outStr[0];
	end = data + dataLength;
	in = data;
	pos = out;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
	}
	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
	}
	return outStr;
}

// Decoder ported to C from SO user Polfosol's implementation here:
// https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c/13935718
// License unknown

static const int B64index[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,62,63,62,62,63,52,53,54,55,
	56,57,58,59,60,61,0,0,0,0,0,0,0,0,1,2,3,4,5,6,
	7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,0,
	0,0,0,63,0,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
	41,42,43,44,45,46,47,48,49,50,51
};

void *b64decode(const char *data, const size_t inputLength, size_t *outLength) {
	if (!inputLength) return "";
	unsigned char *p = (unsigned char *)data;
	const size_t pad1 = inputLength % 4 || p[inputLength - 1] == '=';
	const size_t pad2 = pad1 && (inputLength % 4 > 2 || p[inputLength - 2] != '=');
	const size_t L = (inputLength - pad1) / 4 << 2;
	const size_t strSize = L / 4 * 3 + pad1 + pad2;
	char *str = calloc(strSize + 1, sizeof(*str));
	size_t j = 0;
	for (size_t i = 0; i < L; i += 4) {
		int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
		str[j++] = n >> 16;
		str[j++] = n >> 8 & 0xFF;
		str[j++] = n & 0xFF;
	}
	if (pad1) {
		int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
		str[j++] = n >> 16;
		
		if (pad2) {
			n |= B64index[p[L + 2]] << 6;
			str[j++] = n >> 8 & 0xFF;
		}
	}
	if (outLength) *outLength = strSize;
	str[strSize] = 0;
	return str;
}
