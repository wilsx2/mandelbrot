#include "mandelbrot.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_ITERATIONS 255

int main(int argc, char *argv[]) {
    if (argc != 8) {
        fprintf(stderr, "Expected 7 arguments, received %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    char filename[64];
    sprintf(filename, "%s.ppm", argv[1]);
    int width   = atoi(argv[2]);
    int height  = atoi(argv[3]);
    float x_max = atof(argv[4]);
    float x_min = atof(argv[5]);
    float y_max = atof(argv[6]);
    float y_min = atof(argv[7]);

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // PPM header
    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    
    // Pixel Data
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int i = mandelbrot(
                (x / (float)height) * (x_max - x_min) + x_min,
                (y / (float)height) * (y_max - y_min) + y_min,
                MAX_ITERATIONS
            );
            int is_in = i == MAX_ITERATIONS;
            unsigned char r = 0;
            unsigned char g = 0;
            unsigned char b = is_in ? 0 : 128;

            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }

    return 0;
}
