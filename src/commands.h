#ifndef COMMANDS_H
#define COMMANDS_H

#include "document.h"
#include <string>
#include <vector>

class command {
public:
    virtual ~command() {
    }
    virtual void execute() = 0;
    virtual void un_execute() = 0;
};

// Batch of commands executed/undone together (one insert-mode session).
class command_group : public command {
public:
    command_group() {
    }
    ~command_group();
    void add_command(command* cmd) {
        cmds.push_back(cmd);
    }
    int size() const {
        return (int)cmds.size();
    }
    void execute() override;
    void un_execute() override;

private:
    std::vector<command*> cmds;
};

// Manages undo/redo history. Uses begin_group()/end_group() for insert-mode batching.
class command_history {
public:
    command_history();
    ~command_history();

    void undo();
    void redo();

    // Execute a command and optionally save it.
    // If a group is active (insert mode), the command is added to the group.
    // Otherwise it goes directly into the history stack.
    void execute_command(command* cmd, bool save = true);

    // Call when entering insert mode — opens a new batch.
    void begin_group();
    // Call when leaving insert mode — commits batch if non-empty.
    void end_group();

private:
    std::vector<command*> history;
    int index;
    command_group* active_group;
};

// Insert a single character into the document.
class insert_command : public command {
public:
    insert_command(document& doc, int& cursor_row, int& cursor_col, int row, int col, char ch);
    void execute() override;
    void un_execute() override;

private:
    document& doc;
    int& cursor_row;
    int& cursor_col;
    int row, col;
    char ch;
};

// Backspace — delete char before cursor or merge line with the one above.
class backspace_command : public command {
public:
    backspace_command(document& doc, int& cursor_row, int& cursor_col, int row, int col);
    void execute() override;
    void un_execute() override;

private:
    document& doc;
    int& cursor_row;
    int& cursor_col;
    int row, col;
    char deleted_char;
    bool merged_lines;
};

// Enter — split line at cursor or append empty line.
class enter_command : public command {
public:
    enter_command(document& doc, int& cursor_row, int& cursor_col, int row, int col);
    void execute() override;
    void un_execute() override;

private:
    document& doc;
    int& cursor_row;
    int& cursor_col;
    int row, col;
};

// Paste a line from the clipboard below (direction=1) or above (direction=-1) current row.
class put_command : public command {
public:
    put_command(document& doc, int& cursor_row, int& cursor_col, int row, const std::string& line,
                int direction);
    void execute() override;
    void un_execute() override;

private:
    document& doc;
    int& cursor_row;
    int& cursor_col;
    int orig_row;
    int orig_col;
    std::string line;
    int inserted_at;
};

#endif
