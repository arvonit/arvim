#include "controller.h"
#include "commands.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>

using namespace std;

controller::controller(document& doc, terminal_view& view)
    : doc(doc), raw_view(&view), view(&view), cursor_doc_row(0), cursor_doc_col(0),
      viewport_start(0), insert_mode(false), last_normal_key(0), line_numbers_on(false),
      border_on(false), ln_decorator(nullptr), bd_decorator(nullptr) {
    load_keywords("keywords.txt");
    raw_view->attach(this);
    refresh();
    raw_view->show();
}

controller::~controller() {
    delete ln_decorator;
    delete bd_decorator;
}

void controller::rewrap_view() {
    delete ln_decorator;
    ln_decorator = nullptr;
    delete bd_decorator;
    bd_decorator = nullptr;

    view_interface* base = raw_view;
    for (int token : activation_order) {
        if (token == 'L' && line_numbers_on) {
            ln_decorator = new line_number_decorator(base);
            base = ln_decorator;
        } else if (token == 'B' && border_on) {
            bd_decorator = new border_decorator(base);
            base = bd_decorator;
        }
    }
    view = base;
}

void controller::sync_view_cursor() {
    view->set_cursor_y(cursor_doc_row - viewport_start);
    view->set_cursor_x(cursor_doc_col);
}

void controller::clamp_viewport() {
    int visible = view->get_row_count();
    if (cursor_doc_row < viewport_start)
        viewport_start = cursor_doc_row;
    else if (cursor_doc_row >= viewport_start + visible)
        viewport_start = cursor_doc_row - visible + 1;
}

void controller::refresh() {
    view->clear_status_rows();
    view->clear_color();

    if (line_numbers_on && ln_decorator)
        ln_decorator->set_line_offset(viewport_start);

    view->init_rows();

    int visible = view->get_row_count();
    int total = doc.num_lines();
    int end = min(viewport_start + visible, total);

    for (int r = viewport_start; r < end; r++)
        view->add_row(doc.get_lines()[r]);

    view->finalize_rows();
    apply_keyword_highlighting();
    view->add_status_row(insert_mode ? "-- INSERT --" : "NORMAL", "", true);
    sync_view_cursor();
}

void controller::update() {
    int key = raw_view->get_pressed_key();

    if (key == ARROW_LEFT) {
        move_cursor_left();
        clamp_viewport();
        refresh();
        return;
    }
    if (key == ARROW_RIGHT) {
        move_cursor_right();
        clamp_viewport();
        refresh();
        return;
    }
    if (key == ARROW_UP) {
        move_cursor_up();
        clamp_viewport();
        refresh();
        return;
    }
    if (key == ARROW_DOWN) {
        move_cursor_down();
        clamp_viewport();
        refresh();
        return;
    }

    if (insert_mode) {
        if (key == ESC || key == CTRL_A) {
            exit_insert_mode();
        } else if (key == ENTER) {
            history.execute_command(new enter_command(doc,
                                                      cursor_doc_row,
                                                      cursor_doc_col,
                                                      cursor_doc_row,
                                                      cursor_doc_col));
        } else if (key == BACKSPACE) {
            history.execute_command(new backspace_command(doc,
                                                          cursor_doc_row,
                                                          cursor_doc_col,
                                                          cursor_doc_row,
                                                          cursor_doc_col));
        } else if (key >= 32 && key < 127) {
            history.execute_command(new insert_command(doc,
                                                       cursor_doc_row,
                                                       cursor_doc_col,
                                                       cursor_doc_row,
                                                       cursor_doc_col,
                                                       (char)key));
        }
    } else {
        // Normal mode: handle double-key sequences first.
        if (last_normal_key != 0) {
            int prev = last_normal_key;
            last_normal_key = 0;
            if (prev == 'y' && key == 'y') {
                if (cursor_doc_row < doc.num_lines())
                    clipboard = doc.get_lines()[cursor_doc_row];
                refresh();
                return;
            } else if (prev == 'g' && key == 'g') {
                cursor_doc_row = 0;
                cursor_doc_col = 0;
                clamp_viewport();
                refresh();
                return;
            }
            // Not a valid sequence — fall through and process key normally.
        }

        if (key == 'h') {
            move_cursor_left();
        } else if (key == 'l') {
            move_cursor_right();
        } else if (key == 'k') {
            move_cursor_up();
        } else if (key == 'j') {
            move_cursor_down();
        } else if (key == 'w') {
            move_word_forward();
        } else if (key == 'b') {
            move_word_backward();
        } else if (key == '0') {
            cursor_doc_col = 0;
        } else if (key == '$') {
            cursor_doc_col = max(0, doc.line_length(cursor_doc_row) - 1);
        } else if (key == 'G') {
            cursor_doc_row = doc.num_lines() - 1;
            cursor_doc_col = min(cursor_doc_col, max(0, doc.line_length(cursor_doc_row) - 1));
        } else if (key == 'i') {
            enter_insert_mode();
        } else if (key == 'u') {
            history.undo();
            cursor_doc_row = max(0, min(cursor_doc_row, doc.num_lines() - 1));
            cursor_doc_col = min(cursor_doc_col, doc.line_length(cursor_doc_row));
        } else if (key == CTRL_R) {
            history.redo();
            cursor_doc_row = max(0, min(cursor_doc_row, doc.num_lines() - 1));
            cursor_doc_col = min(cursor_doc_col, doc.line_length(cursor_doc_row));
        } else if (key == 'p') {
            if (!clipboard.empty())
                history.execute_command(new put_command(doc,
                                                        cursor_doc_row,
                                                        cursor_doc_col,
                                                        cursor_doc_row,
                                                        clipboard,
                                                        1));
        } else if (key == 'P') {
            if (!clipboard.empty())
                history.execute_command(new put_command(doc,
                                                        cursor_doc_row,
                                                        cursor_doc_col,
                                                        cursor_doc_row,
                                                        clipboard,
                                                        -1));
        } else if (key == 'y') {
            last_normal_key = 'y';
            return;
        } else if (key == 'g') {
            last_normal_key = 'g';
            return;
        } else if (key == CTRL_L) {
            line_numbers_on = !line_numbers_on;
            if (line_numbers_on)
                activation_order.push_back('L');
            else
                activation_order.erase(remove(activation_order.begin(),
                                              activation_order.end(),
                                              'L'),
                                       activation_order.end());
            rewrap_view();
        } else if (key == CTRL_B) {
            border_on = !border_on;
            if (border_on)
                activation_order.push_back('B');
            else
                activation_order.erase(remove(activation_order.begin(),
                                              activation_order.end(),
                                              'B'),
                                       activation_order.end());
            rewrap_view();
        }
    }

    clamp_viewport();
    refresh();
}

void controller::enter_insert_mode() {
    insert_mode = true;
    history.begin_group();
}

void controller::exit_insert_mode() {
    insert_mode = false;
    history.end_group();
    cursor_doc_col = max(0, cursor_doc_col - 1);
    cursor_doc_col = min(cursor_doc_col, doc.line_length(cursor_doc_row));
}

void controller::move_cursor_left() {
    if (cursor_doc_col > 0)
        cursor_doc_col--;
}

void controller::move_cursor_right() {
    if (cursor_doc_col < doc.line_length(cursor_doc_row))
        cursor_doc_col++;
}

void controller::move_cursor_up() {
    if (cursor_doc_row > 0) {
        cursor_doc_row--;
        cursor_doc_col = min(cursor_doc_col, doc.line_length(cursor_doc_row));
    }
}

void controller::move_cursor_down() {
    if (cursor_doc_row < doc.num_lines() - 1) {
        cursor_doc_row++;
        cursor_doc_col = min(cursor_doc_col, doc.line_length(cursor_doc_row));
    }
}

void controller::move_word_forward() {
    const auto& lines = doc.get_lines();
    int row = cursor_doc_row;
    int col = cursor_doc_col;
    int len = (int)lines[row].size();

    // Skip current word characters.
    while (col < len && !isspace((unsigned char)lines[row][col]))
        col++;
    // Skip whitespace to next word.
    while (col < len && isspace((unsigned char)lines[row][col]))
        col++;

    if (col >= len && row < doc.num_lines() - 1) {
        // Wrap to next line's first non-space.
        row++;
        col = 0;
        len = (int)lines[row].size();
        while (col < len && isspace((unsigned char)lines[row][col]))
            col++;
    }

    cursor_doc_row = row;
    cursor_doc_col = min(col, max(0, (int)lines[row].size() - 1));
}

void controller::move_word_backward() {
    const auto& lines = doc.get_lines();
    int row = cursor_doc_row;
    int col = cursor_doc_col;

    if (col == 0) {
        if (row == 0)
            return;
        row--;
        col = max(0, (int)lines[row].size() - 1);
    } else {
        col--;
    }

    // Skip whitespace backwards.
    while (col > 0 && isspace((unsigned char)lines[row][col]))
        col--;
    // Skip word backwards.
    while (col > 0 && !isspace((unsigned char)lines[row][col - 1]))
        col--;

    cursor_doc_row = row;
    cursor_doc_col = col;
}

void controller::load_keywords(const string& path) {
    ifstream file(path);
    if (!file.is_open())
        return;
    string word;
    while (getline(file, word))
        if (!word.empty())
            keywords.insert(word);
}

void controller::apply_keyword_highlighting() {
    int visible = view->get_row_count();
    int total = doc.num_lines();
    int end = min(viewport_start + visible, total);

    for (int doc_row = viewport_start; doc_row < end; doc_row++) {
        int view_row = doc_row - viewport_start;
        const string& line = doc.get_lines()[doc_row];
        int col = 0;
        int len = (int)line.size();

        while (col < len) {
            while (col < len && isspace((unsigned char)line[col]))
                col++;
            int word_start = col;
            string word;
            while (col < len && !isspace((unsigned char)line[col]))
                word += line[col++];
            if (!word.empty() && keywords.count(word))
                view->set_color(view_row,
                                word_start,
                                word_start + (int)word.size() - 1,
                                TEXT_COLOR_BLUE);
        }
    }
}
