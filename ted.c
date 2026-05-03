#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <ncurses.h>


// ## BASE GAPBUFFER


struct _gapbuf {
    char* buf; // where the buffer starts
    char* pre_gap; // where the gap starts
    unsigned int pre_gap_size;
    
    char* post_gap; // where the post gap starts
    unsigned int post_gap_size;
    unsigned int capacity;
};

typedef struct _gapbuf gapbuf;
typedef struct _gapbuf gapbuf_t;

gapbuf* gapbuf_new() {
    gapbuf* g = malloc(sizeof(gapbuf));
    g->buf = malloc(sizeof(char) * 10);
    memset(g->buf, 0, sizeof(char) * 10);
    g->capacity = 10;
    g->pre_gap = g->buf + 1;
    g->post_gap = g->buf + g->capacity - 2;
    g->pre_gap_size = 0;
    g->post_gap_size = 0;
    return g;
}

gapbuf* gapbuf_dup(gapbuf* g) {
    gapbuf* gdup = malloc(sizeof(gapbuf));
    gdup->buf = malloc(sizeof(char) * g->capacity);
    memset(gdup->buf, 0, sizeof(char) * g->capacity);
    gdup->capacity = g->capacity;
    gdup->pre_gap_size = g->pre_gap_size;
    gdup->post_gap_size = g->post_gap_size;
    gdup->pre_gap = gdup->buf + gdup->post_gap_size + 1;
    gdup->post_gap = gdup->buf + gdup->capacity - gdup->post_gap_size - 2;
    memcpy(gdup->buf, g->buf, g->capacity);
    return gdup;
}

void gapbuf_putchar(gapbuf* g, char ch) {
    if (g->post_gap - g->pre_gap == 1) {
        g->buf = realloc(g->buf, g->capacity * 2);
        int old_capacity = g->capacity;

        g->capacity = g->capacity * 2;
        g->pre_gap = g->buf + g->pre_gap_size + 1;
        char* old_post_gap = g->buf + old_capacity - g->post_gap_size - 2;
        g->post_gap = g->buf + g->capacity - g->post_gap_size - 2;
        memcpy(g->post_gap, old_post_gap, g->post_gap_size + 1);
    }
    *g->pre_gap = ch;
    g->pre_gap++;
    g->pre_gap_size++;
}

int gapbuf_delete_char(gapbuf* g) {
    if (g->post_gap - g->buf < g->capacity - 2) {
        g->post_gap++;
        g->post_gap_size--;
    }
    return *(g->post_gap - 1);
} 

int gapbuf_backspace_char(gapbuf* g) {
    if (g->pre_gap - g->buf > 1) {
        g->pre_gap--;
        g->pre_gap_size--;
    }
    return *(g->pre_gap + 1);
} 

gapbuf* gapbuf_split_to_new(gapbuf* g) {
    gapbuf* gnew = gapbuf_dup(g);
    gnew->pre_gap = gnew->buf + 1;
    gnew->pre_gap_size = 0;

    g->post_gap = g->buf + g->capacity - 2;
    g->post_gap_size = 0;
    return gnew;
}

void gapbuf_move_all_to_pregap(gapbuf* gb) {
    // if (gb->pre_gap_size == 0)
    //     return;
    // if (gb->post_gap_size == 0)
    //     return;
    memcpy(gb->pre_gap, gb->post_gap, gb->post_gap_size);
    gb->pre_gap_size += gb->post_gap_size;
    gb->post_gap_size = 0;
    gb->pre_gap += gb->post_gap_size;
    gb->post_gap = gb->buf + gb->capacity - 2;
}

char* gapbuf_realloc_raw(gapbuf* gb, unsigned int size) {
    unsigned long long postgapdiff = gb->post_gap - gb->buf;
    unsigned long long pregapdiff = gb->pre_gap - gb->buf;
    int prevsz = gb->capacity;
    gb->buf = realloc(gb->buf, size);
    gb->pre_gap = gb->buf + pregapdiff;
    gb->post_gap = gb->buf + postgapdiff;
    memset(gb->buf + gb->capacity - 1, 0, size - prevsz);
    return gb->buf +  gb->capacity - 1; // returns end of buffer
}

#define nmax(x, y) ((x) > (y))?(x):(y) 

void gapbuf_merge(gapbuf* g1, gapbuf* g2) {
    // get pdiff of g1 post gap

    // g2 total size
    unsigned int g2_totrsize = g2->pre_gap_size + g2->post_gap_size;
    
    // realloc while keeping metadata same
    char* endbf = gapbuf_realloc_raw(g1, g1->capacity + g2_totrsize);
    
    // move g1 to pregap
    gapbuf_move_all_to_pregap(g1);

    // // copy data
    memcpy(endbf, g2->buf + 1, g2->pre_gap_size);
    memcpy(endbf + g2->pre_gap_size - 1, g2->post_gap, g2->post_gap_size);

    // update metadata
    g1->post_gap_size += (g2->pre_gap_size + g2->post_gap_size);
    g1->capacity += g2_totrsize;    
}

#define MOVE_LEFT -1
#define MOVE_RIGHT 1

void gapbuf_move_cursor(gapbuf* gb, char dir) {
    if (dir == MOVE_LEFT &&
        (gb->pre_gap_size == 0))
        return;
    if (dir == MOVE_RIGHT && 
        (gb->post_gap_size == 0))
        return;
    
    switch(dir) {
        case MOVE_LEFT:
        gb->post_gap_size++;
        gb->pre_gap_size--;

        gb->post_gap--;
        gb->pre_gap--;
        *gb->post_gap = *gb->pre_gap;
        break;
    case MOVE_RIGHT:
        *gb->pre_gap = *gb->post_gap;
        gb->pre_gap++;
        gb->post_gap++;

        gb->pre_gap_size++;
        gb->post_gap_size--;
        break;
    }
}


void gapbuf_jump_cursor(gapbuf* gb, int len, char dir) {
    if (dir == MOVE_LEFT &&
        (gb->pre_gap_size == 0))
        return;
    if (dir == MOVE_RIGHT && 
        (gb->post_gap_size == 0))
        return;

    
    switch(dir) {
        case MOVE_LEFT:
        if (gb->pre_gap_size - len <= 0) {
            len = gb->pre_gap_size;
        }

        gb->post_gap_size += len;
        gb->pre_gap_size -= len;

        gb->post_gap -= len;
        gb->pre_gap -= len;
        memcpy(gb->post_gap, gb->pre_gap, len);
        break;
    case MOVE_RIGHT:
        if (gb->post_gap_size - len <= 0) {
            len = gb->post_gap_size;
        }
        memcpy(gb->pre_gap, gb->post_gap, len);
        gb->pre_gap += len;
        gb->post_gap += len;

        gb->pre_gap_size += len;
        gb->post_gap_size -= len;
        break;
    }
}

void gapbuf_printw(WINDOW* win, gapbuf_t* gb, char SHOWCURSOR) {
    // gb->buf[gb->gapstart] = 0;
    *gb->pre_gap = 0;
    if (SHOWCURSOR)
        wprintw(win, "%s|%s", gb->buf + 1, gb->post_gap);
    else
        wprintw(win, "%s%s", gb->buf + 1, gb->post_gap);

    wclrtoeol(win);
}

void gapbuf_fprintf(gapbuf_t* gb, FILE* fp) {
    *gb->pre_gap = 0;
    fprintf(fp, "%s%s", gb->buf + 1, gb->post_gap);
}


char alphanumeric(char ch) {
    return ((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z')) || ((ch >= '0') && (ch <= '9')) || ch == '_';
}

int toklen(char* ptr, char dir) {
    char d = (dir == MOVE_LEFT)?-1:1;
    int tklen = 0;
    if (*ptr != 0 && (*ptr == ' ' || *ptr == '\t')) {
        while (*ptr != 0 && (*ptr == ' ' || *ptr == '\t')) {
            ptr+=d;
            tklen++;
        }
        return tklen;
    }
    else if (*ptr != 0 && alphanumeric(*ptr)) {
        while (*ptr != 0 && alphanumeric(*ptr)) {
            ptr+=d;
            tklen++;
        }
        return tklen;
    }
    else if (*ptr != 0) {
        ptr += d;
        return 1;
    }
    return 0;
}



// ## LINES AND LINEOPS



struct line_t;

typedef struct line_t {
    gapbuf_t* ldata;
    char curr;
    char idx;
    struct line_t* prev;
    struct line_t* next;
} line_t;

line_t* view_line_start;

line_t* curr_cursor_line;

int curr_cursor_line_num;

int cursor_view_lines = 20;

void print_ll(line_t* ll) {
    fprintf(stderr, "E\n");
    while (ll->next) {
        fprintf(stderr, ":%p:\n", ll);
        ll = ll->next;
    }
    fprintf(stderr, ":%p:\n", ll);
    fprintf(stderr, "E\n");
}

void delete_lines(line_t* lb) {
    while (lb->next) {
        line_t* lbnext = lb->next;
        free(lb->ldata->buf);
        free(lb->ldata);
        free(lb);
        lb = lbnext;
    }
    free(lb->ldata->buf);
    free(lb->ldata);
    free(lb);
}

line_t* line_new_and_move(line_t* prev_line) {
    if (!prev_line) {
        curr_cursor_line = malloc(sizeof(line_t));
        curr_cursor_line->ldata = gapbuf_new();
        curr_cursor_line->next = NULL;
        curr_cursor_line->prev = NULL;
        curr_cursor_line->curr = 1;
        curr_cursor_line->idx = 0;
        return curr_cursor_line;
    }
    
    line_t* prev_line_nxt = prev_line->next;
    
    line_t* nl = malloc(sizeof(line_t));
    
    prev_line->next = nl;
    nl->next = prev_line_nxt;

    if (prev_line_nxt)
        prev_line_nxt->prev = nl;
    nl->prev = prev_line;

    nl->ldata = gapbuf_new();

    prev_line->curr = 0;
    prev_line->next->curr = 1;
    curr_cursor_line = prev_line->next;
    
    if (curr_cursor_line_num < cursor_view_lines)
        curr_cursor_line_num++;
    else {
        if (view_line_start->next)
            view_line_start = view_line_start->next;
    }
    
    return prev_line->next;
}


void line_cursor_move_up() {
    if (curr_cursor_line->prev){
        if (curr_cursor_line_num > 0) {
            curr_cursor_line_num--;
        }
        else {
            if (view_line_start->prev)
                view_line_start = view_line_start->prev;
        }
        curr_cursor_line->curr = 0;
        curr_cursor_line = curr_cursor_line->prev;
        curr_cursor_line->curr = 1;
    }
}

void line_cursor_move_down() {
    if (curr_cursor_line->next) {
        if (curr_cursor_line_num < cursor_view_lines)
            curr_cursor_line_num++;
        else {
            if (view_line_start->next)
                view_line_start = view_line_start->next;
        }
        curr_cursor_line->curr = 0;
        curr_cursor_line = curr_cursor_line->next;
        curr_cursor_line->curr = 1;
    }
}

void line_cursor_move_left(int jump_mod) {
    if (curr_cursor_line->ldata->pre_gap == curr_cursor_line->ldata->buf + 1)// == curr_cursor_line->ldata->cap)
        line_cursor_move_up();
    else {
        if (!jump_mod)
            gapbuf_move_cursor(curr_cursor_line->ldata, MOVE_LEFT);
        else
            gapbuf_jump_cursor(curr_cursor_line->ldata, 
                toklen(curr_cursor_line->ldata->pre_gap - 1, MOVE_LEFT), 
                MOVE_LEFT);
    }
}

void line_cursor_move_right(int jump_mod) {
    if (curr_cursor_line->ldata->post_gap == curr_cursor_line->ldata->buf + curr_cursor_line->ldata->capacity - 2)
        line_cursor_move_down();
    else {
        if (!jump_mod)
            gapbuf_move_cursor(curr_cursor_line->ldata, MOVE_RIGHT);
        else
            gapbuf_jump_cursor(curr_cursor_line->ldata, 
                toklen(curr_cursor_line->ldata->post_gap, MOVE_RIGHT), 
                MOVE_RIGHT);
    }
}

void line_del_and_merge() {
    // delete the current line up by 1 and merge the gapbufs of the curr line with prev line;

    // save ptr
    line_t* l_to_delete = curr_cursor_line;
    line_t* before = curr_cursor_line->prev;
    line_t* after = curr_cursor_line->next;
    if (!before)
        return;

    
    before->next = after;
    
    if (after)
        after->prev = before;

    gapbuf_merge(before->ldata, l_to_delete->ldata);
    
    if (l_to_delete->next == NULL)
        l_to_delete->prev->next = NULL;

    free(l_to_delete->ldata->buf);
    free(l_to_delete->ldata);
    free(l_to_delete);
    
    if (curr_cursor_line_num > 0)
        curr_cursor_line_num--;
    else {
        if (view_line_start->prev)
            view_line_start = view_line_start->prev;
    }

    curr_cursor_line = before;
    curr_cursor_line->curr = 1;
}

char line_onlastchar() {
    return curr_cursor_line->ldata->post_gap == curr_cursor_line->ldata->buf + curr_cursor_line->ldata->capacity - 2;
}

char line_onfirstchar() {
    return curr_cursor_line->ldata->pre_gap == curr_cursor_line->ldata->buf + 1;
}

void line_split() {
    line_new_and_move(curr_cursor_line);
    free(curr_cursor_line->ldata->buf);
    free(curr_cursor_line->ldata);
    curr_cursor_line->ldata = gapbuf_split_to_new(curr_cursor_line->prev->ldata);

}

void line_putc(char ch) {
    gapbuf_putchar(curr_cursor_line->ldata, ch);
}

int line_bkspc() {
    if (line_onfirstchar()) {
        line_del_and_merge();
        return 1;
    }
    else {
        return gapbuf_backspace_char(curr_cursor_line->ldata);
    }
}

int line_delc() {

    if (line_onlastchar()) {
        line_cursor_move_down();
        line_del_and_merge();
        return 1;
    }
    else
        return gapbuf_delete_char(curr_cursor_line->ldata);
}


void print_contents(WINDOW* win, line_t* line_head) {

    wmove(win, 0, 0);
    int i = 0;
    line_t* vls = view_line_start;
    while(vls->next && i < cursor_view_lines) {
        gapbuf_printw(win, vls->ldata, vls->curr);
        wprintw(win, "\n");
        vls = vls->next;
        i++;
    }
    gapbuf_printw(win, vls->ldata, vls->curr);
    wprintw(win, "\n");
    wclrtoeol(win);

}

void fprint_contents(line_t* line_head, char* fpath) {

    FILE* fp = fopen(fpath, "w");

    while(line_head->next) {
        gapbuf_fprintf(line_head->ldata, fp);
        fprintf(fp, "\n");
        line_head = line_head->next;
    }
    gapbuf_fprintf(line_head->ldata, fp);
    fclose(fp);
}



// ## MAIN



int tabs = 0;

#define CTRL(x) ((x) & 37)
#include <unistd.h>

char* fname;

void parse_args(int argc, char** argv) {
    char* b = getcwd(NULL, 512 * sizeof(char));
    fname = malloc(1024 * sizeof(char));
    int i = 0;
    char** curr_arg = NULL;
    int stdwin = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-file") == 0) {
            i += 1;
            if (i >= argc) {
                printf("ERR:\tNo file name provided.\n");
                exit(1);
            }
            else if (argv[i][0] == '-') {
                printf("ERR:\tInvalid filename\n");
                exit(1);
            }

            if (argv[i][0] == '.' && argv[i][1] != 0 && argv[i][1] == '/') {
                sprintf(fname, "%s/%s", b, &argv[i][2]);
                i+=1;
                continue;
            }
            
            if (argv[i][0] == '/') {
                sprintf(fname, "%s", argv[i]);
                i+=1;
                continue;
            }

            sprintf(fname, "%s/%s", b, argv[i]);
            i+=1;
            continue;
        }
        if (strcmp(argv[i], "-nostdwin") == 0) {
            stdwin = 0;
            i += 1;
            continue;
        }
        i+=1;
        continue;
    }
}

#define KEY_CTRL_O 0xffff
#define KEY_CTRL_X 0xfffe

int main(int argc, char** argv) {
    parse_args(argc, argv);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    line_t* lbuf = line_new_and_move(NULL);
    view_line_start = lbuf;
    curr_cursor_line_num = 0;

    cursor_view_lines = getmaxy(stdscr);
    print_contents(stdscr, lbuf);

    define_key("\x0f", KEY_CTRL_O);
    define_key("\x18", KEY_CTRL_X);
    char runloop = 1;
    while (runloop) {
        int c = getch();

        switch(c) {
            case KEY_CTRL_X:
                runloop = 0;
                break;
            case KEY_CTRL_O:
                fprint_contents(lbuf, fname);
                continue;
            case KEY_UP:
                line_cursor_move_up();
                break;
            case KEY_DOWN:
                line_cursor_move_down();
                break;
            case KEY_LEFT:
                line_cursor_move_left(0);
                break;
            case KEY_RIGHT:
                line_cursor_move_right(0);
                break;
            case KEY_SLEFT:
                line_cursor_move_left(1);
                break;
            case KEY_SRIGHT:
                line_cursor_move_right(1);
                break;
            case KEY_BACKSPACE:
                int delchar = 0;
                delchar = line_bkspc();
                
                if ((char)delchar == '{'||
                    (char)delchar == '['||
                    (char)delchar == '(') {
                        if (tabs > 0)
                            tabs--;
                    }
                if ((char)delchar == '}'||
                    (char)delchar == ']'||
                    (char)delchar == ')') {
                        tabs++;
                    }
                break;
            case KEY_DC:
                line_delc();
                break;
            case '{':
            case '[':
            case '(':
                tabs++;
                line_putc((char)c);
                break;
            case '}':
            case ']':
            case ')':
                if (tabs > 0)
                    tabs--;
                line_putc((char)c);
                break;
            case '\n':
                if (line_onlastchar())
                    line_new_and_move(curr_cursor_line);
                else
                    line_split();
                for (int i = 0; i < tabs; i++)
                    line_putc('\t');
                break;
            default:  
                line_putc((char)c);

        }
        print_contents(stdscr, lbuf);
        refresh();
    }
    delete_lines(lbuf);
    endwin();
}