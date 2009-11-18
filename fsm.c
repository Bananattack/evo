#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "evo_api.h"

#define THREADS 16
#define TRIALS_PER_THREAD 1000
#define MAX_ITERATIONS 1000
#deinfe POPULATION_SIZE 1000

#define NUM_STATES 4
#define NUM_TRANSITIONS ((NUM_STATES) * 2)
#define FINAL_STATE_MAX (1 << (NUM_STATES))

typedef struct 
{
	int transitions[NUM_TRANSITIONS];
	int finalState;
    
    evo_uint rank;
} StateMachine;

#define NUM_TESTS 127
char* tests[NUM_TESTS] = {
	"",
	"0",
	"1",
	"00",
	"01",
	"10",
	"11",
	"000",
	"001",
	"010",
	"011",
	"100",
	"101",
	"110",
	"111",
	"0000",
	"0001",
	"0010",
	"0011",
	"0100",
	"0101",
	"0110",
	"0111",
	"1000",
	"1001",
	"1010",
	"1011",
	"1100",
	"1101",
	"1110",
	"1111",
	"00000",
	"00001",
	"00010",
	"00011",
	"00100",
	"00101",
	"00110",
	"00111",
	"01000",
	"01001",
	"01010",
	"01011",
	"01100",
	"01101",
	"01110",
	"01111",
	"10000",
	"10001",
	"10010",
	"10011",
	"10100",
	"10101",
	"10110",
	"10111",
	"11000",
	"11001",
	"11010",
	"11011",
	"11100",
	"11101",
	"11110",
	"11111",
	"000000",
	"000001",
	"000010",
	"000011",
	"000100",
	"000101",
	"000110",
	"000111",
	"001000",
	"001001",
	"001010",
	"001011",
	"001100",
	"001101",
	"001110",
	"001111",
	"010000",
	"010001",
	"010010",
	"010011",
	"010100",
	"010101",
	"010110",
	"010111",
	"011000",
	"011001",
	"011010",
	"011011",
	"011100",
	"011101",
	"011110",
	"011111",
	"100000",
	"100001",
	"100010",
	"100011",
	"100100",
	"100101",
	"100110",
	"100111",
	"101000",
	"101001",
	"101010",
	"101011",
	"101100",
	"101101",
	"101110",
	"101111",
	"110000",
	"110001",
	"110010",
	"110011",
	"110100",
	"110101",
	"110110",
	"110111",
	"111000",
	"111001",
	"111010",
	"111011",
	"111100",
	"111101",
	"111110",
	"111111"
};

void RandomizeStateMachineParts(StateMachine* machine)
{
	evo_uint i;
	for(i = 0; i < NUM_TRANSITIONS; i++)
	{
		machine->transitions[i] = Random(seed, 0, NUM_STATES);
	}
	machine->finalState = Random(seed, 0, FINAL_STATE_MAX);
}

evo_bool Initialize(evo_Context* context, evo_uint populationSize)
{
    evo_uint i;
    if(context->genes == NULL)
    {
        context->genes = malloc(sizeof(void*) * populationSize);
        for(i = 0; i < populationSize; i++)
        {
            context->genes[i] = malloc(sizeof(StateMachine));
        }
        context->selectionUserData = malloc(sizeof(evo_uint) * populationSize);
    }
    for(i = 0; i < populationSize; i++)
    {
        RandomizeStateMachineParts((StateMachine*) (context->genes[i]));
    }
}

void Finalize(evo_Context* context, evo_uint populationSize)
{
    evo_uint i;
    for(i = 0; i < populationSize; i++)
    {
        free(context->genes[i]);
    }
    free(context->genes);
    free(context->selectionUserData);
}

/* Returns a 1 when the state machine successfully categorized the data provided. */
int TestStateMachine(char* test, StateMachine* machine)
{
    evo_bool accept;
	evo_uint i;
    evo_uint count[2] = {0, 0};
	int state = 0; // Start at state 0
	int len = strlen(test);
    
	for(i = 0; i < len; i++)
	{
		state = machine->transitions[state * 2 + (test[i] - '0')];
        count[patterns[i][j] - '0']++;
	}
    
    accept = ((count[0] & 1) == 0) && ((count[1] & 1) == 0);
	if((machine->finalState & (1 << state)))
	{
		return accept;
	}
	return !accept;
}

double Fitness(evo_Context* context, void* gene)
{
    evo_uint i;
    StateMachine* machine = (StateMachine*) gene;
    double v = 0.0;
    for(i = 0; i < NUM_TESTS; i++)
    {
        if(TestStateMachine(tests[i], machine))
        {
            v++;
        }
    }
    return v;
}

void Selection(evo_Context* context, evo_uint populationSize)
{
    evo_uint* rank = (evo_uint*) context->selectionUserData;
    double* fitness = context->fitness;
	int i, j, k, t;
	int minimum;
	int size;
    
	/* Fill array. */
	for(i = 0; i < populationSize; i++)
	{
		rank[i] = i;
	}
	/* Shuffle array. */
	for(i = 0; i < populationSize; i++)
	{
		j = Random(seed, 0, populationSize);
		t = rank[i];
		rank[i] = rank[j];
		rank[j] = t;
	}
	/* Iterate over each of the tournaments. */
	for(i = 0; i < populationSize; i += TOURNAMENT_SIZE)
	{
		/* Sort the tournaments by fitness. Uses selection sort. */
		size = MIN(i + TOURNAMENT_SIZE, populationSize);
		for(j = i; j < size - 1; j++)
		{
			minimum = j;
			for(k = j + 1; k < size; k++)
			{
				if(fitness[rank[k]] < fitness[rank[minimum]])
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
        /*
            Push the tournament results into a breeding event,
            making the last two breed over the first two items in the tournament.
        */
        evo_Context_AddBreedEvent(context,
            rank[i + TOURNAMENT_SIZE - 1], rank[i + TOURNAMENT_SIZE - 2],
            rank[i], rank[i + 1]
        );
	}
}


void Crossover(evo_Context* context, void* parentA, void* parentB, void* childA, void* childB)
{
	int i;
	int cp, cp2;

    StateMachine* A = parentA;
    StateMachine* B = parentB;
    StateMachine* C = childA;
    StateMachine* D = childB;

	cp = Random(seed, 0, NUM_STATES);
	cp2 = Random(seed, 0, NUM_STATES);
	if(cp > cp2)
	{
		i = cp; cp = cp2; cp2 = i;
	}

	C->finalState = 0;
	D->finalState = 0;
	for(i = 0; i < cp; i++)
	{
		C->finalState |= (A->finalState & (1 << i));
		D->finalState |= (B->finalState & (1 << i));
	}
	for(i = cp; i < cp2; i++)
	{
		C->finalState |= (B->finalState & (1 << i));
		D->finalState |= (A->finalState & (1 << i));
	}	
	for(i = cp2; i < NUM_STATES; i++)
	{
		C->finalState |= (A->finalState & (1 << i));
		D->finalState |= (B->finalState & (1 << i));
	}

	cp = Random(seed, 0, NUM_TRANSITIONS);
	cp2 = Random(seed, 0, NUM_TRANSITIONS);
	if(cp > cp2)
	{
		i = cp; cp = cp2; cp2 = i;
	}
	for(i = 0; i < cp; i++)
	{
		C->transitions[i] = A->transitions[i];
		D->transitions[i] = B->transitions[i];
	}
	for(i = cp; i < cp2; i++)
	{
		C->transitions[i] = B->transitions[i];
		D->transitions[i] = A->transitions[i];
	}	
	for(i = cp2; i < NUM_TRANSITIONS; i++)
	{
		C->transitions[i] = A->transitions[i];
		D->transitions[i] = B->transitions[i];
	}
}

void Mutation(evo_Context* context, void* gene)
{
	int i;
    StateMachine* machine = (StateMachine*) gene;
	/* Add/remove from set of final states */
	if(RandomDouble(seed) < 0.5)
	{
		i = 1 << Random(seed, 0, NUM_STATES);
		machine->finalState = machine->finalState ^ i;
	}
	/* Modify some transitions. */
	else
	{
		i = Random(seed, 0, NUM_TRANSITIONS);
		machine->transitions[i] = Random(seed, 0, NUM_STATES);
	}
}

evo_bool Success(evo_Context* context)
{
    return context->bestFitness == NUM_TESTS;
}

void Initialize(evo_Config* config)
{
    /* Initialize required attributes */
    evo_Config_SetThreadCount(config, THREAD);
    evo_Config_SetTrialsPerThread(config, TRIALS_PER_THREAD);
    evo_Config_SetMaxIterations(config, MAX_ITERATIONS);
    evo_Config_SetPopulationSize(config, POPULATION_SIZE);
    
    /* Initialize population memory managers */
    evo_Config_SetPopulationInitializer(config, Initialize);
    evo_Config_SetPopulationFinalizer(config, Finalize);
    
    /* Initialize genetic operators */
    evo_Config_SetFitnessOperator(config, Fitness);
    evo_Config_SetSelectionOperator(config, Selection);
    evo_Config_SetCrossoverOperator(config, Crossover);
    evo_Config_SetMutationOperator(config, Mutation);
    
    /* Initialize the success predicate. */
    evo_Config_SetSuccessPredicate(config, Success);
}

void GenerateStats(evo_Stats* stats)
{
    double mean = stats.sumIterations / (double)(stats.trials);
    double variance = stats.sumSquaredIterations / (double)(stats.trials - 1);
    double meanSuccess = stats.sumSuccessIterations / (double)(stats.trials);
    double varianceSuccess = stats.sumSquaredSuccessIterations / (double)(stats.trials - 1);
    printf("Statistics: mean %lf stddev %lf (success mean %lf stddev %lf)", mean, variance, sqrt(meanSuccess), sqrt(varianceSuccess));
}

int main(int argc, char** argv)
{
    /* Create a genetic configuration */
    evo_Config* config = evo_Config_New();

    /* Initilize the configuration.*/
    Initialize(config);
    
    /* Execute the genetic algorithm */
    evo_Config_Execute(config);
    
    /* Generate the statistics.*/
    GenerateStats(config->stats);
    
    /* Free the config. */
    evo_Config_Free(config);
    
    return 0;
}
