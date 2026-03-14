#ifndef VIEW_DECORATOR_H
#define VIEW_DECORATOR_H

#include "view_interface.h"
#include <string>

// Abstract decorator base — passes all calls through to inner by default.
class view_decorator : public view_interface {
public:
    explicit view_decorator(view_interface* inner) : inner(inner) {
    }
    virtual ~view_decorator() {
    }

    void init_rows() override {
        inner->init_rows();
    }
    void add_row(const std::string& row) override {
        inner->add_row(row);
    }
    int get_row_count() const override {
        return inner->get_row_count();
    }
    int get_col_count() const override {
        return inner->get_col_count();
    }
    int get_cursor_x() const override {
        return inner->get_cursor_x();
    }
    int get_cursor_y() const override {
        return inner->get_cursor_y();
    }
    void set_cursor_x(int x) override {
        inner->set_cursor_x(x);
    }
    void set_cursor_y(int y) override {
        inner->set_cursor_y(y);
    }
    void clear_status_rows() override {
        inner->clear_status_rows();
    }
    void add_status_row(const std::string& l, const std::string& r, bool dark_bg) override {
        inner->add_status_row(l, r, dark_bg);
    }
    void set_color(int row, int cb, int ce, TEXT_COLOR c) override {
        inner->set_color(row, cb, ce, c);
    }
    void clear_color() override {
        inner->clear_color();
    }
    void finalize_rows() override {
        inner->finalize_rows();
    }

protected:
    view_interface* inner;
};

// ---- Line number decorator ---- (toggled with Ctrl+L)
// Prepends a 6-character line number prefix ("    1 ") to each row.
// Reduces visible column count by 6 and offsets cursor X and color columns.
class line_number_decorator : public view_decorator {
public:
    static const int NUM_WIDTH = 6;

    explicit line_number_decorator(view_interface* inner)
        : view_decorator(inner), current_row(0), line_offset(0) {
    }

    // Call before init_rows() so numbers reflect document line numbers.
    void set_line_offset(int offset) {
        line_offset = offset;
    }

    void init_rows() override {
        current_row = line_offset;
        inner->init_rows();
    }

    void add_row(const std::string& row) override;

    int get_col_count() const override {
        return inner->get_col_count() - NUM_WIDTH;
    }
    int get_cursor_x() const override {
        return inner->get_cursor_x() - NUM_WIDTH;
    }
    void set_cursor_x(int x) override {
        inner->set_cursor_x(x + NUM_WIDTH);
    }

    void set_color(int row, int col_begin, int col_end, TEXT_COLOR color) override {
        inner->set_color(row, col_begin + NUM_WIDTH, col_end + NUM_WIDTH, color);
    }

private:
    int current_row;
    int line_offset;
};

// ---- Border decorator ---- (toggled with Ctrl+B)
// Surrounds the view with a 2-character wide border:
//   - Top: 2 rows of dashes added in init_rows()
//   - Sides: "| " prefix and " |" suffix on each content row
//   - Bottom: 2 rows of dashes added in finalize_rows()
// Reduces visible row count by 4 and column count by 4.
// Offsets cursor X and Y by 2.
class border_decorator : public view_decorator {
public:
    static const int BORDER_WIDTH = 2;

    explicit border_decorator(view_interface* inner) : view_decorator(inner) {
    }

    void init_rows() override;
    void add_row(const std::string& row) override;
    void finalize_rows() override;

    int get_row_count() const override {
        return inner->get_row_count() - 4;
    }
    int get_col_count() const override {
        return inner->get_col_count() - 4;
    }
    int get_cursor_x() const override {
        return inner->get_cursor_x() - BORDER_WIDTH;
    }
    int get_cursor_y() const override {
        return inner->get_cursor_y() - BORDER_WIDTH;
    }
    void set_cursor_x(int x) override {
        inner->set_cursor_x(x + BORDER_WIDTH);
    }
    void set_cursor_y(int y) override {
        inner->set_cursor_y(y + BORDER_WIDTH);
    }

    void set_color(int row, int col_begin, int col_end, TEXT_COLOR color) override {
        inner->set_color(row + BORDER_WIDTH,
                         col_begin + BORDER_WIDTH,
                         col_end + BORDER_WIDTH,
                         color);
    }
};

#endif
