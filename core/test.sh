#!/bin/sh
# Run from Git Bash: MSYS_NO_PATHCONV=1 docker run --rm -v "C:\Games\InkDelve\core:/src:ro" ubuntu:22.04 sh /src/test.sh
apt-get update -qq >/dev/null && apt-get install -y -qq g++ >/dev/null 2>&1
set -e
g++ -std=c++11 -Wall -Wextra -o /tmp/t1 /src/grid.cpp /src/test_grid.cpp && /tmp/t1
g++ -std=c++11 -Wall -Wextra -o /tmp/t2 /src/grid.cpp /src/game.cpp /src/test_game.cpp && /tmp/t2
