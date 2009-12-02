#include <string.h>
#include <time.h>
#include "tests.h"

// Helper function, don't call directly
double TimeHelper(int reset)
{
	static clock_t start = 0.0;
	if (reset)
	{
		start = clock();
		return 0.0;
	}
	return (double)(clock() - start)/CLOCKS_PER_SEC;
}

// StartTime resets timer
// EndTime returns no. of seconds of CPU time since StartTime
void StartTime()
{
	TimeHelper(1); // reset timer
}

double EndTime()
{
	return TimeHelper( 0 );
}

int main(int argc, char** argv)
{
    static const TestData testList[] = {
        {"saw", self_avoiding_walk},
        {"prisoner", prisoner},
        {NULL, NULL},
    };

    int i = 0;
    if(argc < 2)
    {
        fprintf(stderr, "Usage:\n    %s TEST\n", argv[0]);
        fprintf(stderr, "\n    Where test is one of:\n", argv[0]);
        while(testList[i].name && testList[i].cb)
        {
            fprintf(stderr, "        %s\n", testList[i].name);
            i++;
        }
        fprintf(stderr, "\n");
        return -1;
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