#include "mandelbrot.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_ITERATIONS 255

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

    // Open target file
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen()");
        exit(EXIT_FAILURE);
    }

    // PPM header
    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    
    // Pixel Data
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int i = escape_time (
                (x / (double)height) * (real_max - real_min) + real_min,
                (y / (double)height) * (imag_max - imag_min) + imag_min,
                MAX_ITERATIONS
            );
            int is_in = i == MAX_ITERATIONS;
            char r, g, b;
            if (is_in) {
                r = g = b = 0;
            } else {
                hue_to_rgb(i / (float)MAX_ITERATIONS,
                    &r,
                    &g,
                    &b
                );
            }

            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }

    return 0;
}
