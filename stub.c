#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dlist.h"
#include "statistics.h"

int main()
{
    int i;
    for (i = 0; i < 10; i++) {
        STATISTICS_INC(hello);
        printf("%lu", STATISTICS_SUM(hello));
    }
    return 0;
}
