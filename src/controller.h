#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "commands.h"
#include "document.h"
#include "observer.h"
#include "terminal_view.h"
#include "view_decorator.h"
#include "view_interface.h"
#include <set>
#include <string>
#include <vector>

class controller : public ec_observer {
public:
    controller(document& doc, terminal_view& view);
    ~controller();

    void refresh();
    void update() override;

private:
    void enter_insert_mode();
    void exit_insert_mode();

    void move_cursor_up();
    void move_cursor_down();
    void move_cursor_left();
    void move_cursor_right();
    void move_word_forward();
    void move_word_backward();
    void clamp_viewport();
    void sync_view_cursor();

    void load_keywords(const std::string& path);
    void apply_keyword_highlighting();
    void rewrap_view();

    document& doc;
    terminal_view* raw_view;
    view_interface* view;

    command_history history;
    std::set<std::string> keywords;

    int cursor_doc_row;
    int cursor_doc_col;
    int viewport_start;

    bool insert_mode;

    std::string clipboard;
    int last_normal_key;

    bool line_numbers_on;
    bool border_on;
    std::vector<int> activation_order;
    line_number_decorator* ln_decorator;
    border_decorator* bd_decorator;
};

#endif
