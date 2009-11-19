#include "tests.h"

#define BOARD_WIDTH 4
#define BOARD_HEIGHT 6
#define GENE_SIZE 42

int Random(int size)
{
    return ((double) rand()) / ((double) RAND_MAX + 1.0) * size;
}

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
            switch(Random(4))
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



#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct
{
    evo_uint populationSize;
    evo_uint tournamentSize;
} TournamentSelectData;

typedef struct
{
    TournamentSelectData* data;
    evo_uint* rank;
} TournamentSelectContext;

void TournamentSelectBefore(evo_Context* context, void* param)
{
    TournamentSelectContext* tsc;

    tsc = malloc(sizeof(TournamentSelectContext));
    tsc->data = param;
    tsc->rank = malloc(sizeof(evo_uint) * tsc->data->populationSize);

    context->selectionUserData = tsc;
}

void TournamentSelectAfter(evo_Context* context, void* param)
{
    TournamentSelectContext* tsc = context->selectionUserData;

    free(tsc->rank);
    free(tsc);
}

void TournamentSelectPerform(evo_Context* context, evo_uint populationSize)
{
	int i, j, k, t;
	int minimum;
	int size;
    evo_uint tournamentSize;
    evo_uint* rank;
    double* fitnesses;
    TournamentSelectContext* tsc;

    tsc = context->selectionUserData;
    tournamentSize = tsc->data->tournamentSize;
    rank = tsc->rank;
    fitnesses = context->fitnesses;

	// Fill array.
	for(i = 0; i < populationSize; i++)
	{
		rank[i] = i;
	}
	// Shuffle array.
	for(i = 0; i < populationSize; i++)
	{
		j = Random(populationSize);
		t = rank[i];
		rank[i] = rank[j];
		rank[j] = t;
	}
	// Iterate over each of the tournaments.
	for(i = 0; i < populationSize; i += tournamentSize)
	{
		// Sort the tournaments by fitness.
		// Uses selection sort because I'm lazy.
		size = MIN(i + tournamentSize, populationSize);
		for(j = i; j < size - 1; j++)
		{
			minimum = j;
			for(k = j + 1; k < size; k++)
			{
				if(fitnesses[rank[k]] < fitnesses[rank[minimum]])
				{
					minimum = k;
				}
			}
			if(j != minimum)
			{
				t = rank[j];
				rank[j] = rank[minimum];
				rank[minimum] = t;
			}
		}
        evo_Context_AddBreedEvent(context,
            rank[i + tournamentSize - 1],
            rank[i + tournamentSize - 2],
            rank[i],
            rank[i + 1]
        ); 
	}
}

void TournamentSelectFinalize(void* data)
{
    free(data);
}

void TournamentSelectInit(evo_Config* config, evo_uint populationSize,  evo_uint tournamentSize)
{
    TournamentSelectData* data;
    data = malloc(sizeof(TournamentSelectData));

    data->populationSize = populationSize;
    data->tournamentSize = tournamentSize;

    evo_Config_AddContextStartCallback(config, TournamentSelectBefore, data);
    evo_Config_SetSelectionOperator(config, TournamentSelectPerform);
    evo_Config_AddContextEndCallback(config, TournamentSelectAfter, data);
    evo_Config_AddConfigFinalizer(config, TournamentSelectFinalize, data);
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
    
    cp1 = Random(GENE_SIZE);
    cp2 = Random(GENE_SIZE);
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
    evo_uint count = Random(5) + 1;
    while(count)
    {
        count--;
        i = Random(GENE_SIZE);
        switch(Random(4))
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

    evo_Config_SetThreadCount(config, 16);
    evo_Config_SetTrialsPerThread(config, 100);
    evo_Config_SetMaxIterations(config, 1000);
    evo_Config_SetPopulationSize(config, 1000);

    evo_Config_SetPopulationInitializer(config, Initializer);
    evo_Config_SetPopulationFinalizer(config, Finalizer);
    evo_Config_SetFitnessOperator(config, Fitness);
    TournamentSelectInit(config, 1000, 4);
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