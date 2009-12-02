#include <evo_select_tournament.h>
#include "tests.h"

/*#define THREADS 16*/

static int THREADS = 0;
#define TRIALS 240
#define MAX_ITERATIONS 500
#define POPULATION 32
#define MATCHES 100

// Number of states that this machine can hold.
#define STATE_COUNT 4
// Alphabet = { 1 = Cooperate, 0 = Defect } 
#define ALPHABET_DEFECT 0
#define ALPHABET_COOPERATE 1
#define ALPHABET_SIZE 2
// Number of outputs.
#define OUTPUT_COUNT ((ALPHABET_SIZE) * (STATE_COUNT))

typedef struct
{
    // The first state the player uses.
    int initialState;
    // The first response the player uses.
    int initialResponse;
    // The next state for each pair (input, current state).
    int nextStates[OUTPUT_COUNT];
    // Output action in alphabet for each pair (input, current state).
    // Can either Cooperate or Defect for each input given.
    int responses[OUTPUT_COUNT];
} PrisonerPlayer;

static void DumpResults(evo_Context* context, evo_bool last)
{
    FILE* f;
    PrisonerPlayer* gene;
    evo_uint p, i;
    evo_uint populationSize = evo_Context_GetPopulationSize(context);
    char alphabet[] = "DC";
    char name[128];

    if(last)
    {
        sprintf(name, "FSM_%d_%d_%d.txt", context->id, context->prevSeed, context->trial);
    }
    else
    {
        sprintf(name, "FSM_%d_%d_%d.txt", context->id, context->seed, context->trial);
    }
    f = fopen(name, "w");
    fprintf(f, "Trial %d\n", context->trial);
    for(p = 0; p < populationSize; p++)
    {
        fprintf(f, "FSM %d:\n", p);
        gene = context->genes[p];
        fprintf(f, "\tInitial State: %d\n", gene->initialState);
        fprintf(f, "\tInitial Response: %c\n", alphabet[gene->initialResponse]);
        fprintf(f, "\tTransitions:\n");
        for(i = 0; i < OUTPUT_COUNT; i++)
        {
            fprintf(f, "\t\tState: %d\tInput: %c\tNew State: %d\tOutput: %c\n", i % STATE_COUNT, alphabet[i / STATE_COUNT], gene->nextStates[i], alphabet[gene->responses[i]]);
        }
        fprintf(f, "\n\n");
    }
    fclose(f);
}

static evo_bool Initializer(evo_Context* context)
{
    evo_uint i, j;
    evo_uint populationSize;
    PrisonerPlayer* gene;

    populationSize = evo_Context_GetPopulationSize(context);
    if(!context->genes)
    {
        context->genes = malloc(sizeof(void*) * populationSize);
        for(i = 0; i < populationSize; i++)
        {
            gene = malloc(sizeof(PrisonerPlayer));
            context->genes[i] = gene;
        }
    }
    else
    {
        DumpResults(context, 0);
    }
    for(i = 0; i < populationSize; i++)
    {
        gene = context->genes[i];
        gene->initialState = evo_RandomInt(context, 0, STATE_COUNT);
        gene->initialResponse = evo_RandomInt(context, 0, ALPHABET_SIZE);
        for(j = 0; j < OUTPUT_COUNT; j++)
        {
            gene->nextStates[j] = evo_RandomInt(context, 0, STATE_COUNT);
            gene->responses[j] = evo_RandomInt(context, 0, ALPHABET_SIZE);
        }
    }
    return 1;
}

static void Finalizer(evo_Context* context)
{
    evo_uint i;
    evo_uint populationSize;

    DumpResults(context, 1);

    populationSize = evo_Context_GetPopulationSize(context);
    for(i = 0; i < populationSize; i++)
    {
        free(context->genes[i]);
    }
    free(context->genes);

    return;
}

static void RoundRobin(evo_Context* context)
{
    const double payoffMatrix[2][2] = {{ 3.0, 5.0 }, { 0.0, 1.0 }};
    evo_uint populationSize;

    evo_uint stateA, stateB;
    evo_uint responseA, responseB, responseTemp;
    evo_uint i, j, k;

    PrisonerPlayer* a;
    PrisonerPlayer* b;
    double* fitnesses;

    populationSize = evo_Context_GetPopulationSize(context);
    fitnesses = context->fitnesses;
    for(i = 0; i < populationSize - 1; i++)
    {
        for(j = i + 1; j < populationSize; j++)
        {
            a = (PrisonerPlayer*) context->genes[i];
            b = (PrisonerPlayer*) context->genes[j];
            stateA = a->initialState;
            stateB = b->initialState;
            responseA = b->initialResponse;
            responseB = b->initialResponse;
            for(k = 0; k < MATCHES; k++)
            {
                /* Record fitness for both players. */
                fitnesses[i] += payoffMatrix[responseA][responseB];
                fitnesses[j] += payoffMatrix[responseB][responseA];
                /* Feed actions. */
                stateA = a->nextStates[responseB * STATE_COUNT + stateA];
                responseTemp = a->responses[responseB * STATE_COUNT + stateA];

                stateB = b->nextStates[responseA * STATE_COUNT + stateB];
                responseB = b->responses[responseA * STATE_COUNT + stateB];

                responseA = responseTemp;
            }
        }
    }

    /* Normalize the fitnesses */
    for(i = 0; i < populationSize; i++)
    {
        fitnesses[i] /= ((populationSize - 1) * MATCHES);
    }
}

static void Crossover(evo_Context* context,
    void* parentA, void* parentB, void* childA, void* childB)
{
    evo_uint cp1, cp2, i;
    PrisonerPlayer* pa = parentA;
    PrisonerPlayer* pb = parentB;
    PrisonerPlayer* ca = childA;
    PrisonerPlayer* cb = childB;
    
    cp1 = evo_RandomInt(context, 0, OUTPUT_COUNT);
    cp2 = evo_RandomInt(context, 0, OUTPUT_COUNT);
    if(cp1 > cp2)
    {
        i = cp1;
        cp1 = cp2;
        cp2 = i;
    }

    ca->initialState = pa->initialState;
    cb->initialState = pb->initialState;
    ca->initialResponse = pa->initialResponse;
    cb->initialResponse = pb->initialResponse;

    for(i = 0; i < cp1; i++)
    {
        ca->nextStates[i] = pa->nextStates[i];
        cb->nextStates[i] = pb->nextStates[i];
        ca->responses[i] = pa->responses[i];
        cb->responses[i] = pb->responses[i];
    }
    for(i = cp1; i < cp2; i++)
    {
        ca->nextStates[i] = pb->nextStates[i];
        cb->nextStates[i] = pa->nextStates[i];
        ca->responses[i] = pb->responses[i];
        cb->responses[i] = pa->responses[i];
    }
    for(i = cp2; i < OUTPUT_COUNT; i++)
    {
        ca->nextStates[i] = pa->nextStates[i];
        cb->nextStates[i] = pb->nextStates[i];
        ca->responses[i] = pa->responses[i];
        cb->responses[i] = pb->responses[i];
    }
}

static void Mutation(evo_Context* context, void* d)
{
    PrisonerPlayer* gene = d;
    evo_uint i;
    switch(evo_RandomInt(context, 0, 4))
    {
        case 0:
            gene->initialState = evo_RandomInt(context, 0, STATE_COUNT);
            break;
        case 1:
            gene->initialResponse = evo_RandomInt(context, 0, ALPHABET_SIZE);
            break;
        case 2:
            i = evo_RandomInt(context, 0, OUTPUT_COUNT);
            gene->nextStates[i] = evo_RandomInt(context, 0, STATE_COUNT);
            break;
        case 3:
            i = evo_RandomInt(context, 0, OUTPUT_COUNT);
            gene->responses[i] = evo_RandomInt(context, 0, ALPHABET_SIZE);
            break;
    }
}

static evo_bool Success(evo_Context* context)
{
    /* There is no success or "failure" here. We want to evolve strategies to prisoner's dilemma */
    return 0;
}

TEST(prisoner)
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
    evo_Config_SetRandomStreamCount(config, 48);
    evo_Config_SetTrials(config, TRIALS);
    evo_Config_SetMaxIterations(config, MAX_ITERATIONS);
    evo_Config_SetPopulationSize(config, POPULATION);

    evo_Config_SetPopulationInitializer(config, Initializer);
    evo_Config_SetPopulationFinalizer(config, Finalizer);
    evo_Config_SetFitnessOperator(config, RoundRobin);
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
    printf("%d threads took %lf seconds to generate prisoner's dilemma state machine files.\n", THREADS, t);

	evo_Config_Free(config);
	return 0;
}