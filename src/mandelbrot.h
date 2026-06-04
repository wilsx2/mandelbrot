#pragma once

#include <math.h>

unsigned int escape_time(double creal, double cimag, unsigned int max_iterations) {
    double zreal = 0.0;
    double zimag = 0.0;
    unsigned int n = 0;

    while (zreal*zreal + zimag*zimag <= 2*2 && n < max_iterations) {
        double zreal_next;
        zreal_next  = zreal*zreal - zimag*zimag + creal;
        zimag       = 2*zreal*zimag + cimag;
        zreal = zreal_next;

        ++n;
    }

    return n;
}

void hue_to_rgb(float h, char *r, char *g, char *b) {
    int i = floor(h);
    float f = h - i;
    
    float q = 1.0 - f;
    float t = f;

    switch (i % 6) {
        case 0: *r = 255,    *g = t*255,  *b = 0;      break;
        case 1: *r = q*255,  *g = 255,    *b = 0;      break;
        case 2: *r = 0,      *g = 255,    *b = t*255;  break;
        case 3: *r = 0,      *g = q*255,  *b = 255;    break;
        case 4: *r = t*255,  *g = 0,      *b = 255;    break;
        case 5: *r = 255,    *g = 0,      *b = q*255;  break;
    }
}
