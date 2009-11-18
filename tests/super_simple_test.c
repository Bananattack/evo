#include <stdio.h>
#include <stdlib.h>
#include <evo_api.h>

#define BOARD_SIZE 3
#define GENE_LEN 9
#define U '0'
#define D '1'
#define L '2'
#define R '3'

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
            gene = (char*) malloc(sizeof(char) * (GENE_LEN + 1));
            for(j = 0; j < GENE_LEN; j++)
            {
                switch(Random(4))
                {
                    case 0: gene[j] = U; break;
                    case 1: gene[j] = D; break;
                    case 2: gene[j] = L; break;
                    case 3: gene[j] = R; break;
                }
            }
            gene[GENE_LEN] = 0;
            context->genes[i] = gene;
        }
    }
    return 1;
}

void Finalizer(evo_Context* context, evo_uint populationSize)
{
    return;
}

double Fitness(evo_Context* context, void* gene)
{
    return 0;
}

void Selection(evo_Context* context, evo_uint populationSize)
{

}

void Crossover(evo_Context* context,
    void* parentA, void* parentB, void* childA, void* childB)
{

}

void Mutation(evo_Context* context, void* gene)
{

}

evo_bool Success(evo_Context* context)
{
    return 0;
}

int main(int argc, char** argv)
{
    evo_uint i;
    evo_Stats* stats;
	evo_Config* config = evo_Config_New();

    evo_Config_SetThreadCount(config, 16);
    evo_Config_SetTrialsPerThread(config, 1000);
    evo_Config_SetMaxIterations(config, 1000);
    evo_Config_SetPopulationSize(config, 1000);

    evo_Config_SetPopulationInitializer(config, Initializer);
    evo_Config_SetPopulationFinalizer(config, Finalizer);
    evo_Config_SetFitnessOperator(config, Fitness);
    evo_Config_SetSelectionOperator(config, Selection);
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
    printf("Failures %u/%u\n", stats->failures, stats->trials);

	evo_Config_Free(config);
	return 0;
}