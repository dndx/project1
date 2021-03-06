#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <ncurses.h>
#include <sys/param.h>
#include "utils.h"
#include "sound.h"
#include "cs229.h"
#include "aiff.h"

/* Middle line of sample shown area */
#define MIDDLE_LINE (maxx - 32) / 2 + 10

#define SPLASH_IMAGE \
"                                                                  ___     \n" \
"                             ,---,                ,---,  ,--,   ,--.'|_   \n" \
"                 ,---,     ,---.'|              ,---.'|,--.'|   |  | :,'  \n" \
"  .--.--.    ,-+-. /  |    |   | :              |   | :|  |,    :  : ' :  \n" \
" /  /    '  ,--.'|'   |    |   | |   ,---.      |   | |`--'_  .;__,'  /   \n" \
"|  :  /`./ |   |  ,\"' |  ,--.__| |  /     \\   ,--.__| |,' ,'| |  |   |   \n" \
"|  :  ;_   |   | /  | | /   ,'   | /    /  | /   ,'   |'  | | :__,'| :    \n" \
" \\  \\    `.|   | |  | |.   '  /  |.    ' / |.   '  /  ||  | :   '  : |__  \n" \
"  `----.   \\   | |  |/ '   ; |:  |'   ;   /|'   ; |:  |'  : |__ |  | '.'| \n" \
" /  /`--'  /   | |--'  |   | '/  ''   |  / ||   | '/  '|  | '.'|;  :    ; \n" \
"'--'.     /|   |/      |   :    :||   :    ||   :    :|;  :    ;|  ,   /  \n" \
"  `--'---' '---'        \\   \\  /   \\   \\  /  \\   \\  /  |  ,   /  ---`-'   \n" \
"                         `----'     `----'    `----'    ---`-'            \n"

/* Some global variables to keep track of state 
 * I knew this is bad, but since there is no multithreading involved, this saves me lots of function paramaters. 
 *
 * top_sample is the absolute index of sample currently on top of window
 * current_sample is the absolute index of sample cursor is currently on
 * bottom_sample is the absolute index of sample currently on bottom of window
 * sample_buffer is the buffer for copy, NULL for nothing in there
 * sample_buffer_len is the length of sample_buffer
 * changes is a flag denotes file have been changed and enable save
 */
static int top_sample, current_sample, bottom_sample, mark_start = -1, marking;
static int *sample_buffer = NULL;
static int sample_buffer_len = 0;
static int changed = 0;

/* Sample loading request */
struct loadreq {
    int count; /* Counter, callback needs increment this */
    int *dst; /* Destination where loaded samples goes */
    int maxx; /* Max width of window, used to determine whether to show splash or not */
};

/**
 * Callback for loading one sample into the buffer
 */
void load_samples(int *samples, const struct soundfile *info, void *data)
{
    struct loadreq *loadreq = (struct loadreq *) data;

    /* Copy all channels in samples array to buffer */
    memcpy(loadreq->dst + (loadreq->count * info->channels), samples, info->channels * sizeof(int));
    loadreq->count++;

    int keypoint = (int) (info->sample_num * 0.005); /* Update loading progress every 0.5% for performance reason */
    if (!keypoint)
        keypoint = 1;

    if (!(loadreq->count % keypoint) || loadreq->count == info->sample_num) /* Update progress every 0.5% or when finished */
    {
        int i;
        for (i=2; i<17; i++)
        {
            move(i, 0);
            clrtoeol();
        }

        if (loadreq->maxx > 73)
        {
            move(2, 0);
            printw(SPLASH_IMAGE);
        }

        move(16, 0);

        printw("Loading: %u/%u (%.2f%%)", loadreq->count, info->sample_num,
                                        (float) loadreq->count / info->sample_num * 100);
        refresh();
    }
}

/**
 * Draw sample display area (left panel)
 *
 * maxx max width of screen
 * maxy max height of screen
 * samples buffer contains all loaded samples, NULL means no sample available
 * start: draw from this sample, starts from 0, -1 means no sample available
 */
void draw_left_panel(int maxx, int maxy, const struct soundfile *info, int *samples, int start)
{
    int i;
    int plot_width = (maxx - 32) / 2;

    top_sample = start;

    for (i=0; 2+i<maxy; i++) /* Went from top of screen to bottom */
    {
        move(2 + i, 0);

        if (!samples) /* No sample available */
        {
            int j;
            for (j=0; j<maxx-21; j++) /* clean area */
                addch(' ');
            addch('|');

            if (i + 2 == maxy - 1) /* Clean completed */
            {
                mvprintw(2, 0, "No Samples");
                refresh();
                top_sample = -1;
                bottom_sample = -1;
                return;
            }
        }

        if (i + start >= (info->sample_num * info->channels)) /* Clean existing area, with samples */
        {
            int j;
            for (j=0; j<maxx-21; j++) /* Change to 21 if want right column delimeter */
                addch(' ');
            addch('|');
            continue;
        }

        int this_sample = (start + i) / info->channels; /* Current sample number */

        /* If we are marking and this sample is in the range of reverse video */
        if (marking && ((this_sample <= mark_start && this_sample >= current_sample / info->channels) ||
            (this_sample >= mark_start && this_sample <= current_sample / info->channels)))
            attron(A_REVERSE);

        if (!((start + i) % info->channels)) /* First channel of a sample */
            printw("%9d", this_sample);
        else
            printw("         ");
 
        /* Percentage of bars */
        double percentage = (double)  samples[start + i] /
                            (pow(2, info->bit_depth - 1) - 1);

        /* Actual number of bars to draw */
        int rounded =  (int) round(percentage * plot_width);

        int k;

        printw("|");
        
        if (rounded < 0) /* Negative sample */
        {
            for (k=0; k<plot_width+rounded; k++)
                printw(" ");

            for (k=0; k<-rounded; k++)
                printw("-");

            printw("|");
            
            for (k=0; k<plot_width; k++)
                printw(" ");
        }
        else if (rounded > 0) /* Positive sample */
        {
     
            for (k=0; k<plot_width; k++)
                printw(" ");
            
            printw("|");
            
            for (k=0; k<rounded; k++)
                printw("-");

            for (k=0; k<plot_width-rounded; k++)
                printw(" ");
        }
        else /* Zero */
        {
            for (k=0; k<plot_width; k++)
                printw(" ");

            printw("|");

            for (k=0; k<plot_width; k++)
                printw(" ");
        }

        printw("|");
        attroff(A_REVERSE);
    }

    bottom_sample = MIN(start + i, info->sample_num * info->channels) - 1; /* Calculate bottom sample number */

    refresh();
}

/**
 * Draw the right panel
 *
 * maxx max width of screen
 * maxy max height of screen
 */
void draw_right_panel(int maxx, int maxy, const struct soundfile *info)
{
    int right_delimiter = maxx - 21; /* x coordinate right panel starts */

    int i;

    for (i=2; i<maxy; i++) /* Wipe existing content */
    {
        move(i, right_delimiter + 1);
        clrtoeol();
    }

    double duration = (double) info->sample_num / info->sample_rate;
    mvprintw(2, right_delimiter + 2, "Sample Rate: %u", info->sample_rate);
    mvprintw(3, right_delimiter + 4, "Bit Depth: %u", info->bit_depth);
    mvprintw(4, right_delimiter + 5, "Channels: %u", info->channels);
    mvprintw(5, right_delimiter + 2, "Samples: %u", info->sample_num);
    mvprintw(6, right_delimiter + 3, "Length: %.0f:%02.0f:%05.2f", floor(duration / 3600), floor(fmod(duration, 3600) / 60), fmod(fmod(duration, 3600), 60));

    move(7, right_delimiter + 1);
    for (i=0; i<20; i++)
        addch('=');

    /* Middle part start, magic numbers everywhere, baaaaaad :B */
    move(9, right_delimiter + 1);
    if (!marking)
        if (info->sample_num)
           mvprintw(9, right_delimiter + 4, "m: mark");
        else
        {
            move(9, right_delimiter + 1);
            clrtoeol();
        }

    else
    {
        mvprintw(9, right_delimiter + 4, "m: unmark");
        mvprintw(10, right_delimiter + 4, "c: copy");
        mvprintw(11, right_delimiter + 4, "x: cut");
    }

    if (sample_buffer) /* We have something in copy buffer */
    {
        mvprintw(12, right_delimiter + 4, "^: insert before");
        mvprintw(13, right_delimiter + 4, "v: insert after");
    }

    if (changed)
        mvprintw(14, right_delimiter + 4, "s: save");

    mvprintw(15, right_delimiter + 4, "q: quit");

    /* Movement start */
    mvprintw(17, right_delimiter + 2, "Movement:");
    mvprintw(18, right_delimiter + 4, "up/dn");
    mvprintw(19, right_delimiter + 4, "pgup/pgdn");
    mvprintw(20, right_delimiter + 4, "g: goto sample");

    /* Bottom part start */
    move(maxy - 3, right_delimiter + 1);
    for (i=0; i<20; i++)
        addch('=');

    if (marking)
        mvprintw(maxy - 2, right_delimiter + 4, "Marked: %d", mark_start);
    if (sample_buffer_len)
        mvprintw(maxy - 1, right_delimiter + 2, "Buffered: %d", sample_buffer_len);

    refresh();
}

/**
 * This function reads string from ncurses window into buffer up to len-1, works only when keypad is on
 */
void readline(char *buffer, int len)
{
    int curlen = 0; /* Current string length */
    int x, y;

    getyx(stdscr, y, x);
    while (1)
    {
        int c = getch();
        
        if (c == KEY_ENTER || c == '\n') /* User hit enter */
        {
            buffer[curlen] = '\0';
            break;
        }
        else if (isprint(c)) /* Printable letter, echo it */
        {
            if (curlen == len - 1)
                continue;
            buffer[curlen++] = c;
            addch(c);
        }
        else if (c == KEY_BACKSPACE) /* Backspace, delete character if there are */
        {
            if (!curlen)
                continue;
            mvaddch(y, x + curlen - 1, ' ');
            move(y, x + curlen - 1);
            curlen--;
        }
    }
}

/**
 * Prompts user to input something
 *
 * maxy max width of window
 * message the message to display
 * dest user input goes here, NULL means don't need user to input anything (use as message box)
 * len read up to len-1 character
 */
void prompt(int maxy, char *message, char *dest, int len)
{
    move(maxy - 1, 0);
    clrtoeol();
    printw(message);
    if (!dest)
    {
        getch();
        return;
    }
    else
        readline(dest, len);
}

/**
 * Insert samples after specified sample
 *
 * sample_num The index of sample to append after, this could be -1 means insert at the beginning
 * samples The buffer contains all samples
 */
int *insert_after(int sample_num, int *samples, struct soundfile *info)
{
    int *new;
    new = realloc(samples, (info->sample_num + sample_buffer_len)
                      * info->channels * sizeof(int)); /* Reallocate memory according to copy buffer size */
    if (!new) /* Failed, terminate */
    {
        free(samples);
        endwin();
        FATAL("calloc() failed");
    }

    /**
     * Shifts every sample after sample_num by the copy buffer size
     */
    memmove(new + (sample_num + 1 + sample_buffer_len) * info->channels,
            new + (sample_num + 1) * info->channels,
            (info->sample_num - sample_num - 1) * info->channels * sizeof(int));
    /**
     * Copy the copy buffer to the spaces we got by shifting
     */
    memcpy(new + (sample_num + 1) * info->channels,
           sample_buffer, sample_buffer_len * info->channels * sizeof(int));

    info->sample_num += sample_buffer_len; /* Update sample_num accordingly */

    return new; /* Return the pointer to all sample in case it changed by realloc */
}

/**
 * Delete samples and resize the sample array
 *
 * sample_num delete from this sample, inclusive
 * samples the sample array
 */
int *delete_from(int sample_num, int *samples, struct soundfile *info)
{
    if (sample_buffer_len == info->sample_num) /* If we are deleting all samples */
    {
        free(samples);
        info->sample_num = 0;
        return NULL;
    }

    if (sample_num < info->sample_num - 1) /* If we are NOT deleting last sample in array, which needs shifting */
        memmove(samples + sample_num * info->channels,
                samples + (sample_num + sample_buffer_len) * info->channels,
                (info->sample_num - sample_num - sample_buffer_len) *
                info->channels * sizeof(int));

    int *new;
    new = realloc(samples, (info->sample_num - sample_buffer_len)
                      * info->channels * sizeof(int));
    if (!new)
    {
        free(samples);
        endwin();
        FATAL("calloc() failed");
    }

    info->sample_num -= sample_buffer_len; /* update file information*/

    return new;
}

int main(int argc, char *argv[])
{
    #ifndef NDEBUG
    getchar(); /* pause the program so that gdb would have chance to attach it */
    #endif
    FILE *file;
    struct soundfile fileinfo;
    char buffer[1024];
    
    if (argc < 2)
        FATAL("You must specify a sound file to edit.");

    file = fopen(argv[1], "r");
    if (!file)
        FATAL("Could not open file %s", argv[1]);

    if (is_cs229_file(file))
        fileinfo = cs229_fileinfo(file);
    else if (is_aiff_file(file))
        fileinfo = aiff_fileinfo(file);
    else
        FATAL("Unrecognized file %s", argv[1]);

    int *samples = calloc(fileinfo.sample_num * fileinfo.channels, sizeof(int));
    if (!samples)
        FATAL("Could not allocate memory.");
 
    int maxx, maxy;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    getmaxyx(stdscr, maxy, maxx);
 
    if (maxx < 40 || maxy < 24)
    {
        endwin();
        FATAL("Width and Height of window must be geater than 40 and 24");
    }

    if (maxx % 2) /* Width is odd */
        maxx--;

    strcpy(buffer, argv[1]);
    strcat(buffer, fileinfo.format == AIFF ? " (aiff)" : " (cs229)"); /* This is the window "title" */
    mvprintw(0, (maxx - strlen(buffer)) / 2, buffer);

    move(1, 0);

    int i;
    for (i=0; i<maxx; i++)
    {
        addch('=');
    }

    struct loadreq req;
    req.count = 0;
    req.dst = samples;
    req.maxx = maxx;

    /* Load all samples into samples buffer */
    if (fileinfo.format == AIFF)
        aiff_enumerate(file, &fileinfo, load_samples, &req);
    else
        cs229_enumerate(file, &fileinfo, load_samples, &req);

    fclose(file);

    if (!fileinfo.sample_num) /* Zero sample file */
    {
        free(samples); /* Linux will return a valid pointer for malloc(0), which is weird. And we still need to free it */
        samples = NULL;
        top_sample = current_sample = bottom_sample = -1; /* This is the state when no smples are in samples buffer */
    }

    if (samples) /* If there are at least 1 sample */
        draw_left_panel(maxx, maxy, &fileinfo, samples, 0);
    else
        draw_left_panel(maxx, maxy, &fileinfo, samples, -1);

    draw_right_panel(maxx, maxy, &fileinfo);

    if (samples) /* If there are sample */
        move(2, MIDDLE_LINE);
    else
        move(2, 0);

    for (;;)
    {
        int input = getch();
        int curx, cury;
        getyx(stdscr, cury, curx);

        switch (input)
        {
            case KEY_DOWN: /* Down arrow */
                if (current_sample < fileinfo.sample_num * fileinfo.channels - 1) /* If we are not on the last channels of last sample */
                    current_sample++;
                else
                    break;
                draw_right_panel(maxx, maxy, &fileinfo);
                if (cury == (maxy - 1)) /* We are on the last sample of this screen, scroll it down */
                {
                    draw_left_panel(maxx, maxy, &fileinfo, samples, ++top_sample);
                    move(cury, MIDDLE_LINE);
                }
                else
                {
                    if (marking) /* If we are marking, we needs to redraw the panel for reversed video */
                        draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                    move(cury + 1, MIDDLE_LINE);
                }
                break;
            case KEY_UP: /* Up arrow */
                if (current_sample > 0) /* If we are not at the first index */
                    current_sample--;
                else
                    break;
                draw_right_panel(maxx, maxy, &fileinfo);
                if (cury == 2) /* We are at the first index of this screen */
                {
                    draw_left_panel(maxx, maxy, &fileinfo, samples, --top_sample);
                    move(cury, MIDDLE_LINE);
                }
                else
                {
                    if (marking) /* Update reversed video if we are marking */
                        draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                    move(cury - 1, MIDDLE_LINE);
                }
                break;
            case KEY_NPAGE: /* pgdn */
                if (bottom_sample >= fileinfo.sample_num * fileinfo.channels - 1) /* last page */
                    break;
                /* Check if we run over bound */
                current_sample = MIN(current_sample + maxy - 2, fileinfo.sample_num * fileinfo.channels - 1);
                draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample += (maxy - 2));
                draw_right_panel(maxx, maxy, &fileinfo);
                move(MIN(cury, bottom_sample - top_sample + 2), MIDDLE_LINE);
                break;
            case KEY_PPAGE: /* pgup */
                if (top_sample <= 0) /* first page */
                    break;
                if (maxy - 2 > top_sample)
                    current_sample -= top_sample; /* If we can still scrool up */
                else
                    current_sample -= (maxy - 2);
                top_sample = MAX(top_sample - (maxy - 2), 0); /* Don't run over bound */
                draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                draw_right_panel(maxx, maxy, &fileinfo);
                move(cury, MIDDLE_LINE);
                break;
            case 'g': /* Goto */
                prompt(maxy, "Please enter a sample number: ", buffer, 10); /* Prompts user for a sample number, up to 9 digits */
                if (!buffer[0]) /* If user typed nothing */
                {
                    draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                    draw_right_panel(maxx, maxy, &fileinfo);
                    if (samples)
                        move(current_sample - top_sample + 2, MIDDLE_LINE);
                    else
                        move(2, 0);
                    break;
                }
                int num = atoi(buffer);
                if (num > (int) fileinfo.sample_num - 1 || num < 0) /* num is out bound */
                {
                    prompt(maxy, "Invalid sample number", NULL, 0);
                    draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                    draw_right_panel(maxx, maxy, &fileinfo);
                    if (samples)
                        move(current_sample - top_sample + 2, MIDDLE_LINE);
                    else
                    {
                        move(2, 0);
                    }
                    break;
                }
                /* Requested sample doesn't need scrool */
                if (num * fileinfo.channels >= top_sample && num * fileinfo.channels <= bottom_sample)
                {
                    current_sample = num * fileinfo.channels;
                    draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                    draw_right_panel(maxx, maxy, &fileinfo);
                    move(num * fileinfo.channels - top_sample + 2, MIDDLE_LINE);
                }
                else if (num * fileinfo.channels > bottom_sample) /* needs scrool down */
                {
                    bottom_sample = num * fileinfo.channels;
                    top_sample = bottom_sample - (maxy - 3);
                    current_sample = num * fileinfo.channels;
                    draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                    draw_right_panel(maxx, maxy, &fileinfo);
                    move(maxy - 1, MIDDLE_LINE);
                }
                else /* needs scrool up */
                {
                    top_sample = num * fileinfo.channels;
                    bottom_sample = top_sample + (maxy - 3);
                    current_sample = num * fileinfo.channels;
                    draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                    draw_right_panel(maxx, maxy, &fileinfo);
                    move(2, MIDDLE_LINE);
                }
                break;
            case 'm':
                if (!samples) /* No sample, why the hell do you want mark? */
                    break;
                if (!marking) /* Not marking, save current location to mark_start */
                {
                    mark_start = current_sample / fileinfo.channels;
                    marking = 1;
                }
                else /* Mark end */
                {
                    marking = 0;
                }
                draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                draw_right_panel(maxx, maxy, &fileinfo);
                move(current_sample - top_sample + 2, MIDDLE_LINE);
                break;
            case 'c': /* Copy */
            case 'x': /* Cut */
                if (!marking) /* Not marking, no way! */
                    break;
                if (sample_buffer) /* If there was something in copy buffer, free it first */
                {
                    free(sample_buffer);
                    sample_buffer = NULL;
                }

                int mark_end;

                /* These code make sure mark_end is always greater or equal to mark_start */
                if (current_sample / fileinfo.channels < mark_start)
                {
                    mark_end = mark_start;
                    mark_start = current_sample / fileinfo.channels;
                }
                else
                {
                    mark_end = current_sample / fileinfo.channels;
                }

                sample_buffer = calloc((mark_end - mark_start + 1) 
                                       * fileinfo.channels, sizeof(int)); /* allocate memory for copy buffer */
                if (!sample_buffer)
                {
                    free(samples);
                    endwin();
                    FATAL("calloc() failed");
                }

                memcpy(sample_buffer, samples + mark_start * fileinfo.channels, 
                       (mark_end - mark_start + 1) * fileinfo.channels * sizeof(int)); /* Copy samples into buffer  */
                sample_buffer_len = mark_end - mark_start + 1;
                marking = 0;

                if (input == 'x') /* Cutting, needs remove samples */
                {
                    samples = delete_from(mark_start, samples, &fileinfo); /* Remove selected samples from samples buffer */

                     /* If we are deleting from end of array, needs scrool back a little bit */
                    if (current_sample >= fileinfo.sample_num * fileinfo.channels && samples)
                    {
                        current_sample = (fileinfo.sample_num - 1) * fileinfo.channels;
                        
                        if ((fileinfo.sample_num - 1) * fileinfo.channels < top_sample)
                        {
                            top_sample = current_sample;
                            bottom_sample = fileinfo.sample_num * fileinfo.channels - 1;
                        }
                    }
                    else if (!samples)
                        current_sample = top_sample = bottom_sample = -1;
                    changed = 1; /* enable saving */
                }

                draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                draw_right_panel(maxx, maxy, &fileinfo);
                if (samples)
                    move(current_sample - top_sample + 2, MIDDLE_LINE);
                else /* oops, we cutted all samples */
                    move(2, 0);
                break;
            case 'v': /* insert after */
                if (!sample_buffer)
                    break;
                if (samples) /* If there are at least sample, we insert after current sample */
                    samples = insert_after(current_sample / fileinfo.channels,
                                           samples, &fileinfo);
                else /* No samples, insert at the beginning */
                {
                    samples = insert_after(-1, samples, &fileinfo);
                    current_sample = 0;
                    top_sample = 0;
                }
                changed = 1;
                draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                draw_right_panel(maxx, maxy, &fileinfo);
                move(current_sample - top_sample + 2, MIDDLE_LINE);
                break;
            case '^': /* Insert before */
                if (!sample_buffer)
                    break;
                if (samples) /* These are same as above */
                {
                    samples = insert_after(current_sample / fileinfo.channels - 1,
                                           samples, &fileinfo);
                    current_sample += sample_buffer_len * fileinfo.channels;
                }
                else
                {
                    samples = insert_after(-1, samples, &fileinfo);
                    current_sample = (fileinfo.sample_num - 1) * fileinfo.channels;
                    top_sample = current_sample;
                }

                /* Needs scrool down */
                if (current_sample - top_sample + 2 >= maxy)
                    top_sample += sample_buffer_len * fileinfo.channels;
                changed = 1;
                draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                draw_right_panel(maxx, maxy, &fileinfo);
                move(current_sample - top_sample + 2, MIDDLE_LINE);
                break;
            case 's': /* save */
                if (!changed)
                    break;
                file = fopen(argv[1], "w");
                if (!file)
                    FATAL("fopen( failed)");

                if (fileinfo.format == AIFF)
                {
                    write_aiff_header(file, &fileinfo);
                }
                else
                {
                    write_cs229_header(file, &fileinfo);
                }

                for (i=0; i<fileinfo.sample_num; i++) /* Went through all samples and call the write callback */
                {
                    int *sample_data = samples + i * fileinfo.channels;
                    if (fileinfo.format == AIFF)
                        write_to_aiff(sample_data, &fileinfo, file);
                    else
                        write_to_cs229(sample_data, &fileinfo, file);
                }

                /* AIFF padding */
                if (fileinfo.format == AIFF && fileinfo.bit_depth == 8 && (fileinfo.channels * fileinfo.sample_num) % 2)
                    fputc('\0', file);

                fclose(file);
                prompt(maxy, "File saved.", NULL, 0);
                draw_left_panel(maxx, maxy, &fileinfo, samples, top_sample);
                draw_right_panel(maxx, maxy, &fileinfo);
                if (samples)
                    move(current_sample - top_sample + 2, MIDDLE_LINE);
                else
                    move(2, 0);
                break;
            case 'q': /* quit */
                endwin();
                if (sample_buffer)
                    free(sample_buffer);
                if (samples)
                    free(samples);
                return EXIT_SUCCESS;
        }
        /* These shows current program state for debugging */
        #ifndef NDEBUG
        getyx(stdscr, cury, curx);
        mvprintw(0, 0, "C: %5d T: %5d B: %5d MS: %5d", current_sample, top_sample, bottom_sample, mark_start);
        move(cury, curx);
        #endif
        curx++; /* Because we are not using curx, but getyx needs two variables, use this to shut gcc up (unused variable) */
    }
}

