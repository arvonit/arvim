#include "controller.h"
#include "document.h"
#include "terminal_view.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: arvim <file>\n";
        return 1;
    }
    terminal_view view;
    document doc(argv[1]);
    controller ctrl(doc, view);
    return 0;
}
