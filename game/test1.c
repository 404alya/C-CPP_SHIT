#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void FormatFloat(char *buf, int size, float v)
{
    // without an exponent if possible (100, not 1e+02)
    for (int pass = 0; pass < 2; pass++)
        for (int p = 1; p <= 9; p++) {
            snprintf(buf, size, "%.*g", p, v);
            if (strtof(buf, NULL) == v && (pass == 1 || !strchr(buf, 'e'))) return;
        }
}

int main() {

  return 0;
}
