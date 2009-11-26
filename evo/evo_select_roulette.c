#include <stdio.h>
#include <stdlib.h>
#include "evo_select_roulette.h"

static void ContextStart(evo_Context* context, void* data);
static void ContextEnd(evo_Context* context, void* data);
static void Selection(evo_Context* context, evo_uint populationSize);

void evo_UseRouletteSelection(evo_Config* config, evo_uint populationSize)
{
    evo_uint* size = malloc(sizeof(evo_uint));
    *size = populationSize;

    evo_Config_AddContextStartCallback(config, ContextStart, size);
    evo_Config_SetSelectionOperator(config, Selection);
    evo_Config_AddContextEndCallback(config, ContextEnd, size);
    evo_Config_AddConfigFinalizer(config, free, size);
}

static void ContextStart(evo_Context* context, void* data)
{
    evo_uint* size = (evo_uint*) data;
    context->selectionUserData = calloc(*size, sizeof(double));
}

static void ContextEnd(evo_Context* context, void* data)
{
    free(context->selectionUserData);
}

static void Selection(evo_Context* context, evo_uint populationSize)
{
    evo_uint i, j, k, lastUnmarked;
    evo_uint parents[2], children[2];
    double* probabilities;
    double p = 0;
    double sum = 0;
    double sumProbabilities = 0;
    
    probabilities = (double*) context->selectionUserData;

    for(i = 0; i < populationSize; i++)
    {
        sum += context->fitnesses[i];
    }

    for(i = 0; i < populationSize; i++)
    {
        probabilities[i] = sumProbabilities + (context->fitnesses[i] / sum);
        sumProbabilities += probabilities[i];
    }

    for(k = 0; k < populationSize / 4; k++)
    {
        /* Pick parents, using probability. */
        for(j = 0; j < 2; j++)
        {
            lastUnmarked = populationSize;
            p = evo_Random(context);
            parents[j] = populationSize;
            for(i = 0; i < populationSize - 1; i++)
            {                
                if(!context->markedGenes[i] && parents[0] != i)
                {
                    lastUnmarked = i;
                    if(p > probabilities[i] && p < probabilities[i + 1])
                    {
                        parents[j] = i;
                    }
                }
            }
            if(parents[j] == populationSize)
            {
                if(!context->markedGenes[i] && parents[0] != i)
                {
                    parents[j] = i;    
                }
                else
                {
                    parents[j] = lastUnmarked;
                }
            }
        }
        /* Pick children, using inverse probability. */
        for(j = 0; j < 2; j++)
        {
            lastUnmarked = populationSize;
            p = evo_Random(context);
            children[j] = populationSize;
            for(i = populationSize - 1; i > 0; i--)
            {
                if(!context->markedGenes[i] && children[0] != i && parents[0] != i && parents[1] != i)
                {
                    lastUnmarked = i;
                    if(p > 1 - probabilities[i] && p < 1 - probabilities[i - 1])
                    {
                        children[j] = i;
                    }
                }
            }
            if(children[j] == populationSize)
            {
                if(!context->markedGenes[0] && children[0] !=i && parents[0] != i && parents[1] != i)
                {
                    children[j] = i;
                }
                else
                {
                    children[j] = lastUnmarked;
                }
            }
        }

        evo_Context_AddBreedEvent(context, parents[0], parents[1], children[0], children[1]);
        /*if(context->trial == 0 && context->iteration == 999 && context->id == 0)
        {
            printf("%d: %d %d -> %d %d\n\n", context->iteration, parents[0], parents[1], children[0], children[1]);
            for(i = 0; i < populationSize; i++)
            {
                if(i % 8 == 0)
                {
                    printf("\n");
                }
                printf("%d ",  context->markedGenes[i]);
            }
            printf("\n");
        }*/
    }
}