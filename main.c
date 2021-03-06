#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>


#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define SIZEPOP                     100
#define NBREPOCH                    1
#define NBRMIX                      (int)((SIZEPOP/2)*0.4)
#define NBRRESET                    (int)((SIZEPOP/2)*0.2)

#define CONNECTX                    4 //nbr of disc to connect to win
#define NBRROWS                     6
#define NBRCOLUMNS                  7
#define NBRVALINCELL                3
#define NBRCELLS                    NBRROWS * NBRCOLUMNS
#define NBRLAYERS                   30
#define NBRNEURONSPERHIDDENLAYER    NBRCELLS * 10
#define INPUT                       0
#define OUTPUT                      NBRLAYERS - 1

#define MAXWEIGHT 2

sem_t semNN[SIZEPOP];

typedef struct NEURON{
    float state;
    struct NEURON **neurons;//those before him
    float *weight;//those before him
    float error;
}NEURON;

typedef struct{
    int player, numNN;
    float score;
    int nbrWin;
    int nbrLayers;
    int *nbrNeuronsPerLayer;
    NEURON ***neurons;
}NEURALNETWORK;


typedef struct{
	int nbr1, nbr2;
    NEURALNETWORK *n1, *n2;
}FIGHTNN;


typedef struct{
    int moves[NBRCELLS];
    int board[NBRCELLS];
    int nbrDiscs[NBRCOLUMNS];
    int player, turn, wonBy;
}CONNECTFOUR;

int cmpfunc (const void * a, const void * b)
{
    if ( (*(NEURALNETWORK **)b)->score > (*(NEURALNETWORK **)a)->score)
        return 1;
    return -1;
}

float randomFloat(void)
{
    return rand()*1.0/RAND_MAX;
}

int randomInt(int mini, int maxi)
{
    return (rand()*1.0/RAND_MAX) * (maxi - mini) + mini;
}

void nextPlayer(CONNECTFOUR *connectFour)
{
    connectFour->player = 1 - connectFour->player;
}

void initBoard(CONNECTFOUR *connectFour)
{
    int cell, column;
    connectFour->player = 1;
    connectFour->turn   = 0;

    for(cell = 0; cell < NBRCELLS; cell++)
    {
        connectFour->moves[cell] = -1;
        connectFour->board[cell] = -1;
    }

    for(column = 0; column < NBRCOLUMNS; column++)
    {
        connectFour->nbrDiscs[column] = 0;
    }
}

int canAddDisc(CONNECTFOUR *connectFour, int numColumn)
{
    if(numColumn < 0 || numColumn > 6)
        return 0;

    return (connectFour->nbrDiscs[numColumn] < NBRROWS);
}

void addDisc(CONNECTFOUR *connectFour, int numColumn)
{
    connectFour->board[connectFour->nbrDiscs[numColumn] * NBRCOLUMNS + numColumn] = connectFour->player;
    connectFour->nbrDiscs[numColumn]++;
    connectFour->moves[connectFour->turn] = numColumn;
    connectFour->turn++;
}

int checkWinRow(CONNECTFOUR *connectFour, int numRow)
{
    int combo = 0;
    int cptColumn;
    for(cptColumn = 0; cptColumn < NBRCOLUMNS; cptColumn++)
    {
        if(connectFour->board[numRow * NBRCOLUMNS + cptColumn] == connectFour->player)
        {
            combo++;
            if(combo == CONNECTX)
                return 1;
        }
        else
            combo = 0;
    }
    return 0;
}

int checkWinColumn(CONNECTFOUR *connectFour, int numRow)
{
    int combo = 0;
    int cptRow;
    for(cptRow = 0; cptRow < NBRROWS; cptRow++)
    {
        if(connectFour->board[cptRow * NBRCOLUMNS + numRow] == connectFour->player)
        {
            combo++;
            if(combo == CONNECTX)
                return 1;
        }
        else
            combo = 0;
    }
    return 0;
}

int checkWinDiagonals(CONNECTFOUR *connectFour, int numColumn)
{
    int combo = 0;
    int height = connectFour->nbrDiscs[numColumn] - 1;

    int x   =   MAX(0, numColumn - height);
    int y   =   MAX(0, height - numColumn);

    while(y < NBRROWS && x < NBRCOLUMNS)
    {
        if(connectFour->board[y * NBRCOLUMNS + x] == connectFour->player)
        {
            combo++;
            if(combo == CONNECTX)
                return 1;
        }
        else
            combo = 0;
        y++; x++;
    }

    combo = 0;

    x   =   MAX(0, MAX(0, height + numColumn) - (NBRROWS - 1));
    y   =   MIN(NBRROWS - 1, MAX(0, height + numColumn));

    while(y >= 0 && x < NBRCOLUMNS)
    {
        if(connectFour->board[y * NBRCOLUMNS + x] == connectFour->player)
        {
            combo++;
            if(combo == CONNECTX)
                return 1;
        }
        else
            combo = 0;
        y--; x++;
    }

    return 0;
}

int isWin(CONNECTFOUR *connectFour, int numColumn)
{
    if(checkWinColumn(connectFour, numColumn) || checkWinRow(connectFour, connectFour->nbrDiscs[numColumn]-1) || checkWinDiagonals(connectFour, numColumn))
    {
        connectFour->wonBy = connectFour->player;
        return 1;
    }
    return 0;
}

int makeMove(CONNECTFOUR *connectFour, int numColumn)
{
    if(canAddDisc(connectFour, numColumn))
    {
        addDisc(connectFour, numColumn);
        return 1;
    }
    printf("Impossible de jouer ici !\n");
    return 0;
}

void printBoard(int *board)
{
    int disc;
    int x, y;
    for(y = NBRROWS - 1; y >= 0; y--)
    {
        for(x = 0; x < NBRCOLUMNS; x++)
        {
            disc    =   board[y * NBRCOLUMNS + x];
            if(disc == 1)
                printf("X ");
            else if (disc == 0)
                printf("O ");
            else
                printf(". ");
        }
        printf("\n");
    }
    printf("0 1 2 3 4 5 6\n\n");
}

void mutationsNetwork(NEURALNETWORK *network, int nbrMutations)
{
    int cptMutations;
    int layer, row, weight;
    float *pWeight;

    for(cptMutations = 0; cptMutations < nbrMutations; cptMutations++)
    {
        layer   =   randomInt(1, NBRLAYERS);
        row     =   randomInt(0, network->nbrNeuronsPerLayer[layer]);
        weight  =   randomInt(0, network->nbrNeuronsPerLayer[layer-1]);
        pWeight =   &network->neurons[layer][row]->weight[weight];
        *pWeight *= ((randomFloat() - 0.5) * 2 * 1.5);

        if(*pWeight > MAXWEIGHT)
            *pWeight = MAXWEIGHT;
        if(*pWeight < -MAXWEIGHT)
            *pWeight = -MAXWEIGHT;
    }
}

NEURALNETWORK *initNetwork(int nbrNeuronsInput, int nbrNeuronsOuput, int nbrLayers, int nbrNeuronsPerLayers)
{
    int layer, row, rowBis;
    NEURALNETWORK *network = malloc(sizeof(NEURALNETWORK));
    if(network == NULL)
        return NULL;

    network->nbrLayers            =   NBRLAYERS;

    network->nbrNeuronsPerLayer   =   malloc(sizeof(int) * NBRLAYERS);
    if(network->nbrNeuronsPerLayer  == NULL)
        return NULL;

    network->neurons            =   malloc(sizeof(NEURON*) * NBRLAYERS);
    if(network->neurons == NULL)
        return NULL;

    network->nbrNeuronsPerLayer[0]            =   nbrNeuronsInput;
    network->nbrNeuronsPerLayer[NBRLAYERS - 1]  =   nbrNeuronsOuput;
    for(layer = 1; layer <  NBRLAYERS - 1; layer++)
        network->nbrNeuronsPerLayer[layer]      =   nbrNeuronsPerLayers;

    for(layer = 0; layer <  NBRLAYERS; layer++)
    {
        network->neurons[layer] =   malloc(sizeof(NEURON*) * network->nbrNeuronsPerLayer[layer]);
        if(network->neurons[layer] == NULL)
            return NULL;
        for(row = 0; row < network->nbrNeuronsPerLayer[layer]; row++)
        {
            network->neurons[layer][row] = malloc(sizeof(NEURON));
            if(network->neurons[layer][row] == NULL)
                return NULL;

            if(layer > 0)
            {
                network->neurons[layer][row]->neurons    = malloc(sizeof(NEURON*) * network->nbrNeuronsPerLayer[layer-1]);
                if(network->neurons[layer][row]->neurons == NULL)
                    return NULL;

                network->neurons[layer][row]->weight     = malloc(sizeof(NEURON) * network->nbrNeuronsPerLayer[layer-1]);
                if(network->neurons[layer][row]->weight == NULL)
                    return NULL;

                for(rowBis = 0; rowBis < network->nbrNeuronsPerLayer[layer-1]; rowBis++)
                {
                    network->neurons[layer][row]->neurons[rowBis] = network->neurons[layer-1][rowBis];
                    network->neurons[layer][row]->weight[rowBis] = 0;
                }
            }
            network->neurons[layer][row]->state = 0;
        }
    }
    return network;
}

void randomisatorWeight(NEURALNETWORK *network)
{
    int layer, row, rowBis;
    for(layer = 1; layer <  NBRLAYERS; layer++)
        for(row = 0; row < network->nbrNeuronsPerLayer[layer]; row++)
            for(rowBis = 0; rowBis < network->nbrNeuronsPerLayer[layer-1]; rowBis++)
                network->neurons[layer][row]->weight[rowBis] = (randomFloat() - 0.5) * 2;
}

float activateFunction(float score)
{
    return 1.0/(1+exp(-score));
}
void setInput(NEURALNETWORK *network, CONNECTFOUR *connectFour)
{
    int cptCell;
    for(cptCell = 0; cptCell < network->nbrNeuronsPerLayer[INPUT]; cptCell += NBRVALINCELL)
    {
        if(network->player == connectFour->board[cptCell / NBRVALINCELL])
        {
            network->neurons[INPUT][cptCell]->state         =   1;
            network->neurons[INPUT][cptCell + 1]->state     =   0;
            network->neurons[INPUT][cptCell + 2]->state     =   0;
        }
        else if (connectFour->board[cptCell / NBRVALINCELL] != -1)
        {
            network->neurons[INPUT][cptCell]->state         =   0;
            network->neurons[INPUT][cptCell + 1]->state     =   1;
            network->neurons[INPUT][cptCell + 2]->state     =   0;
        }
        else
        {
            network->neurons[INPUT][cptCell]->state         =   0;
            network->neurons[INPUT][cptCell + 1]->state     =   0;
            network->neurons[INPUT][cptCell + 2]->state     =   1;
        }
    }
}

void feedNeuralNetwork(NEURALNETWORK *network)
{
    int layer, row, rowBis;
    float totalScore;
    for(layer = 1; layer <  NBRLAYERS; layer++)
        for(row = 0; row < network->nbrNeuronsPerLayer[layer]; row++)
        {
            totalScore = 0;
            for(rowBis = 0; rowBis < network->nbrNeuronsPerLayer[layer-1]; rowBis++)
            {
                totalScore += (network->neurons[layer][row]->weight[rowBis] * network->neurons[layer-1][rowBis]->state);
            }
            network->neurons[layer][row]->state = activateFunction(totalScore);
        }
}

void calculOutputNeuralNetwork(NEURALNETWORK *network, CONNECTFOUR *connectFour)
{
    setInput(network, connectFour);
    feedNeuralNetwork(network);
}
void printNetwork(NEURALNETWORK *network)
{
    int layer, row, rowBis;
    printf("==============\n");
    printf("nbr layers : %d\n", NBRLAYERS);
    for(layer = 0; layer < NBRLAYERS; layer++)
        printf("%d, ", network->nbrNeuronsPerLayer[layer]);
    printf("\n");

    for(layer = 0; layer < NBRLAYERS; layer++)
    {
        for(row = 0; row < network->nbrNeuronsPerLayer[layer]; row++)
        {
            printf("[%d, %d]: ", layer, row);
            //printf("({addr:%p}: ", network->neurons[layer][row]);
            printf("{state:%1.3f} ", network->neurons[layer][row]->state);
            if(layer > 0)
            {
                for(rowBis = 0; rowBis < network->nbrNeuronsPerLayer[layer-1]; rowBis++)
                    //printf("({addr:%p},", network->neurons[layer][row]->neurons[rowBis]);
                    printf("{weight:%f}),", network->neurons[layer][row]->weight[rowBis]);
                //printf("), ", network->neurons[layer][row]->state);
            }
        }
        printf("\n");
    }
    printf("==============\n");
}

int chooseAMove(NEURALNETWORK *network, CONNECTFOUR *connectFour)
{
    float bestState = 0;
    int bestColumn, cptColumn;

    for(cptColumn = 0; cptColumn < NBRCOLUMNS; cptColumn++)
    {
        if(connectFour->nbrDiscs[cptColumn] >= NBRROWS)
            continue;
         if(bestState < network->neurons[OUTPUT][cptColumn]->state)
         {
            bestState = network->neurons[OUTPUT][cptColumn]->state;
            bestColumn = cptColumn;
         }
    }

    return bestColumn;
}

int chooseAMoveByChance(NEURALNETWORK *network, CONNECTFOUR *connectFour)
{
    float total = 0, subTotal = 0;
    int cptColumn;
    float valueRand;

    for(cptColumn = 0; cptColumn < NBRCOLUMNS; cptColumn++)
    {
        if(connectFour->nbrDiscs[cptColumn] >= NBRROWS)
            continue;
        total += network->neurons[OUTPUT][cptColumn]->state;
    }

    valueRand = randomFloat() * total;

    for(cptColumn = 0; cptColumn < NBRCOLUMNS; cptColumn++)
    {
        if(connectFour->nbrDiscs[cptColumn] >= NBRROWS)
            continue;
        subTotal += network->neurons[OUTPUT][cptColumn]->state;
        if(subTotal >= valueRand)
            return cptColumn;
    }

    for(cptColumn = NBRCOLUMNS-1; cptColumn >0; cptColumn--)
    {
        if(connectFour->nbrDiscs[cptColumn] >= NBRROWS)
            continue;
        return cptColumn;
    }
    return -1;
}

void playAGameHvH()
{
    CONNECTFOUR connectFour;
    CONNECTFOUR *pConnectFour;
    int row;

    pConnectFour = &connectFour;

    initBoard(pConnectFour);
    while(pConnectFour->turn <= NBRCELLS)
    {
        printBoard(pConnectFour->board);
        while(1)
        {
            scanf("%d", &row);
            if(makeMove(pConnectFour, row))
                break;
        }
        if(isWin(pConnectFour, row))
        {
            printBoard(pConnectFour->board);
            printf("WIN\n");
            break;
        }
        nextPlayer(pConnectFour);
    }
}

void playAGameHvNN(NEURALNETWORK *network, int humanPlayer)
{
    network->player = 1 - humanPlayer;

    CONNECTFOUR connectFour;
    CONNECTFOUR *pConnectFour;
    int row, cpt;

    pConnectFour = &connectFour;

    initBoard(pConnectFour);
    while(pConnectFour->turn <= NBRCELLS)
    {
        printBoard(pConnectFour->board);
        if(connectFour.player == humanPlayer)
        {
            while(1)
            {
                scanf("%d", &row);
                if(makeMove(pConnectFour, row))
                    break;
            }
        }
        else
        {
            calculOutputNeuralNetwork(network, pConnectFour);
            for(cpt = 0; cpt < NBRCOLUMNS; cpt++)
                printf("%d : %f\n", cpt, network->neurons[OUTPUT][cpt]->state);
            printf("\n");
            row = chooseAMove(network, pConnectFour);
            makeMove(pConnectFour, row);
            printf("choose : %d\n", row);
        }

        if(isWin(pConnectFour, row))
        {
            printBoard(pConnectFour->board);
            printf("WIN\n");
            break;
        }
        nextPlayer(pConnectFour);
    }
}

void loadNN(NEURALNETWORK *network)
{
	int fd = open("NN.data", O_RDWR | O_CREAT, 0777);
    if(read(fd, network, sizeof(NEURALNETWORK)) == -1)
    {
        printf("Erreur de lecture\n");
        exit(-1);
    }
    close(fd);
}

void saveNN(NEURALNETWORK *network)
{
	int fd = open("NN.data", O_RDWR | O_CREAT, 0777);
    printf("%ld\n", write(fd, network, sizeof(NEURALNETWORK)));
    /*
    if(write(fd, network, sizeof(NEURALNETWORK)) == -1)
    {
        printf("Erreur d'ecriture\n");
        exit(-1);
    }
    close(fd);*/
}

void *playAGameNNvNN(void *arg)
{
	FIGHTNN *NN = (FIGHTNN*) arg;
	float score;
    NEURALNETWORK *network;
    
	
    NN->n1->player = 1;
    NN->n2->player = 0;

    CONNECTFOUR connectFour;
    CONNECTFOUR *pConnectFour;
    int row;

    pConnectFour = &connectFour;

    initBoard(pConnectFour);


    while(pConnectFour->turn < NBRCELLS)
    {
        //printBoard(pConnectFour->board);
        if(connectFour.player ==  NN->n1->player)
            network = NN->n1;
        else
            network = NN->n2;

		sem_wait(&semNN[network->numNN]);
			calculOutputNeuralNetwork(network, pConnectFour);
			row = chooseAMoveByChance(network, pConnectFour);
		sem_post(&semNN[network->numNN]);
        if(makeMove(pConnectFour, row) != 1)
        {
            printf("PAS BON !\n");
            printBoard(pConnectFour->board);
            exit(1);
        }
        if(isWin(pConnectFour, row))
        {
            //printBoard(pConnectFour->board);
            //soon semaphore
            score = (NBRCELLS*1.0/pConnectFour->turn);
			sem_wait(&semNN[network->numNN]);
				network->score += score;
			sem_post(&semNN[network->numNN]);
            network->nbrWin ++;
            break;
        }
        nextPlayer(pConnectFour);
    }

}

NEURALNETWORK *chooseAParent(NEURALNETWORK **networks, NEURALNETWORK *network)
{

    float total = 0, subTotal = 0;
    int cptNetwork;
    float valueRand;

    for(cptNetwork = 0; cptNetwork < SIZEPOP; cptNetwork++)
    {
        if(networks[cptNetwork] == network)
            continue;
        total += networks[cptNetwork]->score;
    }

    valueRand = randomFloat() * total;

    for(cptNetwork = 0; cptNetwork < SIZEPOP; cptNetwork++)
    {
        if(networks[cptNetwork] == network)
            continue;
        subTotal += networks[cptNetwork]->score;
        if(subTotal >= valueRand)
            return networks[cptNetwork];
    }

    for(cptNetwork = SIZEPOP-1; cptNetwork >0; cptNetwork--)
    {
        if(networks[cptNetwork] == network)
            continue;
        return networks[cptNetwork];
    }
    return networks[0];
}

void mix(NEURALNETWORK **networks)
{
    int cptMix;
    int layer, row, rowBis;
    float partOf;
    NEURALNETWORK *networkM;
    NEURALNETWORK *networkF;
    NEURALNETWORK *networkC;


    for(cptMix = 0; cptMix < NBRMIX; cptMix++)
    {
        networkM = chooseAParent(networks, NULL);
        networkF = chooseAParent(networks, networkM);
        networkC = initNetwork(NBRCELLS * NBRVALINCELL, NBRCOLUMNS, NBRLAYERS, NBRNEURONSPERHIDDENLAYER);

        for(layer = 1; layer < NBRLAYERS; layer++)
        {
            for(row = 0; row < networkM->nbrNeuronsPerLayer[layer]; row++)
            {
                for(rowBis = 0; rowBis < networkM->nbrNeuronsPerLayer[layer-1]; rowBis++)
                {
                    partOf = randomFloat();
                    networkC->neurons[layer][row]->weight[rowBis] = networkM->neurons[layer][row]->weight[rowBis] * partOf + networkF->neurons[layer][row]->weight[rowBis] * (1-partOf);
                }
            }
        }
        free(networks[SIZEPOP - cptMix - 1]);
        networks[SIZEPOP - cptMix - 1] = networkC;
    }
}


void someResets(NEURALNETWORK **networks)
{
    int cptReset;

    for(cptReset = 0; cptReset < NBRRESET; cptReset++)
        randomisatorWeight(networks[SIZEPOP - cptReset - 1 - NBRMIX]);
}

void *ITSTHEFINALCOUNTDOWN(void)
{
    int cpt, cpt1, cpt2, cptEpoch;
	pthread_t pth[SIZEPOP][SIZEPOP];
	FIGHTNN NN[SIZEPOP][SIZEPOP];
    NEURALNETWORK **networks;
    networks = malloc(sizeof(NEURALNETWORK*) * SIZEPOP);

    if(networks  == NULL)
        return NULL;
	
    printf("%d\n", (int)(sizeof(NEURALNETWORK) + sizeof(int) * NBRLAYERS + sizeof(NEURON*) * NBRLAYERS + NBRLAYERS*(sizeof(NEURON*) * NBRNEURONSPERHIDDENLAYER + NBRLAYERS*(sizeof(NEURON) + sizeof(NEURON*) * NBRNEURONSPERHIDDENLAYER + sizeof(NEURON) * NBRNEURONSPERHIDDENLAYER))));

    for(cpt = 0; cpt < SIZEPOP; cpt++)
    {
		if(sem_init(&semNN[cpt], 0, 1) == -1)
		{
			printf("Erreur de l'init sem\n");
			return NULL;
		}
        networks[cpt] = initNetwork(NBRCELLS * NBRVALINCELL, NBRCOLUMNS, NBRLAYERS, NBRNEURONSPERHIDDENLAYER);
        networks[cpt]->numNN = cpt;
        
        printf("%p\n", networks[cpt]);
        if(networks[cpt] == NULL)
			return NULL;
			
        randomisatorWeight(networks[cpt]);
        
    }

    printf("Game to do per epoch : %d\n", SIZEPOP*(SIZEPOP-1));
    printf("nbrReset : %d, nbrMix : %d\n",  NBRRESET, NBRMIX);
    for(cptEpoch = 0; cptEpoch < NBREPOCH; cptEpoch++)
    {
        printf("Epoch : %d\n", cptEpoch);
        for(cpt = 0; cpt < SIZEPOP; cpt++)
        {
            networks[cpt]->nbrWin = 0;
            networks[cpt]->score = 0;
		}

        for(cpt1 = 0; cpt1 < SIZEPOP; cpt1++)
        for(cpt2 = 0; cpt2 < SIZEPOP; cpt2++)
        {
			if(cpt1 == cpt2) continue;
			
			NN[cpt1][cpt2].nbr1 = cpt1;
			NN[cpt1][cpt2].nbr2 = cpt2;
			
			NN[cpt1][cpt2].n1 = networks[cpt1];
			NN[cpt1][cpt2].n2 = networks[cpt2];
			pthread_create(&pth[cpt1][cpt2], NULL, playAGameNNvNN, &NN[cpt1][cpt2]);
		}

        for(cpt1 = 0; cpt1 < SIZEPOP; cpt1++)
        for(cpt2 = 0; cpt2 < SIZEPOP; cpt2++)
        {
			if(cpt1 == cpt2) continue;
			pthread_join(pth[cpt1][cpt2], NULL);
		}

        qsort(networks, SIZEPOP, sizeof(NEURALNETWORK*), cmpfunc);
        for(cpt1 = 0; cpt1 < SIZEPOP; cpt1++)
			printf("%d: %f\n", networks[cpt1]->nbrWin, networks[cpt1]->score);
        printf("\n");


        mix(networks);
        someResets(networks);

        for(cpt1 = 0; cpt1 < SIZEPOP; cpt1++)
            mutationsNetwork(networks[cpt1], NBRLAYERS * NBRNEURONSPERHIDDENLAYER*0.005);

    }
		cpt = 0;
		while((cpt = 1 - cpt) != 2)
		playAGameHvNN(networks[0], cpt);
    saveNN(networks[0]);

    return networks;
}


int main(void)
{
    srand(time(NULL));
    if(ITSTHEFINALCOUNTDOWN() == NULL)
        printf("PAS BON\n");



    return 0;
}


