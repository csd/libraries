
/*
 * Video frame conversion functions
 */

void yuv420p_to_bgra(int width, int height, const unsigned char *src, unsigned char *dst) {
	int line, col, linewidth;
	int y, u, v, yy, vr, ug, vg, ub;
	int r, g, b;
	const unsigned char *py, *pu, *pv;

	linewidth = width >> 1;
	py = src;
	pu = py + (width * height);
	pv = pu + (width * height) / 4;

	y = *py++;
	yy = y << 8;
	u = *pu - 128;
	ug =   88 * u;
	ub =  454 * u;
	v = *pv - 128;
	vg =  183 * v;
	vr =  359 * v;

	for (line = 0; line < height; line++) {
		for (col = 0; col < width; col++) {
			r = (yy +      vr) >> 8;
			g = (yy - ug - vg) >> 8;
			b = (yy + ub     ) >> 8;

			// Clamp values (truncate)
			if (r < 0)   r = 0;
			if (r > 255) r = 255;
			if (g < 0)   g = 0;
			if (g > 255) g = 255;
			if (b < 0)   b = 0;
			if (b > 255) b = 255;

			*dst++ = b;
			*dst++ = g;
			*dst++ = r;
			*dst++ = 0;

			y = *py++;
			yy = y << 8;
			if (col & 1) {
				pu++;
				pv++;

				u = *pu - 128;
				ug =   88 * u;
				ub =  454 * u;
				v = *pv - 128;
				vg =  183 * v;
				vr =  359 * v;
			}
		} /* ..for col */
		if ((line & 1) == 0) { // even line: rewind
			pu -= linewidth;
			pv -= linewidth;
		}
	} /* ..for line */
}

void yuyv_to_bgra(int width, int height, const unsigned char *src, unsigned char *dst) {
	const unsigned char *s;
	int l, c;
	int r, g, b, cr, cg, cb, y1, y2;

	l = height;
	s = src;
	while (l--) {
		c = width >> 1;
		while (c--) {
			y1 = *s++;
			cb = ((*s - 128) * 454) >> 8;
			cg = (*s++ - 128) * 88;
			y2 = *s++;
			cr = ((*s - 128) * 359) >> 8;
			cg = (cg + (*s++ - 128) * 183) >> 8;

			r = y1 + cr;
			b = y1 + cb;
			g = y1 - cg;

			// Truncate
                        if (r < 0)   r = 0;
                        if (r > 255) r = 255;
                        if (g < 0)   g = 0;
                        if (g > 255) g = 255;
                        if (b < 0)   b = 0;
                        if (b > 255) b = 255;

			*dst++ = b;
			*dst++ = g;
			*dst++ = r;
			*dst++ = 0;

			r = y2 + cr;
			b = y2 + cb;
			g = y2 - cg;

			// Truncate
                        if (r < 0)   r = 0;
                        if (r > 255) r = 255;
                        if (g < 0)   g = 0;
                        if (g > 255) g = 255;
                        if (b < 0)   b = 0;
                        if (b > 255) b = 255;

			*dst++ = b;
			*dst++ = g;
			*dst++ = r;
			*dst++ = 0;
		}
	}
}

