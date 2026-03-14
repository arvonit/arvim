#include "document.h"

document::document(const std::string& path) : path(path) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back(std::string());
    }
}

document::~document() {
    std::ofstream file(path);
    for (const auto& line : lines) {
        file << line << '\n';
    }
}

void document::split_line_at(int row, int col) {
    std::string tail = lines[row].substr(col);
    lines[row].erase(col);
    lines.insert(lines.begin() + row + 1, tail);
}

void document::merge_line_above(int row) {
    lines[row - 1] += lines[row];
    lines.erase(lines.begin() + row);
}
