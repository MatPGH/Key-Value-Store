# Simple C++ Key-Value Store

A minimal persistent key-value store with append-only storage.

## Commands
- `SET <key> <value>` -> no output on success
- `GET <key>` -> `<value>` or `ERROR` when missing
- `EXIT` -> exit with no output

Keys and values are single tokens (no spaces).

## Build
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o kv_store.exe

## Run
.\kv_store.exe

## Gradebot
.\gradebot.exe project-1 --dir "." --run ".\\kv_store.exe"