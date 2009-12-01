#include <string.h>
#include "tests.h"

int main(int argc, char** argv)
{
    int i = 0;
    if(argc < 2)
    {
        printf("Usage:\n    %s TEST\n", argv[0]);
        printf("\n    Where test is one of:\n", argv[0]);
        while(testList[i].name && testList[i].cb)
        {
            printf("        %s\n", testList[i].name);
            i++;
        }
        printf("\n");
        return;
    }
    while(testList[i].name && testList[i].cb)
    {
        if(!strcmp(testList[i].name, argv[1]))
        {
            printf("Running test '%s'\n", argv[1]);
            return testList[i].cb(argc, argv);
        }
        i++;
    }
    fprintf(stderr, "Failed to find test '%s'\n", argv[1]);
    return -1;
}