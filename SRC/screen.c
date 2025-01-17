#include "common.h"
#ifdef Screen
char * prompt_chars;

#ifdef TURBOC

#include "screen.h"
#include "buffer.h"
#include <ctype.h>

static char col_changed[ROWS+1];
void touch_all(NONE);
void shift_up(INT X INT);   /* (a,b) shift up lines a..b by one */
void shift_dn(INT X INT);   /* (a,b) shift dn lines a..b by one */
void show_mark(STR);
void set_mark(NONE);
void help_screen(NONE);

#define MARGIN 4
#define TOUCH(x,y)      col_changed[y] = min(col_changed[y],x)
#define FIX(y)          col_changed[y] = COLS+1
#define ROW_OK(y)       col_changed[y] == COLS+1

void GOTOXY(INT X INT);
void go_dn(NONE);
void go_up(NONE);
void go_lt(NONE);
void go_rt(NONE);
void go_home(NONE);
void go_end(NONE);
void go_TOP(NONE);
void go_EOB(NONE);
void go_mark(NONE);
void sys(STR);
void l_refresh(INT X STR);
void s_refresh(NONE);
void delunder(NONE);
int  wget(INT X INT);
void inschar(CHAR);
void delchar(NONE);
void breakline(BOOL);
void write_region(NONE);
void read_region(NONE);
void copy_region(NONE);
void clear_msg(NONE);
void get_info(NONE);
void msg(STR);
void show_match(NONE);

int at_x, at_y;
char * dp, *end_dp;
Bool do_refresh = true;

Menu basicM
	    = { "Copy region",          copy_region,
		"Save region",          write_region,
		"Read file",            read_region,
		"save Buffer",          save_buff,
		"Quit",                 q_exit,
		"buffer Info",          get_info,
		"Help",                 help_screen,
                0,0
              };

void Einit(size) int size;{
    int i;
    clrscr();
    normvideo();
    textattr(FORE(BLACK) + BACK(WHITE));


    bwindow(LEFT, TOP,  LEFT+COLS-1, TOP+ROWS, "  ISETL 2.1  ");
      /* extra row at bottom for messages */
    current_buffer = new_buffer(size);
    B_ROWS = ROWS;
    B_COLS = COLS;
    for( i= 1 ; i<=ROWS; i++ ) FIX(i);
    at_x = 1; at_y = 1;
    dp = NULL;
}



#include <assert.h>
Bool v_move = false;

char Egetc(){
    static Bool data_sent = true;
    static Bool data_start= true;     /* at front of line in dp */
    int c;

#ifdef GRAPHICS
    static Bool last_graphics=false;
    extern Bool Graphics;
    if( Graphics ) {
	last_graphics = true;
	return getch();
    }
    if( last_graphics ) {
	last_graphics = false;
        clrscr();
	touch_all();
	recenter();
    }
#endif

    while( dp == NULL && data_start ) {

        if( data_sent ) {
            if( cur_char==EOS) {
                char * old = DEF_MARK;
                DEF_MARK = cur_first(CURSOR);
                show_mark(old);
                show_mark(DEF_MARK);
            }
            recenter();
            data_sent = false;
        }

#ifdef TESTING
/* Only used when debugging the editor */
	assert( START <= WSTART );
        assert(WSTART <= CURSOR );
        assert(CURSOR <  GAP);
        assert( line_count() <= ROWS );
	assert(wherex()==at_x || (wherex()==COLS && at_x>=COLS) );
        assert(wherey()==at_y);
#endif
        v_move = false;
        if( GOAL_COL == -1 ) GOAL_COL = at_x;

        c = getch();
        if( c == 0 ) { /* special char */
            c = getch();
            special(c);
        } else if( isprint(c) ) {
            inschar(c); 
            switch(c) {
              case ')':
              case '}':
              case ']': show_match();
            }

        }       else switch(c) {
            case '\n'      :
            case '\r'      :
                DEF_MARK = cur_first(DEF_MARK);
                if( DEF_MARK <= cur_last(CURSOR) ) { /* send data */
                    go_end();

                    if( cur_char==EOS ){ /* insert '\n' so that it is sent */
                        breakline(false);
                    }
                    dp = DEF_MARK; end_dp = cur_last(CURSOR);

                    go_EOB();
                    if( *(CURSOR-1) != '\n' ) { /* insert '\n' at EOB */
                        Eputc('\n');
                    }

                    LAST_FRONT = dp;
                    LAST_BACK  = end_dp;
                    if( *LAST_BACK == EOS && LAST_BACK != START) {
                        LAST_BACK --;
                    }
                    data_sent  = true;
                    data_start = true;
                    no_prompt(true);
                } else {
                    breakline(true);
                }
                break;

            case ctrl('a') : {
                char * old = DEF_MARK;
                DEF_MARK = LAST_FRONT;
                CURSOR   = LAST_BACK;
                recenter();
                show_mark(old);
                show_mark(DEF_MARK);
                } break;
            case ctrl('b') : { CURSOR = LAST_FRONT;
                               recenter();
                             } break;
	    case ctrl('e') : { line_del();
			       delline();
			       shift_up(at_y,ROWS);
			       TOUCH(0,at_y);
			       recenter();
                             } break;
            case ctrl('l') : clrscr();
                             touch_all();
                             recenter();
                             break;
            case ctrl('t') : set_mark();        break;
            case ctrl('x') : go_mark();         break;
            case ctrl('z') : sys("");           break;
            case ESC       : popmenu(basicM);   break;
            case '\b'      : delchar();         break;
            case '\t'      : inschar('\t');     break;
            default        : putch('\a');
        }
        if( !v_move ) GOAL_COL = -1;
    }

    /* Return *dp.  dp = NULL if nothing left. prompt on before last char */
        if( dp==NULL ) { /* ended with !data_start */
            data_start = true;
            no_prompt(false);
            return '\n';
        }
        /* skip prompt_chars */
        while( data_start && index(prompt_chars, *dp) != NULL
                          && dp < end_dp )
            dp++;

        if( dp >= end_dp ) {
            dp = NULL;
            no_prompt(false);
            return '\n';
        }
        c = *dp;
        dp ++;
        if( dp == end_dp ) dp = NULL;
        data_start = (c=='\n');
        if( dp==NULL && data_start ) { /* last char is '\n' */
            no_prompt(false);
        }
        return c;
}

void special(c) char c; {
    switch(c) {
	case Fkey(1):  help_screen();           break;
        case UP_ARROW: go_up(); v_move = true;  break;
        case DN_ARROW: go_dn(); v_move = true;  break;
        case LT_ARROW: go_lt();                 break;
        case RT_ARROW: go_rt();                 break;
        case INS:      breakline(false);        break;
        case DEL:      delunder();              break;
        case Home:     go_home();               break;
        case End:      go_end();                break;
        case PgUp:     {int i; v_move = true;
                        for(i=0; i<10; i++) go_up(); } break;
        case PgDn:     {int i; v_move = true;
                        for(i=0; i<10; i++) go_dn(); } break;
        case CPgUp:    go_TOP();  break;
        case CPgDn:    go_EOB();  break;
        default:       putch('\a');
    }
}

void go_dn(){
    int    old_at_x   = at_x;

    at_x = cur_dn(GOAL_COL);

    if( at_x != -1 ) {
	if( at_y == ROWS ) { /* scroll new line in at bottom */
	    GOTOXY(1,1);
	    delline();
	    shift_up(1,ROWS);
	    GOTOXY(1,ROWS);
	    l_refresh(at_y,CURSOR);
	} else {    /* move down a line */
	    at_y ++;
	}
	GOTOXY(at_x,at_y);
    } else  at_x = old_at_x;
}


void go_up(){
    int old_at_x = at_x;
    at_x = cur_up(GOAL_COL);
    if( at_x != -1 ) {
        if( at_y == 1 ) { /* scroll new line in at top */
            GOTOXY(1,1);
            insline();
            shift_dn(1,ROWS);
            l_refresh(at_y,CURSOR);
            clear_msg();
        } else {    /* move up a line */
            at_y --;
        }
        GOTOXY(at_x,at_y);
    } else at_x = old_at_x;
}


void go_lt(){
    if(at_x>1){
        GOTOXY(at_x = at_x-1, at_y );
        cur_dec();
    } else {
      go_up();
      go_end();
    }
}

void go_rt(){
    if( cur_char == EOS ) return;

    if(at_x<COLS-1 && cur_char != '\n'){
        GOTOXY(at_x = at_x+1, at_y );
        cur_inc();
    } else { go_dn(); go_home(); }
}

void go_home(){GOTOXY(at_x = 1,at_y); cur_home();}
void go_end(){
    at_x += cur_end();
    CURSOR = cur_first(CURSOR) + at_x - 1;
    GOTOXY(at_x,at_y);
}

/* move cursor to end of buffer */
void go_EOB(){
    CURSOR = (TAIL==STOP) ? GAP-1 : STOP-1;
    recenter();
};

/* move cursor to top of buffer */
void go_TOP(){
    CURSOR = START;
    recenter();
};

void Eputc(c) char c; {
    if( cur_char != EOS ) go_EOB();
    if( isprint(c) || isspace(c) ) {
        do_refresh = c=='\n';
        inschar(c);
        do_refresh = true;
    }
}


int wget(x,y) int x,y; {
    int result;
    gettext(WCol(x),WRow(y), WCol(x),WRow(y),  &result);
    return result;
}

#define B_vt   179
#define B_hz   196
#define B_lt   218
#define B_lb   192
#define B_rt   191
#define B_rb   217



void inschar(c) char c; {
    switch( c ) {
    case '\t':
        TOUCH(at_x,at_y);
        do { cur_ins(' '); at_x++; } while( at_x%8 != 1 );
        break;

    case '\n':
        breakline(true);
        break;

    default:
        cur_ins( c );
        TOUCH(at_x,at_y);
        at_x++;
    }
    l_refresh(at_y,CURSOR);
}


/* refresh line y containing cur */
void l_refresh(y,cur) int y; char * cur;{
    char *p;
    Bool reverse = false;
    int i;       /* going to write *p in col i */

    if( !do_refresh || ROW_OK(y) ) return;
    p = cur_first(cur);
    if( p == cur_first(DEF_MARK) ) {
        textattr(FORE(BLACK) + BACK(WHITE));
        reverse = true;
    }

    i = 1;
    while( i<COLS && *p != '\n' && *p != EOS ) {
        if( i >= col_changed[y] ){
            if( i > 1 && reverse ) {
                textattr(FORE(WHITE) + BACK(BLACK));
                reverse = false;
            }
            GOTOXY(i,y);
            putch(*p);
        }
        p++; i++;
    }
    GOTOXY(i,y);
    if(i > 1 && reverse) {
        textattr(FORE(WHITE) + BACK(BLACK));
        reverse = false;
    }
    clreol();
    if(reverse) {
        textattr(FORE(WHITE) + BACK(BLACK));
    }
    if( i==COLS ) putch('\\');
    GOTOXY(at_x,at_y);
    FIX(y);
}

void delunder() {
    if( cur_char==EOS ) { putch('\a'); return; }
    go_rt();
    delchar();
}

void delchar() {
    if( CURSOR == START ) {putch('\a'); return; }
    if( at_x == 1 ) {
        if( at_y == 1 ) { /* new line at top */
            if( START < WSTART ) { /* something to delete */
                GOTOXY(1,1);
                delline();
                shift_up(1,ROWS);
                cur_del();
                recenter();
            }
        } else { /* scroll lines up */
            delline();
            shift_up(at_y,ROWS);
            cur_del();
            TOUCH(1,at_y-1);
            recenter();
        }
    } else {
        at_x--;
        TOUCH(at_x,at_y);
        cur_del();
        l_refresh(at_y,CURSOR);
   }
}


/* if newline, cursor is after the newline */
void breakline(newline) Bool newline; {
    cur_ins('\n');

    /* maintain window size */
        if( START != WSTART || TAIL != STOP || line_count() > B_ROWS ) {
            if( CURSOR == GAP ) win_dn();
            else                gap_up();
        }


    /* show new next line */
        if( at_y==ROWS ) { /* scroll up */
            shift_up(1,ROWS);
            GOTOXY(COLS,ROWS+1);
            putch(' ');
            win_dn();
            gap_dn();
        } else {
            GOTOXY(1,at_y = at_y+1);
            shift_dn(at_y,ROWS);
            insline();
        }

        TOUCH(0,at_y);
        l_refresh(at_y,CURSOR);

    /* show current line */
        at_y--;
        cur_dec();
        TOUCH(0,at_y);
        l_refresh(at_y,CURSOR);

    if( newline ) { cur_inc(); at_x = 1; at_y = at_y+1; }
    CURSOR = cur_first(CURSOR) + at_x - 1;
    GOTOXY(at_x, at_y);
    clear_msg();
}


void go_mark() {
    char * t;
    t        = DEF_MARK;
    DEF_MARK = CURSOR;
    CURSOR   = t;
    show_mark(CURSOR);
    show_mark(DEF_MARK);
    recenter();
}

void recenter() {
    int i;
    i = 0;
    GOTOXY(1,1);
    while( CURSOR < WSTART ) {
        if( i < ROWS ) {
            shift_dn(1,ROWS);
            insline();
            i++;
        }
        win_up(); gap_up();
    }
    while( GAP   <= CURSOR ) {
        if( i < ROWS ) {
            shift_up(1,ROWS);
            delline();
            i++;
        }
        gap_dn(); win_dn();
    }
    i = line_count();
    while( i<ROWS ) {
        if(        TAIL != STOP  ) {gap_dn(); i++; TOUCH(0,i);}
        else if( WSTART != START ) {win_up(); i++; shift_dn(1,ROWS); insline();}
        else break;
    }
    s_refresh();
}

void s_refresh() {
    char *p;
    int i;
    at_x = at_y = 1;    /* follow p up to CURSOR */

    for( i=1, p = WSTART;  p < GAP; p++ ) {
        if( *p == '\n' || *p == EOS ) {
            l_refresh(i,p);
            i++;
        }
        if( p < CURSOR ) {
            at_x++;
            if( *p == '\n' ) { at_x = 1; at_y++; }
        }
    }
    clear_msg();

    CURSOR = cur_first(CURSOR) + at_x - 1;
    GOTOXY(at_x, at_y);
}




/*
 *   Windows
 */
void box(left,top,right,bot)
int left, top, right, bot; {
     int i;

     if( !( left<right-1 && top<bot-1) ) return;

     window(1,1,COLS,RROW);
     textattr(FORE(BLACK) + BACK(WHITE));

     if( left >= 1 ) {
	 for( i=top+1; i<bot ; i++ ) { GOTOXY(left,i); putch(B_vt); }
     }
     if( right <= COLS ) {
	 for( i=top+1; i<bot ; i++ ) { GOTOXY(right,i); putch(B_vt); }
     }
     if( top >= 1 ) {
	 for( i=left+1; i<right ; i++ ) { GOTOXY(i, top); putch(B_hz); }
     }
     if( bot < RROW ) {
	 for( i=left+1; i<right ; i++ ) { GOTOXY(i, bot); putch(B_hz); }
     }
     if( left > 1 && top > 1 ) { GOTOXY(left, top); putch(B_lt); }
     if( left > 1 && bot <RROW ) { GOTOXY(left, bot); putch(B_lb); }
     if( right<COLS && top > 1 ) { GOTOXY(right,top); putch(B_rt); }
     if( right<COLS && bot <RROW ) { GOTOXY(right,bot); putch(B_rb); }
     textattr(FORE(WHITE) + BACK(BLACK));

     window(left+1, top+1, right-1, bot-1);
     clrscr();
     window(1,1,COLS,RROW);
}

void bwindow(left, top, right, bot, title)
int left, top, right, bot;
char * title;
{    int i;
     int delta;
     int o_left, o_right, o_top, o_bot;

    o_left = max(1, left-MARGIN);
    o_top  = max(1, top-MARGIN);
    o_right= min(COLS, right+MARGIN);
    o_bot  = min(RROW, bot+MARGIN);

     window(o_left,o_top,o_right,o_bot);
     clrscr();

#if 0
     /* THIS CODE CAN BE USED TO CREATE EXPLODING BOXES */
     delta = min(o_right-o_left, o_bot-o_top) / STEPS;
     delta = max(1,delta);

     for( i=STEPS; i>=0; i-- ) {
	 box(o_left+delta*i, o_top+delta*i,
	     o_right-delta*i, o_bot-delta*i);
     }
#else
    box(o_left,o_top,o_right,o_bot);
#endif
     if( top > 1 && title!=NULL && *title != EOS ) {
         int len = strlen(title)+2;
	 GOTOXY( (right-left-len)/2 + left+1, o_top);
         putch(' ');
         while( *title != EOS ) {putch(*(title++));}
         putch(' ');
     }
     window(left,top,right,bot);
     clrscr();
}


struct text_info curwin;
char *overlayed = NULL;
int o_left, o_top, o_right, o_bot;


/* Return false if can't popup (no space or already popped) */
Bool popup(cols, rows, title)
int rows,cols; char* title;
{
    int left, right, top, bot;
    left  = (COLS-cols) / 2;
    right = left+cols-1;
    top   = (ROWS-rows) / 2;
    bot   = top+rows-1;
    if( overlayed != NULL ) return false;
    gettextinfo(&curwin);
    o_left = max(1, left-MARGIN);
    o_top  = max(1, top-MARGIN);
    o_right= min(COLS, right+MARGIN);
    o_bot  = min(RROW, bot+MARGIN);
    overlayed = malloc(2* (o_right-o_left+1)*(o_bot-o_top+1));
    if( overlayed == NULL ) return false;
    gettext(o_left,o_top,o_right,o_bot, overlayed);
    bwindow(left,top,right,bot, title);
    return true;
}

void unpopup(){
    if( overlayed==NULL ) return;
    puttext(o_left,o_top,o_right,o_bot, overlayed);
    window(curwin.winleft, curwin.wintop,
           curwin.winright, curwin.winbottom);
    textattr(curwin.attribute);
    GOTOXY(curwin.curx, curwin.cury);
    free(overlayed);
    overlayed = NULL;
}

void popmsg(width, msg,title) int width; char *msg, *title; {
    int rows, cols, i;
    char *p;
    Bool delay = title==NULL;

    rows = 1; cols = width;
    i = 1; p = msg;
    while( *p ) {
        if( *p=='\n' ) {rows++; cols=max(cols,i); i=0;}
        i++;
        p++;
    }
    cols = max(cols,i);
    cols = max(cols, strlen(title)+4);
    rows += 2;
    if( title==NULL ) { title = "Type any key"; }
    if( popup(cols,rows, title) ) {
        putch('\n');
        for( p = msg; *p; p++ ){
            if( *p == '\n' ) putch('\r');
            putch(*p);
        }
        if( delay ) {int c = getch(); if(!c) getch(); unpopup();}
    } else {
        clrscr();
        cprintf("WARNING:  OUT OF MEMORY\n"
                "SAVE WHAT YOU CAN AND EXIT\n\n%s", msg);
    }
}

Bool popquery(msg) char * msg; {
    char result;
    popmsg(0, msg, "Type y or n");
    do{ result = toupper(getch());
    } while( result != 'Y' && result != 'N' && result != ESC );
    unpopup();
    return result=='Y';
}

#define Max_Text 50

char * poptext(width, msg, initial) int width; char * msg, *initial; {
    static char line[Max_Text+1];
    char *p,c;
    int i;

    width = min(width, Max_Text);
    popmsg(width, msg, "");
    strncpy(line,initial,width);
    putch('\r'); putch('\n');

    p = line;
    i = 0;
    while( *p ) { putch(*p++); i++;}
    *p = EOS;

    while( (c=getch()) != '\n' && c!='\r' ) {
        if (isprint(c) && i < width ) { *p++ = c; i++; putch(c); }
        else if (c=='\b' && i>0     ) {
                *--p = EOS; i--;
                putch('\b');putch(' ');putch('\b');
        } else if( c==ESC ) {
            unpopup(); return NULL;
        }else putch('\a');
    }

    unpopup();
    return line;
}

void sys(cmd) char * cmd; {
    char* screen = malloc( 2*COLS*RROW );
    if( screen==NULL ) return;
    gettext(1,1,COLS,RROW,screen);
    window(1,1,COLS,RROW); GOTOXY(1,RROW);
    system(cmd);
    printf("Press any key to continue.");
    if( getch()==0 ) getch();
    puttext(1,1,COLS,RROW,screen);
    window(LEFT, TOP,  LEFT+B_COLS-1, TOP+B_ROWS);
    GOTOXY(at_x,at_y);
    free(screen);
}

FILE* popfile(mode) char * mode; {
    char *p;
    p = poptext(20, "Enter name of file.\n",
                    "");
    if( p==NULL ) return NULL;
    return FOpen(p,mode);
}

void write_region() {
    FILE * f;
    char * old;
    Bool front = true;
    assert( dp==NULL );

    f = popfile("a");
    if( f==NULL ){ NO_FILE_MSG; return; }

    dp     = cur_first(DEF_MARK);
    end_dp = cur_last(CURSOR);
    if( *end_dp == EOS ) end_dp--;

    while( dp <= end_dp ) {
	Bool failure;
	if( front && index(prompt_chars, *dp) != NULL ) {
	    failure = (putc(' ',f) == EOF);
	} else {
	    failure = (putc(*dp, f)==EOF);
            front = (*dp == '\n');
        }
	dp++;

	if( failure ) {
	    popmsg(0, "Output error", NULL );
	    dp = NULL;
	    fclose(f);
	    return;
	}
    }
    dp = NULL;
    fclose(f);
    popmsg(0, "Region written", NULL);
    old = DEF_MARK;
    DEF_MARK = CURSOR;
    show_mark(old);
    show_mark(DEF_MARK);
    recenter();
}


void popmenu(menu) Menu menu; {
    int row, cols, last;
    int c;

    cols = 0;
    for( row = 0; menu[row].text ; row++ ) {
        cols = max(cols, strlen(menu[row].text));
    }
    last = row-1;
    assert( row <= ROWS );
    popup(cols+2,row+1,"Select one");

    /* Fill menu */
    for( row = 0; menu[row].text ; row++ ) {
        cprintf(" %s\n\r", menu[row].text );
    }

    /* Select one */
    row = 0;
    do {
        /* Highlight row */
        textattr(FORE(BLACK)+BACK(WHITE));
        GOTOXY(1,row+1);
        cprintf(" %-*s", cols, menu[row].text );

        c = getch();
        /* Reset row */
        textattr(FORE(WHITE)+BACK(BLACK));
        GOTOXY(1,row+1);
        cprintf(" %-*s", cols, menu[row].text );

        if( isalpha(c) ){
            int i;
            c = toupper(c);
            for( i = 0; menu[i].text ; i++ ){
                if( index(menu[i].text,c) != NULL ) {
                    row = i;
                    c = '\n';
                    break;
                }
            }
        } else switch( c ) {
        case 0: /* Special char */
            c = getch();
            switch( c ) {
                case UP_ARROW: if( row>0    ) row--; break;
                case DN_ARROW: if( row<last ) row++; break;
            }
            break;
        case '\n':
        case '\r':
            c = '\n';
            break;
        case ESC:
            break;
        }
    } while ( c!= ESC && c!='\n' );

    textattr(FORE(WHITE)+BACK(BLACK));
    unpopup();
    if( c=='\n' ) menu[row].code();
}

void clear_msg(){
    GOTOXY(1,ROWS+1);
    clreol();
    GOTOXY(at_x,at_y);
}

void q_exit() {
    if( popquery("Really exit?") )
        do_exit(GOOD_EXIT);
}

void save_buff() {
    FILE *f = popfile("a");
    char *p;
    Bool failure;
    if( f==NULL ){ NO_FILE_MSG; return; }
    p = START;
    while( p < STOP ) {
	if( *p ) failure = (putc(*p,f)==EOF);
	p++;
	if( p==GAP ) p = TAIL;
	if( failure ) { popmsg(0,"Output error",NULL);
	    fclose(f);
	    return;
	}
    }
    fclose(f);
    popmsg(0, "Buffer written", NULL);
}


void read_region() {
    FILE * f = popfile("r");
    char * start;
    int c;
    if( f==NULL ){ NO_FILE_MSG; return; }
    Eputc('\n');
    start = CURSOR;
    while( (c=getc(f)) != EOF ) {
        Eputc(c);
    }
    fclose(f);
    recenter();
    LAST_FRONT = start;
    LAST_BACK  = CURSOR;
}

void copy_region() {
    char * old = DEF_MARK;
    assert( dp == NULL );
    dp     = cur_first(DEF_MARK);
    end_dp = cur_last(CURSOR);

    if( dp >= end_dp ) {
	dp = NULL;
	return;
    }

    go_EOB();
    if( *(CURSOR-1) != '\n' ) { /* insert '\n' at EOB */
        Eputc('\n');
    }
    LAST_FRONT = CURSOR-1;  /* Precede line */

    while( dp < end_dp ) Eputc( *dp++ );

    recenter();
    LAST_FRONT ++;   /* Move back onto first line */
    inschar('\n');
    LAST_BACK = CURSOR;
    DEF_MARK = CURSOR;
    show_mark(old);
    show_mark(DEF_MARK);
    dp = NULL;
}


void get_info() {
    char text[100];
    int buff_size = STOP - START;
    int text_size = (GAP - START) + (STOP - TAIL);
    int pos       = (CURSOR - START);
    sprintf(text, "Buffer    %5d/%5d  (%d%%)\n"
                  "Position  %5d/%5d  (%d%%)",
          text_size, buff_size, (int)(((float)text_size/buff_size)*100),
          pos,       text_size, (int)(((float)pos/text_size)*100) );
    popmsg(0,text,NULL);
}

void touch_all() {
    int i;
    for( i = 1; i<=ROWS; i++ ) TOUCH(0,i);
}

void shift_up(a,b) int a,b; {
    int i;
    for( i=a; i<b; i++ ) col_changed[i] = col_changed[i+1];
    TOUCH(0,b);
}

void shift_dn(a,b) int a,b; {
    int i;
    for( i=b; i>a; i-- ) col_changed[i] = col_changed[i-1];
    TOUCH(0,a);
}

void Eflush() {
    s_refresh();
}

/* Refresh line with mark */
void show_mark(mark) char *mark;{
    int i;
    char * p;

    if( WSTART <= mark && mark < GAP ) {
        /* i = line containing mark */
        i = 1;
        p = WSTART;
        while( p < mark ) {
            if( *p == '\n' ) i++;
            p++;
        }
        TOUCH(0,i);
        l_refresh(i,mark);
    }
}

void set_mark() {
    char * old = DEF_MARK;
    DEF_MARK  = cur_first(CURSOR);
    show_mark(old);
    show_mark(DEF_MARK);
}


#define MAX_NEST 10

/* assumes CURSOR-1 points to a close bracket */
void show_match() {
    char * p = CURSOR;
    char close[MAX_NEST];
    int  next = 0;

    do {
        p--;
        switch(*p){
        case ')': case ']': case '}': { /* push *p */
            if( next==MAX_NEST ) return;
            close[next] = *p;
            next++;
            } break;
        case '(': case '[': case '{': { /* check *p */
            char other = (  *p=='(' ? ')'
                          : *p=='[' ? ']'
                          :           '}' );
            next--;
            if( other != close[next] ) {
                msg("Mis-matched");
                return;
            }
            }break;
        }
    } while( WSTART  < p && next > 0 );
    if( next > 0 ) msg("Match is off screen");
    else {
        char *t;
        t = CURSOR;
        CURSOR = p;
        recenter();
        delay(500);
        CURSOR = t;
        recenter();
    }
}

void msg(s) char *s; {
    GOTOXY(1,ROWS+1);
    cprintf("%-*s", COLS-2, s);
    GOTOXY(at_x,at_y);
}

void help_screen() {
    char *msg = 
        "ESC            Get menu\n"
        "Arrow Keys     Motion\n"
        "INS            Break line\n"
        "DEL            Delete under\n"
        "Backspace      Delete left\n"
        "Home           Left of line\n"
        "End            Right of line\n"
        "PgUp (PgDn)    Up (down) 8 lines\n"
        "CPgUp (CPgDn)  Top (bottom) of buffer\n"
        "Return (LF)    If below tag, execute; o.w. insert return\n"
        "^A             Mark previous region\n"
        "^B             Go back to bookmark (previous tag)\n"
        "^E             Erase current line\n"
        "^L             Refresh screen\n"
        "^T             Tag top of region\n"
        "^X             Exchange tag and cursor\n"
        "^Z             Escape to DOS.  Use ``exit'' to return";

    popmsg(0,msg,NULL);
}



void GOTOXY(x,y) int x,y; {
   gotoxy(min(x,COLS),y);
}

void nl_at_end() {
    go_EOB();
    if( *(CURSOR-1) != '\n' ) Eputc('\n');
}
#endif
#endif
