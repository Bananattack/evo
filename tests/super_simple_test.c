#include <evo_select_tournament.h>
#include "tests.h"

#define BOARD_WIDTH 4
#define BOARD_HEIGHT 6
#define GENE_SIZE 42

evo_bool Initializer(evo_Context* context, evo_uint populationSize)
{
    evo_uint i, j;
    char* gene;
    if(!context->genes)
    {
        context->genes = malloc(sizeof(void*) * populationSize);
        for(i = 0; i < populationSize; i++)
        {
            gene = (char*) malloc(sizeof(char) * (GENE_SIZE + 1));
            context->genes[i] = gene;
        }
        
    }
    for(i = 0; i < populationSize; i++)
    {
        gene = context->genes[i];
        for(j = 0; j < GENE_SIZE; j++)
        {
            switch(evo_RandomInt(context, 0, 4))
            {
                case 0: gene[j] = 'U'; break;
                case 1: gene[j] = 'D'; break;
                case 2: gene[j] = 'L'; break;
                case 3: gene[j] = 'R'; break;
            }
        }
        gene[GENE_SIZE] = 0;
    }
    
    return 1;
}

void Finalizer(evo_Context* context, evo_uint populationSize)
{
    evo_uint i;
    for(i = 0; i < populationSize; i++)
    {
        free(context->genes[i]);
    }
    free(context->genes);
    return;
}

double Fitness(evo_Context* context, void* d)
{
    char* gene;
    double fitness;
    evo_uint x, y, i;
    evo_bool board[BOARD_WIDTH * BOARD_HEIGHT];

    x = y = 0;
    fitness = 0.0;
    gene = (char*) d;
    memset(board, 0, sizeof(evo_bool) * BOARD_WIDTH * BOARD_HEIGHT);
    
    /* Move through the board. */
    for(i = 0; i < GENE_SIZE; i++)
    {
        board[y * BOARD_WIDTH + x] = 1;
        switch(gene[i])
        {
            case 'U':
                if(y > 0)
                {
                    y--;
                }
                break;
            case 'D':
                if(y < BOARD_HEIGHT - 1)
                {
                    y++;
                }
                break;
            case 'L':
                if(x > 0)
                {
                    x--;
                }
                break;
            case 'R':
                if(x < BOARD_WIDTH - 1)
                {
                    x++;
                }
                break;
        }
    }

    /* Reward one point for each space covered. */
    for(i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++)
    {
        if(board[i])
        {
            fitness++;
        }
    }
    return fitness;
}

/* Two-point Crossover */
void Crossover(evo_Context* context,
    void* parentA, void* parentB, void* childA, void* childB)
{
    evo_uint cp1, cp2, i;
    char* pa = parentA;
    char* pb = parentB;
    char* ca = childA;
    char* cb = childB;
    
    cp1 = evo_RandomInt(context, 0, GENE_SIZE);
    cp2 = evo_RandomInt(context, 0, GENE_SIZE);
    if(cp1 > cp2)
    {
        i = cp1;
        cp1 = cp2;
        cp2 = i;
    }

    for(i = 0; i < cp1; i++)
    {
        ca[i] = pa[i];
        cb[i] = pb[i];
    }
    for(i = cp1; i < cp2; i++)
    {
        ca[i] = pb[i];
        cb[i] = pa[i];
    }
    for(i = cp2; i < GENE_SIZE; i++)
    {
        ca[i] = pa[i];
        cb[i] = pb[i];;
    }
}

void Mutation(evo_Context* context, void* d)
{
    char* gene = d;
    evo_uint i;
    evo_uint count = evo_RandomInt(context, 0, 5) + 1;
    while(count)
    {
        count--;
        i = evo_RandomInt(context, 0, GENE_SIZE);
        switch(evo_RandomInt(context, 0, 4))
        {
            case 0: gene[i] = 'U'; break;
            case 1: gene[i] = 'D'; break;
            case 2: gene[i] = 'L'; break;
            case 3: gene[i] = 'R'; break;
        }
    }
}

evo_bool Success(evo_Context* context)
{
    return context->bestFitness == BOARD_WIDTH * BOARD_HEIGHT;
}

TEST(super_simple_main)
{
    evo_Stats* stats;
	evo_Config* config = evo_Config_New();
/*
Context 0 running 100 trials with seed 0
Context 1 running 100 trials with seed 100071
Context 2 running 100 trials with seed 200142
Context 3 running 100 trials with seed 300213
Context 4 running 100 trials with seed 400284
Context 11 running 100 trials with seed 1100781
Context 5 running 100 trials with seed 500355
Context 6 running 100 trials with seed 600426
Context 7 running 100 trials with seed 700497
Context 8 running 100 trials with seed 800568
Context 9 running 100 trials with seed 900639
Context 10 running 100 trials with seed 1000710
Context 13 running 100 trials with seed 1300923
Context 12 running 100 trials with seed 1200852
Context 14 running 100 trials with seed 1400994
Context 15 running 100 trials with seed 1501065
DING DING DING DING

Failures 344/1600
Best fitness 24.000000
*/
/*
Context 0 running 100 trials with seed 0
Context 0 running 100 trials with seed 100071
Context 0 running 100 trials with seed 200142
Context 0 running 100 trials with seed 300213
Context 0 running 100 trials with seed 400284
Context 0 running 100 trials with seed 500355
Context 0 running 100 trials with seed 600426
Context 0 running 100 trials with seed 700497
Context 0 running 100 trials with seed 800568
Context 0 running 100 trials with seed 900639
Context 0 running 100 trials with seed 1000710
Context 0 running 100 trials with seed 1100781
Context 0 running 100 trials with seed 1200852
Context 0 running 100 trials with seed 1300923
Context 0 running 100 trials with seed 1400994
Context 0 running 100 trials with seed 1501065
DING DING DING DING

Failures 344/100
Best fitness 24.000000
*/
    evo_Config_SetUnitCount(config, 16);
    evo_Config_SetRandomStreamCount(config, 16);
    evo_Config_SetTrials(config, 1600);
    evo_Config_SetMaxIterations(config, 1000);
    evo_Config_SetPopulationSize(config, 1000);

    evo_Config_SetPopulationInitializer(config, Initializer);
    evo_Config_SetPopulationFinalizer(config, Finalizer);
    evo_Config_SetFitnessOperator(config, Fitness);
    evo_UseTournamentSelection(config, 1000, 4);
    evo_Config_SetCrossoverOperator(config, Crossover);
    evo_Config_SetMutationOperator(config, Mutation);
    evo_Config_SetSuccessPredicate(config, Success);

    evo_Config_Execute(config);
    if(!evo_Config_IsUsed(config))
    {
        fprintf(stderr, "Could not use the given config.\n");
        return 0;
    }

    stats = evo_Config_GetStats(config);
    printf("\n", stats->failures, stats->trials);
    printf("Failures %u/%u\n", stats->failures, stats->trials);
    printf("Best fitness %lf\n", stats->bestFitness, stats->trials);

	evo_Config_Free(config);
	return 0;
}