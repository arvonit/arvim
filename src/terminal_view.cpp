#include "terminal_view.h"
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

using namespace std;

static bool g_raw_mode = false;
static struct termios g_orig_termios;

static void set_raw_mode(bool f) {
    g_raw_mode = f;
}
static bool is_raw_mode() {
    return g_raw_mode;
}

static void at_exit_handler() {
    if (is_raw_mode()) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        set_raw_mode(false);
    }
}

static void enable_raw_mode(int fd) {
    struct termios raw;
    if (is_raw_mode())
        return;
    if (!isatty(STDIN_FILENO))
        throw string("Error in atty standard input");
    atexit(at_exit_handler);
    if (tcgetattr(fd, &g_orig_termios) == -1)
        throw string("Fatal error: tcgetattr");
    raw = g_orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0)
        throw string("Fatal error: tcsetattr");
    set_raw_mode(true);
}

// ---- terminal_view_config ----

terminal_view_config::terminal_view_config() {
    init_params();
    init_wnd_size();
}

void terminal_view_config::init_params() {
    cx = 0;
    cy = 0;
}

void terminal_view_config::init_wnd_size() {
    const int ifd = STDIN_FILENO;
    const int ofd = STDIN_FILENO;
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        int orig_row, orig_col, retval;
        retval = get_cursor_position(ifd, ofd, &orig_row, &orig_col);
        if (retval == -1)
            throw string("Cannot get cursor position");
        if (write(ofd, "\x1b[999C\x1b[999B", 12) != 12)
            throw string("Cannot write to standard output");
        retval = get_cursor_position(ifd, ofd, &screenrows, &screencols);
        if (retval == -1)
            throw string("Cannot get cursor position2");
        char seq[32];
        snprintf(seq, 32, "\x1b[%d;%dH", orig_row, orig_col);
        if (write(ofd, seq, strlen(seq)) == -1)
            throw string("Cannot write to standard output");
    } else {
        screencols = ws.ws_col;
        screenrows = ws.ws_row;
    }
}

int terminal_view_config::get_cursor_position(int ifd, int ofd, int* rows, int* cols) {
    char buf[32];
    unsigned int i = 0;
    if (write(ofd, "\x1b[6n", 4) != 4)
        return -1;
    while (i < sizeof(buf) - 1) {
        if (read(ifd, buf + i, 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != ESC || buf[1] != '[')
        return -1;
    if (sscanf(buf + 2, "%d;%d", rows, cols) != 2)
        return -1;
    return 0;
}

void terminal_view_config::add_status_row(const string& left, const string& right, bool dark_bg) {
    status_left.push_back(left);
    status_right.push_back(right);
    status_dark_bg.push_back(dark_bg);
}

// ---- terminal_view ----

terminal_view::terminal_view() : quitting(false), key_last_pressed(KEY_NULL) {
    init();
}

terminal_view::~terminal_view() {
}

void terminal_view::init() {
    enable_raw_mode(STDIN_FILENO);
    write(STDOUT_FILENO, "\x1b[?1049h", 8); // enter alternate screen (vim uses this to hide content from scrollback)
}

void terminal_view::show() {
    while (!should_quit()) {
        refresh();
        key_last_pressed = read_key(STDIN_FILENO);
        if (key_last_pressed == CTRL_Q) {
            quit();
            break;
        }
        notify();
    }
}

void terminal_view::refresh() {
    clear_buffer();
    for (unsigned int i = 0; i < rows.size(); ++i) {
        append_row_buffer(i, rows[i]);
    }
    finish_rows_buffer();
    write(STDOUT_FILENO, buffer.c_str(), buffer.length());
}

void terminal_view::add_row(const string& row) {
    rows.push_back(row);
}

void terminal_view::quit() {
    quitting = true;
    clear_buffer();
    write(STDOUT_FILENO, "\x1b[?1049l", 8); // exit alternate screen, restoring scrollback
}

void terminal_view::clear_buffer() {
    buffer.clear();
    buffer += "\x1b[?25l";
    buffer += "\x1b[H";
}

void terminal_view::append_row_buffer(int row, const string& str_row) {
    if (color_info.find(row) == color_info.end()) {
        buffer += str_row;
    } else {
        int clr_curr = -1;
        int clr_range_right = -1;
        for (int i = 0; i < (int)str_row.size(); ++i) {
            auto it = color_info[row].find(i);
            int clr_new = clr_curr;
            if (it != color_info[row].end()) {
                clr_new = it->second.second;
                clr_range_right = it->second.first;
            } else if (i > clr_range_right && clr_range_right >= 0) {
                clr_new = -1;
                clr_range_right = -1;
            }
            if (clr_new != clr_curr) {
                char buf[16];
                if (clr_new != -1) {
                    snprintf(buf, sizeof(buf), "\x1b[%dm", clr_new);
                    buffer += buf;
                } else {
                    buffer += "\x1b[39m";
                }
                clr_curr = clr_new;
            }
            buffer += str_row[i];
        }
    }
    buffer += "\x1b[39m";
    buffer += "\x1b[0K";
    buffer += "\r\n";
}

void terminal_view::finish_rows_buffer() {
    for (int r = num_rows(); r < config.row_count(); ++r) {
        buffer += "~\x1b[0K\r\n";
    }
    for (int i = 0; i < config.num_status_rows(); ++i) {
        append_status_msg(i);
    }
    // Position cursor
    char buf[32];
    int cx = 1;
    int filerow = config.get_cursor_y();
    const char* row = (filerow >= num_rows()) ? nullptr : get_row(filerow);
    if (row) {
        for (int j = 0; j < config.get_cursor_x(); j++) {
            if (j < (int)strlen(row) && row[j] == TAB)
                cx += 7 - (cx % 8);
            cx++;
        }
    }
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", config.get_cursor_y() + 1, cx);
    buffer += buf;
    buffer += "\x1b[?25h";
}

void terminal_view::append_status_msg(int rs) {
    buffer += "\x1b[0K";
    if (config.status_row_dark_bg(rs))
        buffer += "\x1b[7m";
    string left = config.status_row_left(rs);
    string right = config.status_row_right(rs);
    int len = (int)left.size();
    int rlen = (int)right.size();
    if (len > config.col_count()) {
        left = left.substr(0, config.col_count());
        len = config.col_count();
    }
    buffer += left;
    while (len < config.col_count()) {
        if (config.col_count() - len == rlen) {
            buffer += right;
            break;
        } else {
            buffer += " ";
            len++;
        }
    }
    if (config.status_row_dark_bg(rs))
        buffer += "\x1b[0m";
    if (rs != config.num_status_rows() - 1)
        buffer += "\r\n";
}

void terminal_view::set_color(int row, int col_begin, int col_end, TEXT_COLOR color) {
    if (color != TEXT_COLOR_DEF) {
        color_info[row][col_begin] = {col_end, color};
    } else {
        if (color_info.count(row) && color_info[row].count(col_begin))
            color_info[row].erase(col_begin);
    }
}

int terminal_view::read_key(int fd) {
    int nread;
    char c, seq[3];
    while ((nread = read(fd, &c, 1)) == 0)
        ;
    if (nread == -1)
        exit(1);

    while (1) {
        switch (c) {
        case ESC:
            if (read(fd, seq, 1) == 0)
                return ESC;
            if (read(fd, seq + 1, 1) == 0)
                return ESC;
            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    if (read(fd, seq + 2, 1) == 0)
                        return ESC;
                    if (seq[2] == '~') {
                        switch (seq[1]) {
                        case '3':
                            return DEL_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                        }
                    }
                } else {
                    switch (seq[1]) {
                    case 'A':
                        return ARROW_UP;
                    case 'B':
                        return ARROW_DOWN;
                    case 'C':
                        return ARROW_RIGHT;
                    case 'D':
                        return ARROW_LEFT;
                    case 'H':
                        return HOME_KEY;
                    case 'F':
                        return END_KEY;
                    }
                }
            } else if (seq[0] == 'O') {
                switch (seq[1]) {
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
                }
            }
            break;
        default:
            return c;
        }
    }
}
