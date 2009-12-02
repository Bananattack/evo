#include <evo_select_tournament.h>
#include "tests.h"

/*#define THREADS 16*/

static int THREADS = 0;
#define TRIALS (16*1000)
#define MAX_ITERATIONS 1000
#define POPULATION 1000

#define BOARD_WIDTH 6
#define BOARD_HEIGHT 6
#define GENE_SIZE 42

static evo_bool Initializer(evo_Context* context)
{
    evo_uint populationSize;
    evo_uint i, j;
    char* gene;

    populationSize = evo_Context_GetPopulationSize(context);
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

static void Finalizer(evo_Context* context)
{
    evo_uint i;
    evo_uint populationSize;
    populationSize = evo_Context_GetPopulationSize(context);
    for(i = 0; i < populationSize; i++)
    {
        free(context->genes[i]);
    }
    free(context->genes);
    return;
}

static double IndividualFitness(char* gene)
{
    double fitness;
    evo_uint x, y, i;
    evo_bool board[BOARD_WIDTH * BOARD_HEIGHT];

    x = y = 0;
    fitness = 0.0;
    memset(board, 0, sizeof(evo_bool) * BOARD_WIDTH * BOARD_HEIGHT);
    
    board[0] = 1;
    /* Move through the board. */
    for(i = 0; i < GENE_SIZE; i++)
    {
        /* Parse move string. */
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
        /* Mark this spot. */
        board[y * BOARD_WIDTH + x] = 1;
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

static void PopulationFitness(evo_Context* context)
{
    evo_uint i;
    for(i = 0; i < evo_Context_GetPopulationSize(context); i++)
    {
        context->fitnesses[i] = IndividualFitness((char*) (context->genes[i]));
    }
}

/* Two-point Crossover */
static void Crossover(evo_Context* context,
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
        cb[i] = pb[i];
    }
}

static void Mutation(evo_Context* context, void* d)
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

static evo_bool Success(evo_Context* context)
{
    return context->bestFitness == BOARD_WIDTH * BOARD_HEIGHT;
}

TEST(self_avoiding_walk)
{
    double t;
    evo_Stats* stats;
	evo_Config* config = evo_Config_New();

    if(argc < 3)
    {
        fprintf(stderr, "%s needs a thread count as an argument.\n", argv[1]);
        return -1;
    }
    THREADS = atoi(argv[2]);

    evo_Config_SetUnitCount(config, THREADS);
    evo_Config_SetRandomStreamCount(config, 16);
    evo_Config_SetTrials(config, TRIALS);
    evo_Config_SetMaxIterations(config, MAX_ITERATIONS);
    evo_Config_SetPopulationSize(config, POPULATION);

    evo_Config_SetPopulationInitializer(config, Initializer);
    evo_Config_SetPopulationFinalizer(config, Finalizer);
    evo_Config_SetFitnessOperator(config, PopulationFitness);

    evo_UseTournamentSelection(config, POPULATION, 4);

    evo_Config_SetCrossoverOperator(config, Crossover);
    evo_Config_SetMutationOperator(config, Mutation);
    evo_Config_SetSuccessPredicate(config, Success);

    StartTime();
    evo_Config_Execute(config);
    if(!evo_Config_IsUsed(config))
    {
        fprintf(stderr, "Could not use the given config.\n");
        return 0;
    }

    t = EndTime();

    stats = evo_Config_GetStats(config);
    printf("\n", stats->failures, stats->trials);
    printf("Failures %u/%u\n", stats->failures, stats->trials);
    printf("Best fitness %lf\n", stats->bestFitness, stats->trials);
    printf("%d threads took %lf seconds to generate self-avoiding walk statistics.\n", THREADS, t);

	evo_Config_Free(config);
	return 0;
}