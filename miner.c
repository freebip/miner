#include "libbip.h"
#include "miner.h"

struct regmenu_ menu_screen = { 55, 1, 0, dispatch_screen, keypress_screen, screen_job, 0, show_screen, 0, 0 };
struct appdata_t** appdata_p;
struct appdata_t* appdata;

int main(int p, char** a)
{
    show_screen((void*)p);
}

unsigned short randint(short max)
{
    appdata->randseed = appdata->randseed * get_tick_count();
    appdata->randseed++;
    return ((appdata->randseed >> 16) * max) >> 16;
}

int is_mine_exist(int x, int y)
{
    if (x < 0 || x > 5 || y < 0 || y > 4)
        return 0;
    return (appdata->board[6 * y + x] & MINE_FLAG) == MINE_FLAG;
}

void init_new_board()
{
    _memclr(appdata->board, sizeof(appdata->board));
    appdata->mines = MINS_COUNT_FROM + randint(MINS_COUNT_TO - MINS_COUNT_FROM + 1);
    for (int i = 0; i < appdata->mines; i++)
    {
        int x = randint(6);
        int y = randint(5);
        
        // mine not set?
        if (!appdata->board[6 * y + x])
            // set mine flag
            appdata->board[6 * y + x] = MINE_FLAG;
        else
            i--;
    }

    // calc mine count around each cell
    for (int i = 0;i < 30;i++)
    {
        if (appdata->board[i] == MINE_FLAG)
            continue;

        int x = i % 6;
        int y = i / 6;

        int near_mines = 0;
        near_mines += is_mine_exist(x - 1, y - 1);
        near_mines += is_mine_exist(x, y - 1);
        near_mines += is_mine_exist(x + 1, y - 1);
        near_mines += is_mine_exist(x - 1, y);
        near_mines += is_mine_exist(x + 1, y);
        near_mines += is_mine_exist(x - 1, y + 1);
        near_mines += is_mine_exist(x, y + 1);
        near_mines += is_mine_exist(x + 1, y + 1);
        appdata->board[i] |= near_mines;
    }
}

void show_screen(void* p)
{
    appdata_p = (struct appdata_t**)get_ptr_temp_buf_2();

    if ((p == *appdata_p) && get_var_menu_overlay()) {
        appdata = *appdata_p;
        *appdata_p = (struct appdata_t*)NULL;
        reg_menu(&menu_screen, 0);
        *appdata_p = appdata;
    }
    else {
        reg_menu(&menu_screen, 0);
        *appdata_p = (struct appdata_t*)pvPortMalloc(sizeof(struct appdata_t));
        appdata = *appdata_p;
        _memclr(appdata, sizeof(struct appdata_t));
        appdata->proc = (Elf_proc_*)p;

        appdata->randseed = get_tick_count();
        appdata->state = STATE_STARTING;
        init_new_board();
    }

    if (p && appdata->proc->elf_finish)
        appdata->ret_f = appdata->proc->elf_finish;
    else
        appdata->ret_f = show_watchface;

    draw_screen();

    // не выключаем экран, не выключаем подсветку
    set_display_state_value(8, 1);
    set_display_state_value(4, 1);
    set_display_state_value(2, 0);

    set_update_period(1, 1000);
}

void screen_job()
{
    draw_screen();
    repaint_screen_lines(0, 176);

    if (appdata->state == STATE_PLAY || appdata->state == STATE_PLAY_O)
        appdata->time++;

    set_update_period(1, 1000);
}

void open_near_cells(int x, int y)
{
    if (x < 0 || x > 5 || y < 0 || y > 4)
        return;
    
    if (appdata->board[6 * y + x])
    {
        appdata->board[6 * y + x] |= OPEN_CELL_FLAG;
        return;
    }

    appdata->board[6 * y + x] |= OPEN_CELL_FLAG;
    open_near_cells(x - 1, y - 1);
    open_near_cells(x, y - 1);
    open_near_cells(x + 1, y - 1);
    open_near_cells(x - 1, y);
    open_near_cells(x + 1, y);
    open_near_cells(x - 1, y + 1);
    open_near_cells(x, y + 1);
    open_near_cells(x + 1, y + 1);
}

int dispatch_screen(void* p)
{
    struct gesture_* gest = (struct gesture_*)p;

    if (gest->gesture != GESTURE_CLICK)
        return 0;

    if (gest->touch_pos_x > 117 && gest->touch_pos_y < 30)
    {
        appdata->state = STATE_STARTING;
        appdata->time = 0;
        init_new_board();
        draw_screen();
        repaint_screen_lines(0, 176);
        return 0;
    }

    if (gest->touch_pos_y < 30)
        return 0;

    if (appdata->state == STATE_WIN || appdata->state == STATE_LOSE)
        return 0;

    appdata->state = STATE_PLAY_O;

    int x = gest->touch_pos_x / 29;
    x = x > 5 ? 5 : x;
    int y = (gest->touch_pos_y-30) / 29;
    y = y > 4 ? 4 : y;

    // is open flag and mina flag?
    if (appdata->board[6 * y + x] & MINE_FLAG )
    {
        // boom
        appdata->state = STATE_LOSE;
        appdata->board[6 * y + x] |= OPEN_CELL_FLAG;
    }
    else
    {
        open_near_cells(x, y);

        // check win
        int closed_cells = 0;
        for (int i = 0; i < 30; i++)
        {
            if (!(appdata->board[i] & OPEN_CELL_FLAG))
                closed_cells++;
        }

        if (closed_cells == appdata->mines)
            appdata->state = STATE_WIN;
    }

    draw_screen();
    repaint_screen_lines(0, 176);
    return 0;
}

void keypress_screen()
{
    show_menu_animate(appdata->ret_f, (unsigned int)show_screen, ANIMATE_RIGHT);
};

void print_digits(int value, int x, int y, int spacing)
{
    struct res_params_ res_params;

    int max = 100000;

    while (max)
    {
        if (value / max)
            break;
        max = max / 10;
    }

    do
    {
        int mm = max == 0 ? 0 : value / max;

        get_res_params(0, mm, &res_params);
        show_elf_res_by_id(0, mm, x, y);
        x += res_params.width + spacing;
        if (max == 0)
            break;

        value = value % max;
        max = max / 10;
    } while (max);
}

void draw_screen()
{
    set_graph_callback_to_ram_1();
    set_fg_color(COLOR_BLACK);
    set_bg_color(COLOR_YELLOW);
    fill_screen_bg();

    show_elf_res_by_id(0, RES_MINA, 6, 9);
    print_digits(appdata->mines, 25, 8, 1);
    show_elf_res_by_id(0, RES_CLOCK, 47, 9);
    print_digits(appdata->time, 68, 8, 1);

    for (int i = 30; i < 176; i += 29)
    {
        draw_horizontal_line(i, 0, 176);
        draw_vertical_line(i, 30, 176);
    }
    draw_filled_rect(0, 0, 2, 176);
    draw_rect(117, 0, 175, 30);
    draw_filled_rect(119, 2, 174, 29);

    switch (appdata->state)
    {
        case STATE_STARTING:
        case STATE_PLAY:
            show_elf_res_by_id(0, RES_SMILE, 137, 5);
            break;
        case STATE_WIN:
            show_elf_res_by_id(0, RES_SMILE_OK, 137, 5);
            break;
        case STATE_LOSE:
            show_elf_res_by_id(0, RES_SMILE_BAD, 137, 5);
            break;
        case STATE_PLAY_O:
            show_elf_res_by_id(0, RES_SMILE_O, 137, 5);
            appdata->state = STATE_PLAY;
            break;
    }

    for (int i = 0; i < 30; i++)
    {
        int x = i % 6;
        int y = i / 6;

        // boom?
        if ((appdata->board[i] & (MINE_FLAG | OPEN_CELL_FLAG)) == (MINE_FLAG | OPEN_CELL_FLAG))
        {
            set_fg_color(COLOR_RED);
            draw_filled_rect(2 + 29 * x, 31 + 29 * y, 2 + 29 * x + 29, 31 + 29 * y + 29);
        }
        // open cell or game over and mine exist?
        else if ( (appdata->board[i] & OPEN_CELL_FLAG) ||
            (appdata->state == STATE_LOSE) & ((appdata->board[i] & MINE_FLAG) == MINE_FLAG))
        {
            set_fg_color(COLOR_YELLOW);
            draw_filled_rect(3 + 29 * x, 32 + 29 * y, 2 + 29 * x + 27, 31 + 29 * y + 27);
        }
        else
        {
            set_fg_color(COLOR_BLACK);
            draw_filled_rect(3 + 29 * x, 32 + 29 * y, 2 + 29 * x + 27, 31 + 29 * y + 27);
        }

        if (appdata->board[i] & OPEN_CELL_FLAG)
        {
            int value = appdata->board[i] & 0xf;
            if (value)
            {
                struct res_params_ res_params;
                get_res_params(ELF_INDEX_SELF, value, &res_params);
                show_elf_res_by_id(ELF_INDEX_SELF, value, 16 - res_params.width/2 + 29 * x, 38 + 29 * y);
            }
        }

        if (appdata->state == STATE_LOSE || appdata->state == STATE_WIN)
        {
            if ((appdata->board[i] & MINE_FLAG) == MINE_FLAG)
                show_elf_res_by_id(0, RES_BIG_MINA, 2 + 29 * x, 31 + 29 * y);
        }

    }

}
