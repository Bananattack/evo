/* Standard library (needs math library to be linked). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* pthreads is used for multithreading. */
#include <pthread.h>
/* Library internals. */
#include "evo_api.h"

#define MAX_CALLBACKS 8

static void* _evo_RunThread(void* arg);

typedef struct
{
    evo_uint count;
    evo_UserCallback cb[MAX_CALLBACKS];
    void* param[MAX_CALLBACKS];
} UserCallbackInfo;

typedef struct
{
    evo_uint count;
    evo_UserFinalizer cb[MAX_CALLBACKS];
    void* param[MAX_CALLBACKS];
} UserFinalizerInfo;

/* The configuration structure. Intended to be an opaque data-type to the calling code.  */
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
    
    /* Different user function hooks assigned to this configuration. */
    UserCallbackInfo contextStart;
    UserCallbackInfo contextEnd;
    UserFinalizerInfo configFinalizer;
};

#define RETURN_IF_INVALID(c) \
    do \
    { \
        if(!c || c->running || c->used) \
        { \
            return; \
        } \
    } while(0)
    
#define EVO_ATTR_SETTER(func, attr, type) \
    void func(evo_Config* config, type value) \
    { \
        RETURN_IF_INVALID(config); \
        config->attr = value; \
    }


evo_Config* evo_Config_New()
{
    evo_Config* config = calloc(1, sizeof(evo_Config));
    return config;
}

void evo_Config_Free(evo_Config* config)
{
    evo_uint i;
    if(!config->running)
    {
        /* Invoke all user config finalizer callbacks */
        for(i = 0; i < config->configFinalizer.count; i++)
        {
            config->configFinalizer.cb[i](config->configFinalizer.param[i]);
        }
        /* Free the rest. */
        free(config);
    }
}

evo_bool evo_Config_IsUsed(evo_Config* config)
{
    return config->used;
}

evo_Stats* evo_Config_GetStats(evo_Config* config)
{
    return (config->running) ? ((evo_Stats*) NULL) : &config->stats;
}

/* Attributes. */
EVO_ATTR_SETTER(evo_Config_SetThreadCount, threadCount, evo_uint)
EVO_ATTR_SETTER(evo_Config_SetTrialsPerThread, trialsPerThread, evo_uint)
EVO_ATTR_SETTER(evo_Config_SetMaxIterations, maxIterations, evo_uint)
EVO_ATTR_SETTER(evo_Config_SetPopulationSize, populationSize, evo_uint)

/* Callbacks. */
EVO_ATTR_SETTER(evo_Config_SetPopulationInitializer, populationInitializer, evo_PopulationInitializer)
EVO_ATTR_SETTER(evo_Config_SetPopulationFinalizer, populationFinalizer, evo_PopulationFinalizer)
EVO_ATTR_SETTER(evo_Config_SetFitnessOperator, fitnessOperator, evo_FitnessOperator)
EVO_ATTR_SETTER(evo_Config_SetSelectionOperator, selectionOperator, evo_SelectionOperator)
EVO_ATTR_SETTER(evo_Config_SetCrossoverOperator, crossoverOperator, evo_CrossoverOperator)
EVO_ATTR_SETTER(evo_Config_SetMutationOperator, mutationOperator, evo_MutationOperator)
EVO_ATTR_SETTER(evo_Config_SetSuccessPredicate, successPredicate, evo_SuccessPredicate)

/* Optional callbacks. */
#define EVO_CALLBACK_ADDER(func, attr, cbType) \
    void func(evo_Config* config, cbType cb, void* param) \
    { \
        RETURN_IF_INVALID(config); \
        if(attr.count >= MAX_CALLBACKS) \
        { \
            return; \
        } \
        attr.cb[attr.count] = cb; \
        attr.param[attr.count] = param; \
        attr.count++; \
    }

EVO_CALLBACK_ADDER(evo_Config_AddContextStartCallback, config->contextStart, evo_UserCallback)
EVO_CALLBACK_ADDER(evo_Config_AddContextEndCallback, config->contextEnd, evo_UserCallback)
EVO_CALLBACK_ADDER(evo_Config_AddConfigFinalizer, config->configFinalizer, evo_UserFinalizer)

/* Aggregates statistics after all trials are finished. */
static void _evo_Config_PopulateStats(evo_Config* config, evo_Context** contexts)
{
	evo_uint i;
    evo_Stats* stats;
    evo_Stats* overall;

    overall = &config->stats;
    memset(overall, 0, sizeof(evo_Stats));

    for(i = 0; i < config->threadCount; i++)
    {
        stats = &contexts[i]->stats;
    
        overall->trials += stats->trials;
        overall->failures += stats->failures;
        overall->sumIterations += stats->sumIterations;
        overall->sumSquaredIterations += stats->sumSquaredIterations;
        overall->sumSuccessIterations += stats->sumSuccessIterations;
        overall->sumSquaredSuccessIterations += stats->sumSquaredSuccessIterations;
 
        if(stats->minIteration < overall->minIteration || i == 0)
        {
            overall->minIteration = stats->minIteration;
        }
        if(stats->maxSuccessIteration > overall->maxSuccessIteration)
        {
            overall->maxSuccessIteration = stats->maxSuccessIteration;
        }
        if(stats->maxIteration > overall->maxIteration)
        {
            overall->maxIteration = stats->maxIteration;
        }
        if(stats->bestFitness > overall->bestFitness)
        {
            overall->bestFitness = stats->bestFitness;
        }
    }
}

/*
    Starts evolutionary algorithm execution across multiple threads.
    
    Requires the following to be set:
    * thread count
    * trials per thread
    * population size
    * max iterations
    
    * population initializer
    * population finializer
    
    * fitness operator
    * selection operator
    * crossover operator
    * mutation operator
    * success predicate
    
    All required configuration details must be filled, or this will fail.
    The configuration must not already be running, and must not have been used previously,
    or this will fail.
    
    On failure, the program will exit with an error message.
*/
void evo_Config_Execute(evo_Config* config)
{
    int a;
    evo_uint i, j;
    pthread_t* threads;
    evo_Context** contexts;
    
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
    
    /* Create the array of contexts */
    contexts = malloc(sizeof(evo_Context*) * config->threadCount);
    
    /* Allocate and initialize each context. */
    for(i = 0; i < config->threadCount; i++)
    {
        /* Create the context. */
        contexts[i] = calloc(1, sizeof(evo_Context));
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
        a = pthread_create(&threads[i], NULL, _evo_RunThread, contexts[i]);
    }
    
    /* Join the threads together when they're done. */
    for(i = 0; i < config->threadCount; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
    /* Aggregate all statistics. */
    _evo_Config_PopulateStats(config, contexts);

    printf("DING DING DING DING\n");
    
    /* Configuration is no longer running. Rejoice! */
    config->running = 0;

    /* Free EVERYTHING. */
    free(threads);
    for(j = 0; j < config->threadCount; j++)
    {
        free(contexts[j]);
    }
    free(contexts);
}

static void* _evo_RunThread(void* arg)
{
    evo_bool success;
    evo_Context* context;
    evo_Config* config;
    evo_uint i, j;
    evo_uint threadID, trialsPerThread, maxIterations, populationSize;
    
    context = (evo_Context*) arg;
    config = context->config;
    threadID = context->threadID;
    trialsPerThread = config->trialsPerThread;
    maxIterations = config->maxIterations;
    populationSize = config->populationSize;

    /* Create the neccessary arrays */
    context->fitnesses = malloc(populationSize * sizeof(double));
    context->breedEvents = malloc(populationSize * sizeof(evo_uint));
    context->markedGenes = malloc(populationSize * sizeof(evo_bool));
    
    /* Invoke all user start-of-run callbacks */
    for(i = 0; i < config->contextStart.count; i++)
    {
        config->contextStart.cb[i](context, config->contextStart.param[i]);
    }

    /* Keep going until every iteration has completed. */
    for(context->trial = 0; context->trial < trialsPerThread; context->trial++)
    {
        /* Initialize/rerandomize the population. */
        config->populationInitializer(context, populationSize);
        
        /* Evaluate all population members' initial fitness. */
        context->bestFitness = 0;
        for(i = 0; i < populationSize; i++)
        {
            context->fitnesses[i] = config->fitnessOperator(context, context->genes[i]);
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
            config->selectionOperator(context, populationSize);
            
            /* Use the parent and child lists to reproduce. */
            for(i = 0; i < context->breedEventSize; i += 4)
            {
                /* Perform crossover. */
                config->crossoverOperator(context,
                    context->genes[context->breedEvents[i]], context->genes[context->breedEvents[i + 1]], 
                    context->genes[context->breedEvents[i + 2]], context->genes[context->breedEvents[i + 3]]);
                    
                /* Mutate the children. */
                config->mutationOperator(context, context->genes[context->breedEvents[i + 2]]);
                config->mutationOperator(context, context->genes[context->breedEvents[i + 3]]);
                
                /* Update the fitness of child A */
                j = context->breedEvents[i + 2];
                context->fitnesses[j] = config->fitnessOperator(context, context->genes[j]);
                if(context->fitnesses[j] > context->bestFitness)
                {
                    context->bestFitness = context->fitnesses[j];
                }
                
                /* Update the fitness of child B */
                j = context->breedEvents[i + 3];
                context->fitnesses[j] = config->fitnessOperator(context, context->genes[j]);
                if(context->fitnesses[j] > context->bestFitness)
                {
                    context->bestFitness = context->fitnesses[j];
                }
            }
            
            /* Algorithm was successful, stop early. */
            if(config->successPredicate(context))
            {
                success = 1;
                break;
            }
            /* Otherwise, go onto another iteration. */
        }
        
        /* Successful! Update specific success-only stats. */
        if(success)
        {
            if(context->iteration > context->stats.maxSuccessIteration)
            {
                context->stats.maxSuccessIteration = context->iteration;
            }
            context->stats.sumSuccessIterations += context->iteration;
            context->stats.sumSquaredSuccessIterations += context->iteration * context->iteration;
        }
        /* If the algorithm was not successful, record it. */
        else
        {
            context->stats.failures++;
        }
        /* Update context stats. */
        if(context->trial == 0 || context->iteration < context->stats.minIteration)
        {
            context->stats.minIteration = context->iteration;
        }
        if(context->iteration > context->stats.maxIteration)
        {
            context->stats.maxIteration = context->iteration;
        }
        if(context->bestFitness > context->stats.bestFitness)
        {
            context->stats.bestFitness = context->bestFitness;
        }
        
        context->stats.sumIterations += context->iteration;
        context->stats.sumSquaredIterations += context->iteration * context->iteration;
        
    }
    /* Calculate the number of trials. */
    context->stats.trials += context->trial;
    
    /* Invoke all user end-of-run callbacks */
    for(i = 0; i < config->contextEnd.count; i++)
    {
        config->contextEnd.cb[i](context, config->contextEnd.param[i]);
    }
    
    /* Free the population */
    context->config->populationFinalizer(context, populationSize);
    
    /* Ding ding ding ding. */
    return NULL;
}

evo_bool evo_Context_AddBreedEvent(evo_Context* context, evo_uint pa, evo_uint pb, evo_uint ca, evo_uint cb)
{
    /* Already marked for breeding, fail. */
    if(context->markedGenes[pa] || context->markedGenes[pb]
        || context->markedGenes[ca] || context->markedGenes[cb])
    {
        return EVO_FALSE;
    }
    /* Add some new breeding events. */
    else
    {
        context->breedEvents[context->breedEventSize++] = pa;
        context->breedEvents[context->breedEventSize++] = pb;
        context->breedEvents[context->breedEventSize++] = ca;
        context->breedEvents[context->breedEventSize++] = cb;
        
        context->markedGenes[pa] = 1;
        context->markedGenes[pb] = 1;
        context->markedGenes[ca] = 1;
        context->markedGenes[cb] = 1;

        return EVO_TRUE;
    }
}