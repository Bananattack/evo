#ifndef TESTS_H
#define TESTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <evo_api.h>

#define TEST(func) \
    int func(int argc, char** argv)

void StartTime();
double EndTime();

TEST(self_avoiding_walk);
TEST(prisoner);

typedef struct 
{
    const char* name;
    int (*cb)(int, char**);
} TestData;

#endif