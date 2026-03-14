# ArVim

A vim-like terminal text editor written in C++. 

## Functionality

ArVim has a modal interface for text-editing: `i` goes into insert mode and `ESC` goes back to 
normal mode. Supports some vim normal-mode navigation including navigation with `h/j/k/l` or arrow 
keys, `w/b` for going between words, `0/$` for beginning/end of line, and `gg/G` for top/bottom 
of file. Copying and pasting a line is supported with normal vim keybinds as well as undo/redo. 
`Ctrl+L` shows line numbers, `Ctrl+B` shows a border, and `Ctrl+Q` quits the app.

## Usage

```bash
arvim <file_name>
```

## Build Instructions

```bash
cmake -B build
cmake --build build
```
