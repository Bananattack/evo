#ifndef TESTS_H
#define TESTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <evo_api.h>

#define TEST(func) \
    int func(int argc, char** argv)

TEST(super_simple_main);


#endif