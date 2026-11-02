# Usage: ./build.sh name [args]

set -ex

clang -Wall -Wextra $1 ${@:2} -o output.o
