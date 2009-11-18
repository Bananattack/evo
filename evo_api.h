#ifndef EVO_API_H
#define EVO_API_H


/* An wrapper for boolean types, since C's stdbool is only C99. */
#define EVO_TRUE 1
#define EVO_FALSE 0
typedef int evo_bool;
/*
    For *slightly* less writing for each unsigned int.
    Also allows me to swap internal representation easier, even though I'm not likely to.
*/
typedef unsigned int evo_uint;

/* Here we declare the structures but do not define them. */
typedef struct evo_Config evo_Config;
typedef struct evo_Context evo_Context;
typedef struct evo_Stats evo_stats;



/*
    Evolutionary algorithm callbacks.
    
    Gene encoding is abstracted from the library,
    but user-defined evolutionary operators must share the same encoding
    or else the results of the evolutionary algorithm are undefined.
    
    Similarly, the fitness operator and selection operator must to some extent
    agree on definitions of "good" fitness, or results will be undefinied.
*/
/*
    The population initializer.
    
    Called every trial.
    
    Sets up every gene in the population.
    
    The function is expected to setup the context's initial genes.
    If the context's genes array is NULL, then this means a fresh allocation is needed.
    Otherwise, the genes array should be re-randomized/reallocated.
    
    This initializer can also allocate a userdata type for use with the selection operator.
    If it does though, the population finalizer should be tasked with freeing it.
    
    If the initialization succeeds, this function should return true.
    Otherwise, return false to indicate there was a failure (allocation, or whatever).
*/
typedef evo_bool (*evo_PopulationInitializer)(evo_Context* context, evo_uint populationSize);
/*
    The population finalizer.
    
    Called when a thread has finished all of its trials.
    
    
    This function is expected to free the memory associated with every gene in the context.
    
*/
typedef void (*evo_PopulationFinalizer)(evo_Context* context, evo_uint populationSize);

/*
    The fitness operator.
    
    Takes a gene, and returns a numeric value, assessing how good thaat particular gene is.
*/
typedef double (*evo_FitnessOperator)(evo_Context* context, void* gene);
/*
    The selection operator.
    
    Tasked with deciding which genes should breed, and which genes should be replaced.
    
    It should use evo_Context_AddParents and evo_Context_AddChildren to construct the list
    used by crossover and mutation.
*/
typedef void (*evo_SelectionOperator)(evo_Context* context, evo_uint populationSize);
/*
    The crossover operator.
    
    Takes 2 parents (genes chosen to breed), and 2 children (genes chosen to be replaced).
    The crossover operator should perform some kind of copy or combination of the parent genes,
    and overwrite the contents of the children.
 */
typedef void (*evo_CrossoverOperator)(evo_Context* context,
    void* parentA, void* parentB, void* childA, void* childB);
/*
    The mutation operator.
    
    Takes a child gene (created by crossover) and rearranges its contents in some manner.
*/
typedef void (*evo_MutationOperator)(evo_Context* context, void* gene);

/*
    The success predicate.

    Returns true (non-zero) if the evolutionary algorithm was "successful"
    If this returns false (zero), the evolutionary algorithm should continue and is not yet considered successful.
    
    Note that the algorithm will always terminate at the configured maxIterations.
    However, when that occurs, the algorithm marks that particular iteration as a failure.
*/
typedef evo_bool (*evo_SuccessPredicate)(evo_Context* context);
/*
    A callback into user code.
    
    This might be custom allocator/free routines for userdata within the program.
    User callbacks should not rely on call order relative to other user callbacks.
    This means that callbacks should probably agree on what memory can get manipulated by each, exclusively.
*/
typedef void (*evo_UserCallback)(evo_Context* context);



/* Creates a new cnfiguration for the evolutionary algorithm. */
evo_Config* evo_Config_New();
/*
    Frees the data associated with the configuration.

    The configuration must not be running when it is called, or these will fail.
*/
void evo_Config_Free(evo_Config* config);

/* 
    The crucial settings for the evolutionary algorithm.
    
    The configuration must not already be running,
    and must not have been used previously, or these will fail.
*/
/*
    Attributes
    
    Thread count: the number of threads to use.
    Trials per thread: the number of times to run the genetic algorithm across the number of threads.
    Max iterations: The maximum number of iterations to run a trial for, before marking a trial as a failure.
    Population size: the number of genes in the population.
*/
void evo_Config_SetThreadCount(evo_Config* config, evo_uint threadCount);
void evo_Config_SetTrialsPerThread(evo_Config* config, evo_uint trialsPerThread);
void evo_Config_SetMaxIterations(evo_Config* config, evo_uint maxIterations);
void evo_Config_SetPopulationSize(evo_Config* config, evo_uint populationSize);
/*
    Callbacks
    
    See descriptions alongside the typedefs (a while above) for each of these, for more details.
*/
void evo_Config_SetPopulationInitializer(evo_Config config, evo_PopulationInitializer populationInitializer);
void evo_Config_SetPopulationFinalizer(evo_Config config, evo_PopulationFinalizer populationFinalizer);

void evo_Config_SetFitnessOperator(evo_Config* config, evo_FitnessOperator fitnessOperator);
void evo_Config_SetSelectionOperator(evo_Config* config, evo_SelectionOperator selectionOperator);
void evo_Config_SetCrossoverOperator(evo_Config* config, evo_CrossoverOperator crossoverOperator);
void evo_Config_SetMutationOperator(evo_Config* config, evo_MutationOperator mutationOperator);

void evo_Config_SetSuccessPredicate(evo_Config* config, evo_SuccessPredicate terminationPredicate);
/* Optional callbacks. */
void evo_Config_AddContextStartCallback(evo_Config* config, evo_UserCallback cb);
void evo_Config_AddContextEndCallback(evo_Config* config, evo_UserCallback cb);
/* Starts execution across multiple threads. */
void evo_Config_Execute(evo_Config* config);

/* A structure containing overall stats. Can be used to construct other statistics when the program finishes. */
struct evo_Stats
{
    /* Total number of trials */
    evo_uint trails;
    /*
        The number of failed trials.
        Successes are mutually exclusive, so all trials that are not failures are successes
    */
    evo_uint failures;
    /* The sum of the iterations taken, over all trials. */
    double sumIterations;
    /* The sum of all the iterations squared */
    double sumSquaredIterations;
    /* The sum of the iterations taken, over all trials (only counting successes). */
    double sumSuccessIterations;
    /* The sum of all the iterations squared (only counting successes). */
    double sumSquaredSuccessIterations;
    /* The minimum/maximum of all iterations taken */
    double minIteration;
    double maxSuccessIteration;
    double maxIteration;
};

/* A structure containing various information utilized by each thread in the evolutionary algorithm. */
struct evo_Context
{
    /* For internal use. */
    evo_Config* configuration;
    /*
        The thread id, which might be useful if for some reason global storage is required by
        part of the algorithm.
    */
    evo_uint threadID;
  
    
    /* The number of trials that this thread has run of the evolutionary algorithm */
    evo_uint trial;
    /* The number of iterations that this trial of the evolutionary algorithm has run for. */
    evo_uint iteration;
    
    /*
        Overall statistics
        
        These can be used to construct other statistics when the program finishes.
    */
    evo_Stats stats;
    
    /* The fitness of the best in the population. Might be useful for success predicates. */
    double bestFitness;
       
    /*
        An array of genes in the population.
        Encoding is abstracted, but user-defined evolutionary operators
        must share the same encoding or else the results of the algorithm are undefined.
    */
    void** genes;    
    /*
        A fitness value for each gene in the population. 
        Typically, a selection operator will try to maximize the fitness.
        
        However, the particular encoding depends on agreed upon encoding for fitness values
        between the fitness operator and the selection operator.
    */
    double* fitnesses;
    
    /* Selection-related data. After each generation, these are wiped. */
    evo_uint breedEventSize; /* Size consumed by active breeding entries. Always a multiple of 4. */
    /* 
        An array of population indexes.
        Each breeding event takes four items in this array.
        
        For a slice of 4 elements from the array, the following ordering is followed:
        0, 1 - parents
        2, 3 - children
    */
    evo_uint* breedEvents; 
    evo_bool* markedGenes; /* Checklist of which parents/children are already marked for selection. */
    /* Userdata for selection operator. */
    void* selectionUserData;
};

/*
    For ease-of-use with the selection operator.
    AddBreedEvent takes valid indexes within the genes array to be chosen as parents/children.

    If it successfully added the parents/children, then it will return 1.
    However, if any of the genes was already chosen by a previous call, then it will return 0.
*/
evo_bool evo_Context_AddBreedEvent(evo_Context* context,
    evo_uint parentA, evo_uint parentB, evo_uint childA, evo_uint childB);



#endif