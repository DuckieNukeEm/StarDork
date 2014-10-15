/* stardork.c by Ryan Kulla <rkulla@gmail.com>
 *
 * See the README for details.
 *
 */
#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/kd.h>

#define YMIN 1
#define XMIN 0
#define START_POS_Y 18
#define START_POS_X 45
#define LEVEL_MAX 30
#define HUD_START_POS 17
#define sndon(freq) ioctl(0, KIOCSOUND, 1193810 / freq)
#define sndoff() ioctl(0, KIOCSOUND, 0)

#define PROB_DOWN 's'
#define PROB_UP 'w'
#define PROB_LEFT 'a'
#define PROB_RIGHT 'd'
#define PROB_UP_RIGHT 'e'
#define PROB_UP_LEFT 'q'
#define PROB_DOWN_RIGHT 'c'
#define PROB_DOWN_LEFT 'z'
 //below is the keypad directions used when determing 'movement' and location
 //    _ _ _
 //   |1|2|3|
 //   |8|9|4|
 //   |7|6|5|


struct vehicle {
    char *ship;
    char *blank;
} aircraft = { "X", " " };

struct display {
    int power;
    int level;
    int lives; 
    int moves;
} hud = { 9, 1, 3 };

struct maze {
    char *star;
    char *wormhole;
    char *blank;
    int plant1[255];
    int plant2[255];
    int wormhole_y;
    int wormhole_x;
} space = { ".", "@", " " };

struct probe_gun {
    char *probe;
    char *blank;
    unsigned int speed;
} fire = { "+", " ", 5 };    

struct maze store[255];

void control_ship(int y, int x);
void move_ship(int *y, int *x, int input);
void shoot(int y, int x, int input);
void aftershot(int y, int x);
void probe_delay(void);
void check_if_bumped(int y, int x);
bool check_if_wormhole_is_hit(int y, int x);
void check_if_you_won(void);
void plot_stars_randomly(void);
void plot_wormhole(int y, int x);
void check_if_dead(int y, int x);
void display_hud(void);
void display_wormhole(void);
void count_moves(void);
void game_over(int y, int x);
void clear_tracks(int y, int x);
int is_out_of_bounds(int y, int x);
void change_colors(int color);
void show_stars(void);
void winner(void);
void set_signal_handler(void);
static void catch_sigint(int signo);


int main(int argc, char **argv) {
    int y, x;
    
    set_signal_handler();

    initscr(); 

    curs_set(0);
    cbreak(); 
    noecho(); 
    keypad(stdscr, TRUE); 
    scrollok(stdscr, TRUE);

    space.wormhole_y = rand() % LINES; 
    space.wormhole_x = rand() % COLS - 1;
    y = START_POS_Y;
    x = START_POS_X;
    hud.moves = 0;

    plot_stars_randomly(); 
    mvprintw(y, x, aircraft.ship); 
    control_ship(y, x);

    refresh();
    endwin(); 

    return(0);
}

void control_ship(int y, int x) {
    int input;

    change_colors(COLOR_WHITE);
    show_stars();

    while (1) {
        display_hud();
        display_wormhole();
        input = getch();
        move_ship(&y, &x, input);
        //shoot(&y, &x, input);
        shoot(y, x, input);
        check_if_bumped(y, x);
        check_if_wormhole_is_hit(y, x);
        mvprintw(2,2,"x %i", x);
        mvprintw(3,3,"y %i",y);
    } 
}

void move_ship(int *y, int *x, int input) {
    switch (input) {
        case KEY_UP: 
            if (is_out_of_bounds(*y-1, *x)) {break;}

                clear_tracks(*y, *x);
                mvprintw(--*y, *x, aircraft.ship);          
                break;
        case KEY_DOWN: 
            if (is_out_of_bounds(*y+1, *x)) {break;}

                clear_tracks(*y, *x);
                mvprintw(++*y, *x, aircraft.ship);
                break;
        case KEY_RIGHT:
            if (is_out_of_bounds(*y, *x+1)) {break;}
            
                clear_tracks(*y, *x); 
                mvprintw(*y, ++*x, aircraft.ship);
                break;
        case KEY_LEFT:
            if ((is_out_of_bounds(*y, *x-1))) {break;}
            
                clear_tracks(*y, *x); 
                mvprintw(*y, --*x, aircraft.ship);
                break;
        case KEY_HOME:
        case KEY_A1:
            if (is_out_of_bounds(*y-1, *x-1)) {break;}
            
                clear_tracks(*y, *x); 
                mvprintw(--*y, --*x, aircraft.ship);
                break;
        case KEY_PPAGE:
            if (is_out_of_bounds(*y-1,*x+1)) {break;}
            
                clear_tracks(*y, *x); 
                mvprintw(--*y, ++*x, aircraft.ship);
                break;
        case KEY_END:
        case KEY_C1:
            if (is_out_of_bounds(*y+1,*x-1)) {break;}
            
                clear_tracks(*y, *x); 
                mvprintw(++*y, --*x, aircraft.ship);
                break;
        case KEY_NPAGE:
            if (is_out_of_bounds(*y+1, *x+1)) {break;}
            
                clear_tracks(*y, *x); 
                mvprintw(++*y, ++*x, aircraft.ship);
                break;
    }
    count_moves();
}

void shoot(int y, int x, int input) {
        int xadj = 0;
        int yadj = 0;

          switch (input) {
            case PROB_UP_LEFT:      
                xadj = -1; 
                yadj = -1;               
                break;
            case PROB_UP:
                yadj = -1;
                break;
            case PROB_UP_RIGHT:
                xadj = 1;
                yadj = -1;
                break;
            case PROB_RIGHT:
                xadj = 1;
                break;
            case PROB_DOWN_RIGHT:
                xadj = 1;
                yadj = 1;
                break;
            case PROB_DOWN:
                yadj = 1;
                break;
            case PROB_DOWN_LEFT:
                xadj = -1;
                yadj = 1;
                break;
            case PROB_LEFT:
                xadj = -1;
                break;
            }

    if (xadj != 0 || yadj != 0) {
            while(YMIN < y && y < LINES && XMIN < x && x < COLS) {
                y += yadj;
                x += xadj;
                mvprintw(y, x, fire.probe);
                aftershot(y, x);
            if (check_if_wormhole_is_hit(y, x)) {break; }
                }
      }
}

void display_hud(void) {
    int i;

    for (i = 1; i <= hud.power; i++) {
        mvaddch(0, i + HUD_START_POS, '|');
    }

    mvprintw(0, i + HUD_START_POS, " ");
    mvprintw(0, 11, "Power: "); 
    mvprintw(0, 30, "Lives: %d", hud.lives);
    mvprintw(0, 40, "Level: %d", hud.level);
}

void display_wormhole(void) {
    mvprintw(space.wormhole_y, space.wormhole_x, space.wormhole);
}

void count_moves(void) {
    hud.moves++;
    mvprintw(0, 50, "Moves: %d", hud.moves);
}


void aftershot(int y, int x) {
    probe_delay();
    mvprintw(y, x, fire.blank);
    show_stars();
}

void probe_delay(void) {
    napms(fire.speed);
    refresh();
}

bool check_if_wormhole_is_hit(int y, int x) {
    if (y == space.wormhole_y && x == space.wormhole_x) {
        plot_wormhole(y, x);
        hud.level++;
        plot_stars_randomly(); 
        check_if_you_won();
        return true;
    }
    return false;
}

void plot_wormhole(y, x) {
    int r1, r2, i, j;

    r1 = rand() % LINES -1 ;
    r1++;
    r2 = rand() % COLS;
    
    // Avoid plotting wormhole where stars currently exist:
    for (i = 0; i < COLS; i++) {
        for (j = 0; j < COLS; j++) {
            if ((r1 == store[j].plant1[i] && r2 == store[j].plant2[i]) || r1 == 0) {
                plot_wormhole(y, x);
                return; // end recursion
            }
        }
    }

    space.wormhole_y = r1;
    space.wormhole_x = r2;
}

void plot_stars_randomly(void) {
    int r1, r2, i; 
    static int star_plant = 0;

    srand((unsigned)time(NULL));
    for (i = 1; i < COLS; i++) {
        r1 = rand() % LINES -1 ; // adding +1 avoids 0 so it doesn't show stars over hud
        r1++; // the +1 above doesn't work cause it can knock it over the # of columns!
        r2 = rand() % COLS; 
        store[star_plant].plant1[i] = r1;
        store[star_plant].plant2[i] = r2;
    }

    star_plant++;
}

void show_stars(void) {
    int plant, i;

    for (i = 0; i < COLS; i++) {
        for (plant = 0; plant < COLS; plant++) {
            mvprintw(store[plant].plant1[i], store[plant].plant2[i], space.star);
        }
    }
}

void check_if_you_won(void) {
    if (hud.level == LEVEL_MAX)
         winner(); 
}

void check_if_bumped(int y, int x) {
    int c, i;
 
    for (i = 0; i < COLS; i++) {
        for (c = 0; c < COLS; c++) {
            if (y == store[c].plant1[i] && x == store[c].plant2[i]) {
                mvprintw(y, x, aircraft.ship, A_BOLD);
                change_colors(COLOR_RED);
                getch();
                hud.power--;  
            }
        }
    }
    change_colors(COLOR_WHITE);
    check_if_dead(y, x);
}

void check_if_dead(int y, int x) {
    if (hud.power < 1) { 
        hud.lives--;
        hud.power = 9; 
    
        if (hud.lives < 1) {
            game_over(y, x);
        }
    }
    show_stars();
}

void game_over(int y, int x) {
    int i = 1, j = LINES, fr;

    attrset(A_BOLD);
    mvprintw(y, x, aircraft.ship);
    getch();
    mvprintw(y, x, space.blank);

    srand((unsigned)time(NULL));
    while (i < (LINES / 2 - 2)) {
        ++i;
        j--;
        fr = (rand() % 150) + 100;
        sndon(fr);
        mvprintw(i, (COLS / 2 - 30), space.blank);
        mvprintw(i + 1, (COLS / 2 - 30), "G");
        mvprintw(i, (COLS / 2 - 20), space.blank);
        mvprintw(i + 1, (COLS / 2 - 20), "A");
        mvprintw(i, (COLS / 2 - 10), space.blank);
        mvprintw(i + 1, (COLS / 2 - 10), "M");
        mvprintw(i, (COLS / 2), space.blank);
        mvprintw(i + 1, (COLS / 2), "E");

        mvprintw(j, (COLS / 2 - 30), space.blank);
        mvprintw(j - 1, (COLS / 2 - 30), "O");
        mvprintw(j, (COLS / 2 - 20), space.blank);
        mvprintw(j - 1, (COLS / 2 - 20), "V");
        mvprintw(j, (COLS / 2 - 10), space.blank);
        mvprintw(j - 1, (COLS / 2 - 10), "E");
        mvprintw(j, (COLS / 2), space.blank);
        mvprintw(j - 1, (COLS / 2), "R");
        napms(100);
        refresh();
    }

    sndoff();
    nocbreak();
    getch();
    attrset(A_NORMAL);
    refresh();
    flushinp();
    endwin();
    exit(0);
}

int is_out_of_bounds(int y, int x) {
    if ( y == YMIN - 1 || y == LINES || x == XMIN - 1 || x == COLS) {
        return 1;
    } else {
        return 0;
    }
}


void winner(void) {
    WINDOW *win = newwin(0, 0, 0, 0);
    char *logname = getenv("LOGNAME");
    char ending[] = "You are a true stardork.";
    char won_msg[255];
    int fr, i;
    fr = i = 0;

    wattrset(win, A_BOLD);
    box(win, ' ', ' ');
    sprintf(won_msg, "You win %s, in %d moves", (logname ? logname : " "), hud.moves);
    mvwprintw(win, 4, (COLS / 2) - strlen(won_msg) / 2, "%s", won_msg);

    srand((unsigned)time(NULL));
    for (i = 0; i < sizeof(ending); i++) {
        fr = (rand() % 550) + 100;
        sndon(fr);
        mvwprintw(win, 12, i + ((COLS / 2) - 12), "%c", ending[i]);
        napms(200);
        wrefresh(win);
    }
    sndoff();
    wgetch(win);
    wattrset(win, A_NORMAL);
    wrefresh(win);
    flushinp();
    endwin();
    exit(0);
}

void clear_tracks(int y, int x) {
    mvprintw(1,1,"clearing tracks");
    mvprintw(y, x, aircraft.blank);
}

void change_colors(color) {
    if (has_colors() != FALSE) { 
        start_color();
        init_pair(1, color, COLOR_BLACK);
        attron(COLOR_PAIR(1));
    }
}

void set_signal_handler(void) {
    struct sigaction sa_old, sa_new;

    sa_new.sa_handler = catch_sigint;
    sigemptyset(&sa_new.sa_mask);
    sa_new.sa_flags = 0;
    sigaction(SIGINT, &sa_new, &sa_old);
}

static void catch_sigint(int signo) {
    mvprintw(5, 10, "Bye Bye!\n", 11);
    attrset(A_NORMAL);
    refresh();
    napms(1000);
    endwin();
    exit(0);
}

