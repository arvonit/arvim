#ifndef TERMINAL_VIEW_H
#define TERMINAL_VIEW_H

#include "observer.h"
#include "view_interface.h"
#include <map>
#include <string>
#include <vector>

enum KEY_ACTION {
    KEY_NULL = 0,
    CTRL_A = 1,
    CTRL_B = 2,
    CTRL_C = 3,
    CTRL_D = 4,
    CTRL_E = 5,
    CTRL_F = 6,
    CTRL_G = 7,
    CTRL_H = 8,
    TAB = 9,
    CTRL_L = 12,
    ENTER = 13,
    CTRL_Q = 17,
    CTRL_R = 18,
    CTRL_S = 19,
    CTRL_U = 21,
    CTRL_V = 22,
    CTRL_Y = 25,
    CTRL_Z = 26,
    ESC = 27,
    SLASH = 47,
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

// Internal configuration — tracks cursor position and status bar state.
class terminal_view_config {
public:
    terminal_view_config();

    int row_count() const {
        return screenrows - (int)status_left.size();
    }
    int col_count() const {
        return screencols;
    }
    int get_cursor_x() const {
        return cx;
    }
    int get_cursor_y() const {
        return cy;
    }
    void set_cursor_x(int pos) {
        cx = pos;
    }
    void set_cursor_y(int pos) {
        cy = pos;
    }

    int num_status_rows() const {
        return status_left.size();
    }
    std::string status_row_left(int r) const {
        return status_left[r];
    }
    std::string status_row_right(int r) const {
        return status_right[r];
    }
    bool status_row_dark_bg(int r) const {
        return status_dark_bg[r];
    }

    void clear_status_rows() {
        status_left.clear();
        status_right.clear();
        status_dark_bg.clear();
    }
    void add_status_row(const std::string& left, const std::string& right, bool dark_bg);

private:
    void init_params();
    void init_wnd_size();
    int get_cursor_position(int ifd, int ofd, int* rows, int* cols);

    int cx, cy;
    int screenrows, screencols;
    std::vector<std::string> status_left;
    std::vector<std::string> status_right;
    std::vector<bool> status_dark_bg;
};

// Concrete terminal view — renders to the terminal using ANSI escape codes.
// Also acts as the ec_observer_subject that drives the event loop.
class terminal_view : public view_interface, public ec_observer_subject {
public:
    terminal_view();
    virtual ~terminal_view();

    // Enter the main event loop (blocks until quit).
    void show();

    // Redraw the entire screen from the current row/color buffers.
    void refresh();

    // Returns the last key pressed (call from within update()).
    int get_pressed_key() const {
        return key_last_pressed;
    }

    // Signal quit and clear screen.
    void quit();

    // --- view_interface implementation ---
    void init_rows() override {
        rows.clear();
    }
    void add_row(const std::string& row) override;
    int get_row_count() const override {
        return config.row_count();
    }
    int get_col_count() const override {
        return config.col_count();
    }
    int get_cursor_x() const override {
        return config.get_cursor_x();
    }
    int get_cursor_y() const override {
        return config.get_cursor_y();
    }
    void set_cursor_x(int x) override {
        config.set_cursor_x(x);
    }
    void set_cursor_y(int y) override {
        config.set_cursor_y(y);
    }
    void clear_status_rows() override {
        config.clear_status_rows();
    }
    void add_status_row(const std::string& left, const std::string& right, bool dark_bg) override {
        config.add_status_row(left, right, dark_bg);
    }
    void set_color(int row, int col_begin, int col_end, TEXT_COLOR color) override;
    void clear_color() override {
        color_info.clear();
    }
    void finalize_rows() override {
    }

private:
    void init();
    void clear_buffer();
    void append_row_buffer(int row, const std::string& str_row);
    void finish_rows_buffer();
    void append_status_msg(int rs);
    bool should_quit() const {
        return quitting;
    }
    int read_key(int fd);
    int num_rows() const {
        return rows.size();
    }
    const char* get_row(int r) const {
        return rows[r].c_str();
    }

    std::string buffer;
    std::vector<std::string> rows;
    terminal_view_config config;
    std::map<int, std::map<int, std::pair<int, TEXT_COLOR>>> color_info;
    bool quitting;
    int key_last_pressed;
};

#endif
