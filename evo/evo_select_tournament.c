#include <stdlib.h>
#include "evo_select_tournament.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct
{
    evo_uint populationSize;
    evo_uint tournamentSize;
} TournamentSelectionConfig;

typedef struct
{
    TournamentSelectionConfig* selectionConfig;
    evo_uint* rank;
} TournamentSelectionContext;

static void ContextStart(evo_Context* context, void* data);
static void ContextEnd(evo_Context* context, void* data);
static void Selection(evo_Context* context);

void evo_UseTournamentSelection(evo_Config* config, evo_uint populationSize,  evo_uint tournamentSize)
{
    TournamentSelectionConfig* selectionConfig;
    selectionConfig = malloc(sizeof(TournamentSelectionConfig));

    selectionConfig->populationSize = populationSize;
    selectionConfig->tournamentSize = tournamentSize;

    evo_Config_AddContextStartCallback(config, ContextStart, selectionConfig);
    evo_Config_SetSelectionOperator(config, Selection);
    evo_Config_AddContextEndCallback(config, ContextEnd, selectionConfig);
    evo_Config_AddConfigFinalizer(config, free, selectionConfig);
}

static void ContextStart(evo_Context* context, void* selectionConfig)
{
    TournamentSelectionContext* selectionContext;

    selectionContext = malloc(sizeof(TournamentSelectionContext));
    selectionContext->selectionConfig = selectionConfig;
    selectionContext->rank = malloc(sizeof(evo_uint) * selectionContext->selectionConfig->populationSize);

    context->selectionUserData = selectionContext;
}

static void ContextEnd(evo_Context* context, void* selectionConfig)
{
    TournamentSelectionContext* selectionContext = context->selectionUserData;

    free(selectionContext->rank);
    free(selectionContext);
}

static void Selection(evo_Context* context)
{
	int i, j, k, t;
	int minimum;
	int size;
    evo_uint tournamentSize;
    evo_uint* rank;
    double* fitnesses;
    TournamentSelectionContext* selectionContext;
    evo_uint populationSize = evo_Context_GetPopulationSize(context);

    selectionContext = context->selectionUserData;
    tournamentSize = selectionContext->selectionConfig->tournamentSize;
    rank = selectionContext->rank;
    fitnesses = context->fitnesses;

	// Fill array.
	for(i = 0; i < populationSize; i++)
	{
		rank[i] = i;
	}
	// Shuffle array.
	for(i = 0; i < populationSize; i++)
	{
		j = evo_RandomInt(context, 0, populationSize);
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