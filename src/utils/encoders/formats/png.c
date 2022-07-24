//
//  png.c
//  C-ray
//
//  Created by Valtteri on 8.4.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../../datatypes/image/imagefile.h"
#include <stddef.h>
#include "png.h"

#include <stdio.h>
#include <stdlib.h>
#include "../../logging.h"

#include "../../../vendored/lodepng.h"

#ifndef WINDOWS
#include <sys/utsname.h>
#endif

void encodePNGFromArray(const char *filename, const unsigned char *imgData, size_t width, size_t height, struct renderInfo imginfo) {
	LodePNGInfo info;
	lodepng_info_init(&info);
	info.time_defined = 1;
	
	char version[60];
	sprintf(version, "C-ray v%s [%.8s], © 2015-2020 Valtteri Koskivuori", imginfo.crayVersion, imginfo.gitHash);
	char samples[16];
	sprintf(samples, "%i", imginfo.samples);
	char bounces[16];
	sprintf(bounces, "%i", imginfo.bounces);
	char renderTime[64];
	smartTime(imginfo.renderTime, renderTime);
	char threads[16];
	sprintf(threads, "%i", imginfo.threadCount);
#ifndef WINDOWS
	char sysinfo[1300];
	struct utsname name;
	uname(&name);
	sprintf(sysinfo, "%s %s %s %s %s", name.machine, name.nodename, name.release, name.sysname, name.version);
#endif
	
	lodepng_add_text(&info, "C-ray Version", version);
	lodepng_add_text(&info, "C-ray Source", "https://github.com/vkoskiv/c-ray");
	lodepng_add_text(&info, "C-ray Samples", samples);
	lodepng_add_text(&info, "C-ray Bounces", bounces);
	lodepng_add_text(&info, "C-ray RenderTime", renderTime);
	lodepng_add_text(&info, "C-ray Threads", threads);
#ifndef WINDOWS
	lodepng_add_text(&info, "C-ray SysInfo", sysinfo);
#endif
	
	LodePNGState state;
	lodepng_state_init(&state);
	state.info_raw.bitdepth = 8;
	state.info_raw.colortype = LCT_RGB;
	lodepng_info_copy(&state.info_png, &info);
	state.encoder.add_id = 1;
	state.encoder.text_compression = 0;
	
	size_t bytes = 0;
	unsigned char *buf = NULL;
	
	unsigned error = lodepng_encode(&buf, &bytes, imgData, (unsigned)width, (unsigned)height, &state);
	if (error) logr(warning, "Error %u: %s\n", error, lodepng_error_text(error));
	
	writeFile(buf, bytes, filename);
	
	lodepng_info_cleanup(&info);
	lodepng_state_cleanup(&state);
	free(buf);
}
