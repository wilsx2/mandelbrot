#include "mandelbrot.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_ITERATIONS 255

struct pixel colorize(double complex c) {
    int i = escape_time(c, MAX_ITERATIONS);
    int is_in = i == MAX_ITERATIONS;
    if (is_in) {
        return (struct pixel){0, 0, 0};
    }
    return hue_to_rgb(i / (float)MAX_ITERATIONS);
}

int main(int argc, char *argv[]) {
    if (argc != 8) {
        fprintf(stderr, "Expected 7 arguments, received %d\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    // Parse arguments
    char filename[64];
    sprintf(filename, "%s.ppm", argv[1]);
    int width    = atoi(argv[2]);
    int height   = atoi(argv[3]);
    double real_max = atof(argv[4]);
    double real_min = atof(argv[5]);
    double imag_max = atof(argv[6]);
    double imag_min = atof(argv[7]);

    // Save
    struct axis_limits lim = {
        real_min + imag_min*I,
        real_max + imag_max*I
    };
    return !save_to_ppm(filename, width, height, &lim, colorize);
}
