#pragma once

/**
 * @brief Calculate iterations of z until it is determined z will diverge to infinity
 * @param dx The real component of c
 * @param dy The imaginary component of c 
 * @return The number of iterations it took for z_n to determine z_n will diverge to infinity
 */
unsigned int mandelbrot(float dx, float dy, unsigned int max_iterations) {
    // z_0 = 0
    float x = 0; // Real component of z_n
    float y = 0; // Imaginary component of z_n
    
    unsigned int iterations = 0;
    while (x*x + y*y <= 2*2 && iterations < max_iterations) {
        // z_n = z_{n-1}^2 + c
        float x_next = x*x - y*y + dx;
        y = 2*x*y + dy;
        x = x_next;
        
        ++iterations;
    }

    return iterations;
}
