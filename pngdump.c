#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define null NULL

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef uint8_t byte;

static void dump(const byte* data, int x, int y, int w, int h, int stride) {
    printf("(%d,%d) %dx%d\n", x, y, w, h);
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            int ix = i * stride + j;
            printf("0x%02X ", data[ix]);
        }
        printf("\n");
    }
}

static void histogram(const byte* data, int x, int y, int w, int h, int stride) {
    int histogram[256] = { 0 };
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            int ix = i * stride + j;
            histogram[data[ix]]++;
        }
    }
    for (int i = 0; i < countof(histogram); i++) {
        printf("%d, %d\n", i, histogram[i]);
    }
}

static int args_option_index(int argc, const char* argv[], const char* option) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) { break; } // no options after '--'
        if (strcmp(argv[i], option) == 0) { return i; }
    }
    return -1;
}

static int args_remove_at(int ix, int argc, const char* argv[]) { // returns new argc
    assert(0 < argc);
    assert(0 < ix && ix < argc); // cannot remove argv[0]
    for (int i = ix; i < argc; i++) { argv[i] = argv[i+1]; }
    argv[argc - 1] = "";
    return argc - 1;
}

static int parse_roi(int *argc, const char* argv[], 
       int *rx, int *ry, int *rw, int *rh, int iw, int ih) {
    int r = 0;
    int ix = args_option_index(*argc, argv, "--roi");
    if (ix >= 0) {
        if (*argc < ix + 1) {
            perror("expected --roi X,Y:WxH");
            r = EXIT_FAILURE;
        } else { // function does not touch *rx, *ry, *rw, *rh on failure
            int x = 0;
            int y = 0;
            int w = 0;
            int h = 0;
            if (sscanf(argv[ix + 1], "%d,%d:%dx%d", &x, &y, &w, &h) != 4) {
                perror("expected --roi X,Y:WxH");
                r = EXIT_FAILURE;
            } else if (0 <= x && x + w <= iw && 0 <= y && y + h <= ih) {
                *argc = args_remove_at(ix, *argc, argv); // removes "--roi"
                *argc = args_remove_at(ix, *argc, argv); // removes "X,Y:WxH"
                *rx = x;
                *ry = y;
                *rw = w;
                *rh = h;
            } else {
                fprintf(stderr, "%d,%d:%d:%d out of [%d][%d] range\n", 
                    x, y, w, h, iw, ih);
                r = EXIT_FAILURE;
            }
        }
    }
    return r;
}

static int usage() {
    fprintf(stderr, "pngdump [--roi X,Y:WxH] dump|histogram\n");
    return EXIT_FAILURE;
}

int main(int argc, const char* argv[]) {
    int r = 0;
    int w = 0;
    int h = 0;
    int c = 0;
    byte* data  = null;  /* Pointer to loaded image data. */
    data = stbi_load("camera.png", &w, &h, &c, 0);
    if (data == null) {
        fprintf(stderr, "failed to read \"camera.png\" errno=%d \"%s\"", 
            errno, strerror(errno));
        r = EXIT_FAILURE;
    }
    if (r == 0) {
        if (c != 1) {
            fprintf(stderr, "expected 1 byte per pixel instead of %d"
                " in file \"camera.png\" %dx%d", c, w, h);
            r = EXIT_FAILURE;
        }
    }
    int rx = 0; // default roi 0,0:w:h
    int ry = 0;
    int rw = w;
    int rh = h;
    if (r == 0) {
        r = parse_roi(&argc, argv, &rx, &ry, &rw, &rh, w, h);
    }
    if (r == 0) {
        if (argc < 1) {
            perror("expected command: dump or histogram");
            usage();
            r = EXIT_FAILURE;
        } else if (strcmp(argv[1], "dump") == 0) {
            dump(data, rx, ry, rw, rh, w);
        } else if (strcmp(argv[1], "histogram") == 0) {
            histogram(data, rx, ry, rw, rh, w);
        } else {
            fprintf(stderr, "unexpected command: %s", argv[1]);
        }
    }
    if (data != null) { free(data); }
    return r;
}
