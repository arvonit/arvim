#include "view_decorator.h"
#include <cstdio>
#include <string>

// ---- line_number_decorator ----

void line_number_decorator::add_row(const std::string& row) {
    char prefix[line_number_decorator::NUM_WIDTH + 1];
    snprintf(prefix, sizeof(prefix), "%5d ", current_row + 1);
    current_row++;
    inner->add_row(std::string(prefix) + row);
}

// ---- border_decorator ----

static std::string make_dash_row(int width) {
    // "+-....-+" with total length = width
    // Two border chars on each side: "| " and " |" = 4 chars consumed.
    // Dash row is just dashes for the full inner width.
    return std::string(width, '-');
}

void border_decorator::init_rows() {
    inner->init_rows();
    int w = inner->get_col_count();
    inner->add_row(make_dash_row(w));
    inner->add_row(make_dash_row(w));
}

void border_decorator::add_row(const std::string& row) {
    // Pad or truncate to get_col_count() width, then wrap with "| " and " |"
    int content_w = get_col_count();
    std::string padded = row;
    if ((int)padded.size() > content_w)
        padded = padded.substr(0, content_w);
    while ((int)padded.size() < content_w)
        padded += ' ';
    inner->add_row("| " + padded + " |");
}

void border_decorator::finalize_rows() {
    int w = inner->get_col_count();
    inner->add_row(make_dash_row(w));
    inner->add_row(make_dash_row(w));
    inner->finalize_rows();
}
