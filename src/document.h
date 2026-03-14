#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <fstream>
#include <string>
#include <vector>

class document {
public:
    explicit document(const std::string& path);
    ~document();

    char get_char_at(int row, int col) const {
        return lines[row][col];
    }
    void insert_char_at(int row, int col, char ch) {
        lines[row].insert(col, 1, ch);
    }
    void remove_char_at(int row, int col) {
        lines[row].erase(col, 1);
    }
    void insert_line_at(int row) {
        lines.insert(lines.begin() + row, std::string());
    }
    void remove_line(int row) {
        lines.erase(lines.begin() + row);
    }
    void split_line_at(int row, int col);
    void merge_line_above(int row);

    int line_length(int row) const {
        return (int)lines[row].size();
    }
    int num_lines() const {
        return (int)lines.size();
    }
    const std::vector<std::string>& get_lines() const {
        return lines;
    }

private:
    std::vector<std::string> lines;
    std::string path;
};

#endif
