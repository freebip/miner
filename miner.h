#ifndef __MINER_H__
#define __MINER_H__

#include "libbip.h"

typedef unsigned short word;
typedef unsigned char byte;

struct appdata_t
{
    Elf_proc_* proc;
    void* ret_f;
    int current_screen;
    byte board[30];
    byte mines;
    word time;
    int state;
    int mode;
    unsigned long randseed;
};

// preset

#define MINS_COUNT_FROM 3
#define MINS_COUNT_TO  5 

#define MINE_FLAG 0x80
#define OPEN_CELL_FLAG 0x40
#define SET_FLAG 0x20

#define MODE_SET_FLAG 0
#define MODE_MINA 1

// state

#define STATE_STARTING 0
#define STATE_PLAY 1
#define STATE_PLAY_O 2
#define STATE_LOSE 3
#define STATE_WIN 4

// res

#define RES_BIG_MINA 10
#define RES_CLOCK 11
#define RES_MINA 12
#define RES_SMILE 13
#define RES_SMILE_BAD 14
#define RES_SMILE_O 15
#define RES_SMILE_OK 16
#define RES_FLAG 17
#define RES_MODE_FLAG 18
#define RES_MODE_MINA 19

void show_screen(void* return_screen);
void keypress_screen();
int dispatch_screen(void* p);
void screen_job();
void draw_screen();

#endif