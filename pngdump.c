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

typedef struct option_entry_s {
    char const* option;
    char const* format;
    int count;
} option_entry_t;

typedef struct command_entry {
    char const* command;
    void (*roi)(const byte* data, int x, int y, int w, int h, int stride);
} command_entry_t;

static void dump(const byte* data, int roiX, int roiY, int roiW, int roiH, int w);
static void histogram(const byte* data, int roiX, int roiY, int roiW, int roiH, int w);
static int find_option(const char* option);
static int find_command(const char* command);

const option_entry_t OPTION_TABLE[] = {
    {"roi", "%d,%d:%dx%d", 4},
    {null,  null,          0}
};

const command_entry_t COMMAND_TABLE[] = {
    {"dump",      dump     },
    {"histogram", histogram},
    {null,        null     }
};

const char opt_delim[3] = "--\0"; // TODO: "\0" does not do what it seems to do...

int main(int argc, const char* argv[]) {
    // TODO: the state machine with 2 states is actually 1 "if" statement.
    //       The command parser is overcomplicated.
    //       The local variables meaning should be obvious from call site
    //       and do not deserve the comments.
    int r     =  0;  /* Return code. */
    int w     =  0;  /* Image width in pixels.*/
    int h     =  0;  /* Image height in pixels. */
    int c     =  0;  /* Number of image components in file. */
    int i     =  0;  /* Iterator. */
    int roi_x =  0;  /* ROI x-coordinate. */
    int roi_y =  0;  /* ROI y-coordinate. */
    int roi_w =  0;  /* ROI width. */
    int roi_h =  0;  /* ROI height. */
    int index = -1;  /* Index of found option/command. */
    char* token = null;  /* Pointer to string token. */
    byte* data  = null;  /* Pointer to loaded image data. */
    enum {
        PS_OPTIONS = 0,
        PS_ARGUMENTS= 1
    };
    int state = PS_OPTIONS;
    if (argc > 1) {
        data = stbi_load("camera.png", &w, &h, &c, 0);
        if (data == null) {
            r = errno;
            perror("file not found\n");
        } else {
            for (i = 1; i < argc; i++) {
                /* Parser state machine. */
                switch (state) {
                    case PS_OPTIONS: {
                        if (strncmp(argv[i], opt_delim, strlen(opt_delim)) == 0) {
                            // TODO: resolve following issues:
                            // next line is dangerous - it assumes that argv[i] is modifieable
                            // which is not true on all platforms. strtok also uses globals
                            // which is not a great practice to begin with... There is a
                            // a cleaner way then (char*)argv[i] cast. FIXME
                            /* Current command line argument is an option. */
                            token = strtok((char*)argv[i], opt_delim);
                            if (token != null) {
                                /* Current command line argument is an option.*/
                                index = find_option(token);
                                if (index != -1) {
                                    /* Located option. Check if option requires additional arguments. */
                                    if (OPTION_TABLE[index].format != null) {
                                        /* Option requires additional arguments to be parsed. */
                                        state = PS_ARGUMENTS;
                                    }
                                } else {
                                    /* Option not found. */
                                    printf("%s option not supported.\n", token);
                                    return EXIT_FAILURE;
                                }
                            }
                        } else {
                            index = find_command(argv[i]);
                            if (index != -1) {
                                /* Execute command.*/
                                COMMAND_TABLE[index].roi(data, roi_x, roi_y, roi_w, roi_h, w);
                            } else {
                                /* Command not found. */
                                printf("%s command not supported.\n", argv[i]);
                                return EXIT_FAILURE;
                            }
                        }
                        break;
                    }
                    case PS_ARGUMENTS: {
                        int retval = sscanf(argv[i], OPTION_TABLE[index].format, &roi_x, &roi_y, &roi_w, &roi_h);
                        if (retval != OPTION_TABLE[index].count) {
                            printf("incorrect number of arguments.");
                            return EXIT_FAILURE;
                        } else {
                            if ((roi_x > w) || (roi_x < 0)) {
                                printf("x value out of bounds.\n");
                                return EXIT_FAILURE;
                            }
                            if ((roi_y > h) || (roi_y < 0)) {
                                printf("y out of bounds.\n");
                                return EXIT_FAILURE;
                            }
                            if (((roi_x + roi_w) > w) || (roi_w < 0)) {
                                printf("W out of bounds.\n");
                                return EXIT_FAILURE;
                            }
                            if (((roi_y + roi_h) > h) || (roi_h < 0)) {
                                printf("H out of bounds.\n");
                                return EXIT_FAILURE;
                            }
                        }
                        state = PS_OPTIONS;
                        break;
                    }
                    default: {
                        printf("INVALID STATE\n");
                        break;
                    }
                }
            }
            free(data);
        }
    }
    return r;
}

static void dump(const byte* data, int x, int y, int w, int h, int stride) {
    printf("(%d,%d) %dx%d\n", x, y, w, h);
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            int ix = i + (stride * y);
            printf("0x%02X ", data[ix]);
        }
        printf("\n");
    }
}

static void histogram(const byte* data, int x, int y, int w, int h, int stride) {
    int histogram[256] = { 0 };
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            int ix = i + (stride * y);
            histogram[data[ix]]++;
        }
    }
    for (int i = 0; i < countof(histogram); i++) {
        printf("%d, %d\n", i, histogram[i]);
    }
}

// functions below are local to the file and should not
// expect null as an argument.

static int find_option(const char* option) {
    assert(option != null); 
    int index = 0;
    if (option != null) {
        while (OPTION_TABLE[index].option != null) {
            if (strcmp(option, OPTION_TABLE[index].option) == 0) {
                break;
            } else {
                index++;
            }
        }
    } else {
        index = -1; // this should be an assert()
    }
    return index;
}

static int find_command(const char* command) {
    assert(command != null); 
    int index = 0;
    if (command != null) {
        while (COMMAND_TABLE[index].command != null) {
            if (strcmp(command, COMMAND_TABLE[index].command) == 0) {
                break;
            } else {
                index++;
            }
        }
    } else {
        index = -1; // this should be an assert()
    }
    return index;
}
