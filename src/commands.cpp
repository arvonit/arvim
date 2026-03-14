#include "commands.h"
#include <algorithm>

// ---- command_group ----

command_group::~command_group() {
    for (auto* c : cmds)
        delete c;
}

void command_group::execute() {
    for (auto* c : cmds)
        c->execute();
}

void command_group::un_execute() {
    for (auto it = cmds.rbegin(); it != cmds.rend(); ++it)
        (*it)->un_execute();
}

// ---- command_history ----

command_history::command_history() : index(-1), active_group(nullptr) {
}

command_history::~command_history() {
    for (auto* c : history)
        delete c;
}

void command_history::undo() {
    if (index < 0)
        return;
    history[index--]->un_execute();
}

void command_history::redo() {
    if (index >= (int)history.size() - 1)
        return;
    history[++index]->execute();
}

void command_history::begin_group() {
    active_group = new command_group();
}

void command_history::end_group() {
    if (!active_group)
        return;
    if (active_group->size() > 0) {
        // Prune any forward history.
        while ((int)history.size() > index + 1) {
            delete history.back();
            history.pop_back();
        }
        history.push_back(active_group);
        index++;
    } else {
        delete active_group;
    }
    active_group = nullptr;
}

void command_history::execute_command(command* cmd, bool save) {
    cmd->execute();
    if (!save) {
        delete cmd;
        return;
    }

    if (active_group) {
        active_group->add_command(cmd);
    } else {
        // Direct command (paste, etc.) — individually undoable.
        while ((int)history.size() > index + 1) {
            delete history.back();
            history.pop_back();
        }
        history.push_back(cmd);
        index++;
    }
}

// ---- insert_command ----

insert_command::insert_command(document& doc, int& cursor_row, int& cursor_col, int row, int col,
                               char ch)
    : doc(doc), cursor_row(cursor_row), cursor_col(cursor_col), row(row), col(col), ch(ch) {
}

void insert_command::execute() {
    doc.insert_char_at(row, col, ch);
    col++;
    cursor_row = row;
    cursor_col = col;
}

void insert_command::un_execute() {
    doc.remove_char_at(row, col - 1);
    col--;
    cursor_row = row;
    cursor_col = col;
}

// ---- backspace_command ----

backspace_command::backspace_command(document& doc, int& cursor_row, int& cursor_col, int row,
                                     int col)
    : doc(doc), cursor_row(cursor_row), cursor_col(cursor_col), row(row), col(col), deleted_char(0),
      merged_lines(false) {
}

void backspace_command::execute() {
    if (col == 0) {
        if (row > 0) {
            merged_lines = true;
            int old_len = doc.line_length(row - 1);
            doc.merge_line_above(row);
            row--;
            col = old_len;
        }
    } else {
        deleted_char = doc.get_char_at(row, col - 1);
        doc.remove_char_at(row, col - 1);
        col--;
    }
    cursor_row = row;
    cursor_col = col;
}

void backspace_command::un_execute() {
    if (merged_lines) {
        doc.split_line_at(row, col);
        row++;
        col = 0;
    } else {
        doc.insert_char_at(row, col, deleted_char);
        col++;
    }
    cursor_row = row;
    cursor_col = col;
}

// ---- enter_command ----

enter_command::enter_command(document& doc, int& cursor_row, int& cursor_col, int row, int col)
    : doc(doc), cursor_row(cursor_row), cursor_col(cursor_col), row(row), col(col) {
}

void enter_command::execute() {
    if (col == doc.line_length(row)) {
        doc.insert_line_at(row + 1);
    } else {
        doc.split_line_at(row, col);
    }
    row++;
    cursor_row = row;
    cursor_col = 0;
}

void enter_command::un_execute() {
    if (col == doc.line_length(row)) {
        doc.remove_line(row);
    } else {
        doc.merge_line_above(row);
    }
    row--;
    cursor_row = row;
    cursor_col = col;
}

// ---- put_command ----

put_command::put_command(document& doc, int& cursor_row, int& cursor_col, int row,
                         const std::string& line, int direction)
    : doc(doc), cursor_row(cursor_row), cursor_col(cursor_col), orig_row(row), orig_col(cursor_col),
      line(line) {
    inserted_at = (direction >= 0) ? row + 1 : row;
}

void put_command::execute() {
    doc.insert_line_at(inserted_at);
    // Set the line content character by character
    for (int i = 0; i < (int)line.size(); ++i) {
        doc.insert_char_at(inserted_at, i, line[i]);
    }
    cursor_row = inserted_at;
    cursor_col = 0;
}

void put_command::un_execute() {
    doc.remove_line(inserted_at);
    cursor_row = orig_row;
    cursor_col = orig_col;
}
