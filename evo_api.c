/* Standard library (needs math library to be linked). */
#include <stdlib.h>
#include <string.h>
/* pthreads is used for multithreading. */
#include <pthread.h>
/* Library internals. */
#include "evo_api.h"

/* The configuration structure. Intended to be an opaque data-type to the calling code. */
struct evo_Config
{
    /* Internals */
    evo_bool running; /* Whether or not this configuration is already running. */
    evo_bool used; /* Whether or not this configuration has already been utilized. */

    /* Attributes */
	evo_uint threadCount; /* Total number of threads that can run at once. */
	evo_uint trialsPerThread; /* Number of trials of the evolutionary algorithm to run on each thread */
    evo_uint maxIterations; /* The maximum number of iterations to run for, before marking a run as a failure. */
    evo_uint populationSize; /* The size of the population in the evolutionary algorithm. */

    /* Callbacks - see their typedefs in evo_api.h for usage info. */
    evo_PopulationInitializer populationInitializer;
    evo_PopulationFinalizer populationFinalizer;
    evo_FitnessOperator fitnessOperator;
    evo_SelectionOperator selectionOperator;
    evo_CrossoverOperator crossoverOperator;
    evo_MutationOperator mutationOperator;
    evo_SuccessPredicate successPredicate;
    
    /* Statistics. Filled after the algorithms are completely finished. */
    evo_Stats stats;
};

#define RETURN_IF_INVALID(c) \
    do \
    { \
        if(!c || c->running || c->used) \
        { \
            return; \
        } \
    } while(0)


evo_Config* evo_Config_New()
{
    evo_Config* config = calloc(1, sizeof(evo));
    /*
        NOTE: if anything by default needs to be non-zero here for some reason, initialize it here.
        Howeverm, if config were NULL, we'd need to return before touching the data.
        Currently, it doesn't matter.
    */
    return config;
}

void evo_Config_Free(evo_Config* config)
{
    if(!config->running)
    {
        free(config->stats);
        free(config);
    }
}

/* Attributes */
void evo_Config_SetThreadCount(evo_Confg* config, evo_uint threadCount)
{
    RETURN_IF_INVALID(config);
    config->threadCount = threadCount;
}

void evo_Config_SetTrialsPerThread(evo_Config* config, evo_uint trialsPerThread)
{
    RETURN_IF_INVALID(config);
    config->trialsPerThread = trialsPerThread;
}

void evo_Config_SetMaxIterations(evo_Config* config, evo_uint maxIterations)
{
    RETURN_IF_INVALID(config);
    config->maxIterations = maxIterations;
}

void evo_Config_SetPopulationSize(evo_Config* config, evo_uint populationSize)
{
    RETURN_IF_INVALID(config);
    config->populationSize = populationSize;
}



/* Callbacks */
void evo_Config_SetPopulationInitializer(evo_Config* config, evo_PopulationInitializer populationInitializer)
{
    RETURN_IF_INVALID(config);
    config->populationInitializer = populationInitializer;
}

void evo_Config_SetPopulationFinalizer(evo_Config* config, evo_PopulationFinalizer populationFinalizer)
{
    RETURN_IF_INVALID(config);
    config->populationFinalizer = populationFinalizer;
}

void evo_Config_SetFitnessOperator(evo_Config* config, evo_FitnessOperator fitnessOperator)
{
    RETURN_IF_INVALID(config);
    config->fitnessOperator = fitnessOperator;
}

void evo_Config_SetSelectionOperator(evo_Config* config, evo_SelectionOperator selectionOperator)
{
    RETURN_IF_INVALID(config);
    config->selectionOperator = selectionOperator;
}

void evo_Config_SetCrossoverOperator(evo_Config* config, evo_CrossoverOperator crossoverOperator)
{
    RETURN_IF_INVALID(config);
    config->crossoverOperator = crossoverOperator;
}

void evo_Config_SetMutationOperator(evo_Config* config, evo_MutationOperator mutationOperator)
{
    RETURN_IF_INVALID(config);
    config->mutationOperator = mutationOperator;
}

void evo_Config_SetSuccessPredicate(evo_Config* config, evo_SuccessPredicate successPredicate)
{
    RETURN_IF_INVALID(config);
    config->successPredicate = successPredicate;
}



/* Execution. Here be dragons. */
static void* thread_run(void* arg);

void evo_Config_Execute(evo_Config* config)
{
    int a;
    evo_uint i, j;
    pthread_t* threads;
    evo_Context** contexts;
    evo_Stats* stats;
    
    RETURN_IF_INVALID(config);
    if(!config->threadCount
        || !config->trialsPerThread
        || !config->maxIterations
        || !config->populationSize
        || !config->populationInitializer
        || !config->populationFinalizer
        || !config->fitnessOperator
        || !config->selectionOperator
        || !config->crossoverOperator
        || !config->mutationOperator
        || !config->successPredicate)
    {
        return;
    }
    
    config->running = 1;
    config->used = 1;
    

    /* Create the array of threads. */
    threads = malloc(sizeof(pthread_t) * config->threadCount);
    /* Failed to allocate threads array. */
    if(!threads)
    {
        return;
    }
    
    /* Create the array of contexts */
    contexts = malloc(sizeof(evo_Context*) * config->threadCount);
    /* Failed to allocate contexts array. */
    if(!contexts)
    {
        free(threads);
        return;
    }
    
    /* Allocate and initialize each context. */
    for(i = 0; i < config->threadCount; i++)
    {
        /* Create the context. */
        contexts[i] = calloc(1, sizeof(evo_Context));    
        /* Failed to allocate context. */
        if(!contexts[i])
        {
            free(threads);
            /* Free all entries up to the current one. */
            for(j = 0; j < i; j++)
            {
                free(contexts[j]);
            }
            free(contexts);
            return;
        }
        /*
            Populate the bare minimum.
            
            Later: initialize genes, fitness, et cetera, in the particular thread.
            The reason it is not done here is because this already has ugly-enough memory checking.
            
            Also, the initializations can be done in potentially parallel, provided a thread-safe library.
        */
        contexts[i]->config = config;
        contexts[i]->threadID = i;
        contexts[i]->trial = 0;
        contexts[i]->iteration = 0;
    }
    
    /* Start up the threads. */
    for(i = 0; i < config->threadCount; i++)
    {
        /* Create the thread. */
        a = pthread_create(&threads[i], NULL, thread_run, contexts[i]);
        /* Failed to create thread. */
        if(!a)
        {
            free(threads);
            for(j = 0; j < config->threadCount; j++)
            {
                free(contexts[j]);
            }
            free(contexts);
            return;
        }
    }
    
    /* Join the threads together when they're done. */
    for(i = 0; i < config->threadCount; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
    /* Aggregate all statistics. */
    stats = calloc(1, sizeof(evo_Stats));
    for(i = 0; i < config->threadCount; i++)
    {
        stats.trails += contexts[i]->stats.trails;
        stats.failures += contexts[i]->stats.failures;
        stats.sumIterations += contexts[i]->stats.sumIterations;
        stats.sumSquaredIterations += contexts[i]->stats.sumSquaredIterations;
        stats.sumSuccessIterations += contexts[i]->stats.sumSuccessIterations;
        stats.sumSquaredSuccessIterations += contexts[i]->stats.sumSquaredSuccessIterations;

        stats.minIteration = (i == 0 || contexts[i]->stats.minIteration < stats.minIteration) ? contexts[i]->stats.minIteration : stats.minIteration;
        stats.maxSuccessIteration = (contexts[i]->stats.maxSuccessIteration > stats.maxSuccessIteration) ? contexts[i]->stats.maxSuccessIteration : stats.maxSuccessIteration;
        stats.maxIteration = (contexts[i]->stats.maxIteration > stats.minIteration) ? contexts[i]->stats.maxIteration : stats.maxIteration;
    }
    
    /* Configuration is no longer running. Rejoice! */
    config->running = 0;
    config->stats = stats;

    /* Free EVERYTHING. */
    free(threads);
    for(j = 0; j < config->threadCount; j++)
    {
        free(contexts[j]);
    }
    free(contexts);
}

static void* thread_run(void* arg)
{
    evo_bool success;
    evo_Context* context;
    evo_uint i;
    evo_uint threadID, trialsPerThread, maxIterations, populationSize;
    
    context = (evo_Context*) arg;
    threadID = context->threadID;
    trialsPerThread = context->config->trialsPerThread;
    maxIterations = context->config->maxIterations;
    populationSize = context->config->populationSize;

    /* Create the neccessary arrays */
    context->fitnesses = malloc(populationSize * sizeof(double));
    context->parentPopIndexes = malloc(populationSize * sizeof(evo_uint));
    context->childPopIndexes = malloc(populationSize * sizeof(evo_uint));
    context->markedGenes = malloc(populationSize * size(evo_bool));

    /* Keep going until every iteration has completed. */
    for(context->trial = 0; context->trial < trialsPerThread; context->trial++)
    {
        /* Initialize/rerandomize the population. */
        context->config->populationInitializer(context, populationSize);
        
        /* Evaluate all population members' initial fitness. */
        context->bestFitness = 0;
        for(i = 0; i < populationSize; i++)
        {
            context->fitnesses[i] = context->config->fitnessOperator(context, context->genes[i]);
            if(context->fitnesses[i] > context->bestFitness)
            {
                context->bestFitness = context->fitnesses[i];
            }
        }
        
        success = 0;
        
        /* Do the main genetic algorithm. */
        for(context->iteration = 0; context->iteration < maxIterations; context->iteration++)
        {
            /* Clear things for the selection operator. */
            context->breedEventSize = 0;
            memset(context->markedGenes, 0, populationSize);
            /* Perform user-defined selection */
            context->config->selectionOperator(config, populationSize);
            
            /* Use the parent and child lists to reproduce. */
            for(i = 0; i < context->breedEventSize; i += 4)
            {
                /* Perform crossover. */
                context->config->crossoverOperator(config,
                    context->gene[context->breedEvent[i]], context->gene[context->breedEvent[i + 1]], 
                    context->gene[context->breedEvent[i + 2]], context->gene[context->breedEvent[i + 3]]);
                    
                /* Mutate the children. */
                context->config->mutationOperator(config, context->gene[context->breedEvent[i + 2]]);
                context->config->mutationOperator(config, context->gene[context->breedEvent[i + 3]]);
                
                /* Update the fitness of child A */
                j = context->breedEvent[i + 2];
                context->fitnesses[j] = context->config->fitnessOperator(config, context->gene[j]);
                if(context->fitnesses[j] > context->bestFitness)
                {
                    context->bestFitness = context->fitnesses[j];
                }
                
                /* Update the fitness of child B */
                j = context->breedEvent[i + 3];
                context->fitnesses[j] = context->config->fitnessOperator(config, context->gene[j]);
                if(context->fitnesses[j] > context->bestFitness)
                {
                    context->bestFitness = context->fitness[j];
                }
            }
            
            /* Algorithm was successful, stop early. */
            if(context->config->successPredicate(config))
            {
                success = 1;
                break;
            }
            /* Otherwise, go onto another iteration. */
        }
        
        /* Successful! Update specific success-only stats. */
        if(success)
        {
            context->stats.maxSuccessIteration = (context->stats.maxSuccessIteration < context->iteration)
                        ? context->iteration : context->stats.maxSuccessIteration;
            context->stats.sumSuccessIterations += context->iteration;
            context->stats.sumSquaredSuccessIterations += context->iteration * context->iteration;
        }
        /* If the algorithm was not successful, record it. */
        else
        {
            context->stats.failures++;
        }
        /* Update context stats. */
        context->stats.minIteration = (trial == 0 || context->iteration < context->stats.minIteration) ? context->iteration : context->stats.minIteration;     
        context->stats.maxIteration = (context->stats.maxIteration < context->iteration) ? context->iteration : context->stats.maxIteration;
        
        context->stats.sumIterations += context->iteration;
        context->stats.sumSquaredIterations += context->iteration * context->iteration;
        
    }
    /* Calculate the number of trials. */
    context->stats.trials += context->trial;
    /* Free the population */
    context->config->populationFinalizer(context, populationSize);
    
    /* Ding ding ding ding. */
    return NULL;
}

evo_bool evo_Context_AddBreedEvent(evo_Context* context, evo_uint parentA, evo_uint parentB, evo_uint childA, evo_uint childB)
{
    /* Already marked for breeding, fail. */
    if(context->markedGenes[parentA] || context->markedGenes[parentB]
        || context->markedGenes[childA] || context->markedGenes[childB])
    {
        return EVO_FALSE;
    }
    /* Add some new breeding events. */
    else
    {
        context->breedEvents[context->breedEventSize++] = parentA;
        context->breedEvents[context->breedEventSize++] = parentB;
        context->breedEvents[context->breedEventSize++] = childA;
        context->breedEvents[context->breedEventSize++] = childB;
        
        context->markedGenes[parentA] = 1;
        context->markedGenes[parentB] = 1;
        context->markedGenes[childA] = 1;
        context->markedGenes[childB] = 1;
    }
}