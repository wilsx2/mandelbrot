#include "mandelbrot.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_ITERATIONS 255

int main(int argc, char *argv[]) {
    if (argc != 9) {
        fprintf(stderr, "Expected 7 arguments, received %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    char filename[64];
    sprintf(filename, "%s.ppm", argv[1]);
    int width    = atoi(argv[2]);
    int height   = atoi(argv[3]);
    double x_max = atof(argv[4]);
    double x_min = atof(argv[5]);
    double y_max = atof(argv[6]);
    double y_min = atof(argv[7]);

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
                (x / (double)height) * (x_max - x_min) + x_min,
                (y / (double)height) * (y_max - y_min) + y_min,
                MAX_ITERATIONS
            );
            int is_in = i == MAX_ITERATIONS;
            int r, g, b;
            if (is_in) {
                r = 0;
                g = 0;
                b = 0;
            } else {
                float hue = i / (float)MAX_ITERATIONS;

                // Convert HSV (S & V = 100%) to RGB
                float h = hue * 6.0;
                int j = floor(h);
                float f = h - j;
                
                float q = 1.0 - f;
                float t = f;

                switch (j % 6) {
                    case 0: r = 255,    g = t*255,  b = 0;      break;
                    case 1: r = q*255,  g = 255,    b = 0;      break;
                    case 2: r = 0,      g = 255,    b = t*255;  break;
                    case 3: r = 0,      g = q*255,  b = 255;    break;
                    case 4: r = t*255,  g = 0,      b = 255;    break;
                    case 5: r = 255,    g = 0,      b = q*255;  break;
                }
            }

            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }

    return 0;
}
