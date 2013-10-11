
#define WIN_SETTINGS

#define WIN_WIDTH  500
#define WIN_HEIGHT 500
#define WIN_X      300
#define WIN_Y      200

#define WIN_DISPLAY GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL
#define VERSION_MAJOR 3
#define VERSION_MINOR 0

#define NGL_NEAR -100.0f
#define NGL_FAR   500.0f

#define CAM_FOV 45.0f
#define CAM_NEAR 0.1f
#define CAM_FAR 500.0f


#define WRLD_MAX_WIDTH   2000.0f
#define WRLD_MAX_HEIGHT  2000.0f
#define WRLD_PAGE_WIDTH  300.0f
#define WRLD_PAGE_HEIGHT 300.0f



#define CFG_LOGFILE "noengine.log"
#define CFG_LOGFILE_SERVER "noengine.serv.log"
#define CFG_LOGLEVEL Logger::LOG_DEBUG

// terminal colours
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

// font rendering
// #ifndef __CFG_FONT_RENDERING__
// #define __CFG_FONT_RENDERING__
// #define FONT_FACES { "data/fonts/freefont/FreeSerif.ttf", "data/fonts/droid/DroidSans.ttf" }
// #define FONT_SIZES { 12, 24, 18, 24 }
// #define FONT_NUM_FACES 2
// #define FONT_NUM_SIZES 4
// #endif
