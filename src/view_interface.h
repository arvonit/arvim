#ifndef VIEW_INTERFACE_H
#define VIEW_INTERFACE_H

#include <string>

enum TEXT_COLOR {
    TEXT_COLOR_BLACK = 30,
    TEXT_COLOR_WHITE = 37,
    TEXT_COLOR_CYAN = 36,
    TEXT_COLOR_YELLOW = 33,
    TEXT_COLOR_GREEN = 32,
    TEXT_COLOR_MAGENTA = 35,
    TEXT_COLOR_RED = 31,
    TEXT_COLOR_BLUE = 34,
    TEXT_COLOR_DEF = -1
};

// Abstract interface for all view operations used by controller and commands.
// terminal_view is the concrete implementation; decorators wrap this interface.
class view_interface {
public:
    virtual ~view_interface() {
    }

    virtual void init_rows() = 0;
    virtual void add_row(const std::string& row) = 0;

    // Available content area after subtracting status rows and decorator space.
    virtual int get_row_count() const = 0;
    virtual int get_col_count() const = 0;

    virtual int get_cursor_x() const = 0;
    virtual int get_cursor_y() const = 0;
    virtual void set_cursor_x(int x) = 0;
    virtual void set_cursor_y(int y) = 0;

    virtual void clear_status_rows() = 0;
    virtual void add_status_row(const std::string& left, const std::string& right,
                                bool dark_bg) = 0;

    virtual void set_color(int row, int col_begin, int col_end, TEXT_COLOR color) = 0;
    virtual void clear_color() = 0;

    // Called by controller after all add_row() calls. Default is a no-op;
    // border_decorator overrides this to append its bottom border rows.
    virtual void finalize_rows() {
    }
};

#endif
