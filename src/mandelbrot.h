#pragma once

#include <math.h>
#include <complex.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

// math

unsigned int escape_time(double complex c, unsigned int max_iterations) {
    double complex z = 0.0 + 0.0*I;
    unsigned int n = 0;

    while (cabs(z) <= 2*2 && n < max_iterations) {
        z = z*z + c;
        ++n;
    }

    return n;
}

// plotting

struct pixel { uint8_t r, g, b; };
struct axis_limits { double complex min, max; };

struct pixel hue_to_rgb(float h) {
    struct pixel out;

    int i = floor(h*6);
    float f = h - i;
    
    float q = 1.0 - f;
    float t = f;

    switch (i % 6) {
        case 0: out.r = 255,   out.g = t*255, out.b = 0;      break;
        case 1: out.r = q*255, out.g = 255,   out.b = 0;      break;
        case 2: out.r = 0,     out.g = 255,   out.b = t*255;  break;
        case 3: out.r = 0,     out.g = q*255, out.b = 255;    break;
        case 4: out.r = t*255, out.g = 0,     out.b = 255;    break;
        case 5: out.r = 255,   out.g = 0,     out.b = q*255;  break;
    }

    return out;
}

int save_to_ppm(const char* filename, size_t width, size_t height,
                const struct axis_limits* lim, struct pixel(*colorize)(double complex)) {
    // Open target file
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return 0;
    }

    // PPM header
    fprintf(fp, "P6\n%zu %zu\n255\n", width, height);
    
    // Pixel Data
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            double real = (x / (double)width)  * (creal(lim->max) - creal(lim->min)) + creal(lim->min);
            double imag = (y / (double)height) * (cimag(lim->max) - cimag(lim->min)) + cimag(lim->min);
            double complex c = real + imag*I;

            struct pixel rgb = colorize(c);
    
            fputc(rgb.r, fp);
            fputc(rgb.g, fp);
            fputc(rgb.b, fp);
        }
    }

    return 1;
}
