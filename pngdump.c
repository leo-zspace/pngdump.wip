/*#####################################################################
**  Included Files
#####################################################################*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/*#####################################################################
**  Local Defines
#####################################################################*/

#define null NULL
#define min(a,b) (((a)<(b))?(a):(b))
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

typedef unsigned char byte;

/* Structure of command table entry. */
typedef struct OptionEntry
{
    char const *optStr;     /* Command string. */
    char const *argFormat;  /* Command option format. */
    int         numArgs;    /* Number of expected arguments. */

} OptionEntry_t;

/* Structure of command table entry. */
typedef struct CommandEntry
{
    char const *cmdStr;                                             /* Command string. */
    void (*cmdFunc)(const byte* data, int roiX, int roiY, int roiW, int roiH, int w);  /* Command function handler. */

} CommandEntry_t;

/** Enumeration of command line argument parser states. */
typedef enum ParserState
{
    PS_OPTIONS,   /**< Parse option state. */
    PS_ARGUMENTS  /**< Parser arguments state. */

} ParserState_t;


/*#####################################################################
**  Private Function Prototypes
#####################################################################*/

static void dump(const byte* data, int roiX, int roiY, int roiW, int roiH, int w);
static void histogram(const byte* data, int roiX, int roiY, int roiW, int roiH, int w);
static int findOption(const char* option);
static int findCommand(const char* command);


/*#####################################################################
**  Public Data
#####################################################################*/


/*#####################################################################
**  Private Data
#####################################################################*/

/* Table of command line options. */
OptionEntry_t OPTION_TABLE[] =
{
    {"roi", "%d,%d:%dx%d", 4},
    {null,  null,          0}  /* End of table indicator. MUST BE LAST!!! */
};


/* Table of command line arguments */
CommandEntry_t COMMAND_TABLE[] =
{
    {"dump",      dump     },
    {"histogram", histogram},
    {null,        null     }  /* End of table indicator. MUST BE LAST!!! */
};

/* Option delimiter. */
const char optDelim[3] = "--\0";


/*#####################################################################
**  Public Functions
#####################################################################*/


/**********************************************************************
 * @brief Main execution.
 *
 * @param [in] argc Number of arguments.
 * @param [in] argv List of arguments.
 *
 * @return Exit code.
 *
 * @note None.
**********************************************************************/
int main(int argc, const char* argv[])
{
    int r     =  0;  /* Return code. */
    int w     =  0;  /* Image width in pixels.*/
    int h     =  0;  /* Image height in pixels. */
    int c     =  0;  /* Number of image components in file. */
    int i     =  0;  /* Iterator. */
    int roiX  =  0;  /* ROI x-coordinate. */
    int roiY  =  0;  /* ROI y-coordinate. */
    int roiW  =  0;  /* ROI width. */
	int roiH  =  0;  /* ROI height. */
    int index = -1;  /* Index of found option/command. */

    char* token = null;  /* Pointer to string token. */
    byte* data  = null;  /* Pointer to loaded image data. */

    ParserState_t currState = PS_OPTIONS;  /* Current state of parser state machine.  */

    /* Check to see if there are any options to run before proceeding. */
    if (argc > 1)
    {
        /* Load image file. */
        data = stbi_load("camera.png", &w, &h, &c, 0);

        //dump(data, 0, 0, w, h, w);

        /* Check that image file loaded correctly. */
        if (data == null)
        {
            /* Error loading image file. Report error and exit. */
            r = errno;
            perror("file not found\n");
        }
        else
        {
            /* Parse command line arguments. */
            for(i = 1; i < argc; i++)
            {
            	/* Parser state machine. */
                switch (currState)
                {
                    case PS_OPTIONS:
                    {
                        /* Check to see if current command line argument is an option or not
                         * by checking if argument begins with option delimiter.. */
                    	if(strncmp(argv[i], optDelim, strlen(optDelim)) == 0)
                    	{
                    		/* Current command line argument is an option. */
							token = strtok(argv[i], optDelim);

							if (token != null)
							{
								/* Current command line argument is an option.*/
								index = findOption(token);

								if (index != -1)
								{
									/* Located option. Check if option requires additional arguments. */
									if (OPTION_TABLE[index].argFormat != null)
									{
										/* Option requires additional arguments to be parsed. */
										currState = PS_ARGUMENTS;
									}
								}
								else
								{
									/* Option not found. */
									printf("%s option not supported.\n", token);
									return EXIT_FAILURE;
								}
							}
                        }
                        else
                        {
                            /* Current command line argument is not an option.
                             * Check if command line argument is a supported command. */
                            index = findCommand(argv[i]);

                            if (index != -1)
                            {
                                /* Execute command.*/
                                COMMAND_TABLE[index].cmdFunc(data, roiX, roiY, roiW, roiH, w);
                            }
							else
							{
								/* Command not found. */
								printf("%s command not supported.\n", argv[i]);
								return EXIT_FAILURE;
							}
                        }

                        break;
                    }

                    case PS_ARGUMENTS:
                    {
                    	/* TODO: Determine a more flexible way for handling different argument formats. */
                    	int retval = sscanf(argv[i], OPTION_TABLE[index].argFormat, &roiX, &roiY, &roiW, &roiH);

                    	/* Check that the expected number of arguments have been parsed.*/
                    	if (retval != OPTION_TABLE[index].numArgs)
                    	{
                    		printf("incorrect number of arguments.");
                    		return EXIT_FAILURE;
                    	}
                    	else
                    	{
                    		/* Check that parsed arguments are within bounds of image. */
                    		/* Check ROI x-coordinate. */
                    		if ((roiX > w) || (roiX < 0))
                    		{
                    			printf("x value out of bounds.\n");
                    			return EXIT_FAILURE;
                    		}

                    		/* Check ROI y-coordinate. */
                    		if ((roiY > h) || (roiY < 0) )
                    		{
                    			printf("y out of bounds.\n");
                    			return EXIT_FAILURE;
                    		}

                    		/* Check ROI width. */
                    		if (((roiX + roiW) > w) || (roiW < 0))
                    		{
                    			printf("W out of bounds.\n");
                    			return EXIT_FAILURE;
                    		}

                    		/* Check ROI height. */
                    		if (((roiY + roiH) > h) || (roiH < 0))
                    		{
                    			printf("H out of bounds.\n");
                    			return EXIT_FAILURE;
                    		}
                    	}

                    	currState = PS_OPTIONS;
                        break;
                    }

                    default:
                    {
                    	printf("INVALID STATE\n");
                        break;
                    }
                }
            }

            /* Free memory from loaded image file. */
            free(data);
        }
    }

    return r;  /* Exit. */
}


/*#####################################################################
**  Private Functions
#####################################################################*/

/**********************************************************************
 * @brief Dumps selected raw image data to stdout of region of interest (ROI).
 *
 * @param [in] data Pointer to image data.
 * @param [in] roiX X-coordinate of ROI.
 * @param [in] roiY Y-coordinate of ROI.
 * @param [in] roiW Width of ROI.
 * @param [in] roiH Height of ROI.
 * @param [in] w    Width of image.
 *
 * @note Upper left of image is origin (0,0).
**********************************************************************/
static void dump(const byte* data, int roiX, int roiY, int roiW, int roiH, int w)
{
	int iX = 0;  /* x-coordinate iterator. */
	int iY = 0;  /* y-coordinate iterator. */
	int iD = 0;  /* Data offset. */

    printf("(%d,%d) %dx%d\n", roiX, roiY, roiW, roiH);

    /* Traverse region of interest. */
    for (iY = roiY; iY < (roiY + roiH); iY++)
    {
    	for (iX = roiX; iX < (roiX + roiW); iX++)
        {
        	/* Convert 2-dimensional coordinates back into array index. */
            iD = iX + (w * iY);
            printf("0x%02X ", data[iD]);
        }

        printf("\n");
    }
}


/**********************************************************************
 * @brief Dumps histogram data in CSV format to stdout of region of interest (ROI).
 *
 * @param [in] data Pointer to image data.
 * @param [in] roiX X-coordinate of ROI.
 * @param [in] roiY Y-coordinate of ROI.
 * @param [in] roiW Width of ROI.
 * @param [in] roiH Height of ROI.
 * @param [in] w    Width of image.
 *
 * @note Upper left of image is origin (0,0).
**********************************************************************/
static void histogram(const byte* data, int roiX, int roiY, int roiW, int roiH, int w)
{
	int iX = 0;  /* x-coordinate iterator. */
	int iY = 0;  /* y-coordinate iterator. */
	int iD = 0;  /* Data offset. */
	int histTable[256] = { 0 };  /* Image histogram data. */

	/* Traverse region of interest. */
    for (iY = roiY; iY < (roiY + roiH); iY++)
    {
    	for (iX = roiX; iX < (roiX + roiW); iX++)
        {
        	/* Convert 2-dimensional coordinates back into array index. */
            iD = iX + (w * iY);
            histTable[data[iD]]++;
        }
    }

    /* Output */
    for (iD = 0; iD < NELEMS(histTable) - 1; iD++)
    {
    	printf("%d, %d\n", iD, histTable[iD]);
    }

    printf("%d, %d\n", iD, histTable[iD]);
}


/**********************************************************************
 * @brief Locates option in supported option table.
 *
 * @param [in] option Option string to locate.
 *
 * @return Index in option table for option. -1 if option cannot be found.
 *
 * @note None.
**********************************************************************/
static int findOption(const char* option)
{
	int index = 0; /* Index within option table of found option. */

	/* Parameter check. */
	if (option != null)
	{
		while (OPTION_TABLE[index].optStr != null)
		{
			if (strcmp(option, OPTION_TABLE[index].optStr) == 0)
			{
				/* Located option. */
				break;
			}
			else
			{
				/* Test next option string. */
				index++;
			}
		}
	}
	else
	{
		/* Bad parameters. */
		index = -1;
	}

	return index;
}


/**********************************************************************
 * @brief Locates command in supported command table.
 *
 * @param [in] command Command string to locate.
 *
 * @return Index in command table for command. -1 if command cannot be found.
 *
 * @note None.
**********************************************************************/
static int findCommand(const char* command)
{
	int index = 0; /* Index within command table of found command. */

	/* Parameter check. */
	if (command != null)
	{
		while (COMMAND_TABLE[index].cmdStr != null)
		{
			if (strcmp(command, COMMAND_TABLE[index].cmdStr) == 0)
			{
				/* Located option. */
				break;
			}
			else
			{
				/* Test next command string. */
				index++;
			}
		}
	}
	else
	{
		/* Bad parameters. */
		index = -1;
	}

	return index;
}

