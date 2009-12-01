#ifndef TESTS_H
#define TESTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <evo_api.h>

#define TEST(func) \
    int func(int argc, char** argv)

TEST(self_avoiding_walk);
TEST(prisoner);

typedef struct 
{
    const char* name;
    int (*cb)(int, char**);
} TestData;

static const TestData testList[] = {
    {"saw", self_avoiding_walk},
    {"prisoner", prisoner},
    {NULL, NULL},
};


#endif