#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <windows.h>

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define SIZEPOP                     10
#define NBREPOCH                    1

#define CONNECTX                    4 //nbr of disc to connect to win
#define NBRROWS                     6
#define NBRCOLUMNS                  7
#define NBRVALINCELL                3
#define NBRCELLS                    NBRROWS * NBRCOLUMNS
#define NBRLAYERS                   10
#define NBRNEURONSPERHIDDENLAYER    NBRCELLS * 3
#define INPUT                       0
#define OUTPUT                      NBRLAYERS - 1

#define MAXWEIGHT 2

LARGE_INTEGER frequency;
LARGE_INTEGER start;
LARGE_INTEGER end;
double interval;

clock_t t0clock, t1clock, t2clock, t3clock;

typedef struct NEURON{
    float state;
    struct NEURON **neurons;//those before him
    float *weight;//those before him
    float error;
}NEURON;

typedef struct{
    int player;
    int nbrWin;
    int nbrLayers;
    int *nbrNeuronsPerLayer;
    NEURON ***neurons;
}NEURALNETWORK;


typedef struct{
    int moves[NBRCELLS];
    int board[NBRCELLS];
    int nbrDiscs[NBRCOLUMNS];
    int player, turn, wonBy;
}CONNECTFOUR;

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
        layer   =   randomInt(1, network->nbrLayers);
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

    network->nbrLayers            =   nbrLayers;

    network->nbrNeuronsPerLayer   =   malloc(sizeof(int) * nbrLayers);
    if(network->nbrNeuronsPerLayer  == NULL)
        return NULL;

    network->neurons            =   malloc(sizeof(NEURON*) * nbrLayers);
    if(network->neurons == NULL)
        return NULL;

    network->nbrNeuronsPerLayer[0]            =   nbrNeuronsInput;
    network->nbrNeuronsPerLayer[nbrLayers - 1]  =   nbrNeuronsOuput;
    for(layer = 1; layer <  nbrLayers - 1; layer++)
        network->nbrNeuronsPerLayer[layer]      =   nbrNeuronsPerLayers;

    for(layer = 0; layer <  nbrLayers; layer++)
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
    for(layer = 1; layer <  network->nbrLayers; layer++)
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
    for(layer = 1; layer <  network->nbrLayers; layer++)
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
    printf("nbr layers : %d\n", network->nbrLayers);
    for(layer = 0; layer < network->nbrLayers; layer++)
        printf("%d, ", network->nbrNeuronsPerLayer[layer]);
    printf("\n");

    for(layer = 0; layer < network->nbrLayers; layer++)
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
    int row;

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
            row = chooseAMove(network, pConnectFour);
            makeMove(pConnectFour, row);
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

void playAGameNNvNN(NEURALNETWORK *n1, NEURALNETWORK *n2)
{
    NEURALNETWORK *network;
    n1->player = 1;
    n2->player = 0;

    CONNECTFOUR connectFour;
    CONNECTFOUR *pConnectFour;
    int row;

    pConnectFour = &connectFour;

    initBoard(pConnectFour);


    while(pConnectFour->turn < NBRCELLS)
    {
        //printBoard(pConnectFour->board);
        if(connectFour.player == n1->player)
            network = n1;
        else
            network = n2;

        calculOutputNeuralNetwork(network, pConnectFour);
        row = chooseAMove(network, pConnectFour);
        if(makeMove(pConnectFour, row) != 1)
        {
            printf("PAS BON !\n");
            printBoard(pConnectFour->board);
            exit(1);
        }
        if(isWin(pConnectFour, row))
        {
            //printBoard(pConnectFour->board);
            network->nbrWin++;
            return;
        }
        nextPlayer(pConnectFour);
    }

}

int cmpfunc (const void * a, const void * b)
{
    const NEURALNETWORK* n1 = *(NEURALNETWORK **)a;
    const NEURALNETWORK* n2 = *(NEURALNETWORK **)b;
    return ( n2->nbrWin - n1->nbrWin);
}

void *ITSTHEFINALCOUNTDOWN(void)
{
    int cpt, cpt1, cpt2, cptEpoch;
    srand(time(NULL));

    NEURALNETWORK **networks;
    networks = malloc(sizeof(NEURALNETWORK*) * SIZEPOP);

    if(networks  == NULL)
        return NULL;


    for(cpt = 0; cpt < SIZEPOP; cpt++)
    {
        networks[cpt] = initNetwork(NBRCELLS * NBRVALINCELL, NBRCOLUMNS, NBRLAYERS, NBRNEURONSPERHIDDENLAYER);
        randomisatorWeight(networks[cpt]);
        printf("%p\n", networks[cpt]);
    }

    printf("Game to do per epoch : %d\n", SIZEPOP*SIZEPOP);
    for(cptEpoch = 0; cptEpoch < NBREPOCH; cptEpoch++)
    {
        printf("Epoch : %d\n", cptEpoch);
        for(cpt = 0; cpt < SIZEPOP; cpt++)
            networks[cpt]->nbrWin = 0;

        QueryPerformanceCounter(&start);

        for(cpt1 = 0; cpt1 < SIZEPOP; cpt1++)
        for(cpt2 = 0; cpt2 < SIZEPOP; cpt2++)
            playAGameNNvNN(networks[cpt1], networks[cpt2]);

        QueryPerformanceCounter(&end);
        interval = (double) (end.QuadPart - start.QuadPart) / frequency.QuadPart;
        printf("%1.20f\n", interval);


        for(cpt = 0; cpt < SIZEPOP; cpt++)
            printf("%d\n", networks[cpt]->nbrWin);

        qsort(networks, SIZEPOP, sizeof(NEURALNETWORK*), cmpfunc);
        printf("\n");

        for(cpt = 0; cpt < SIZEPOP; cpt++)
            printf("%d\n", networks[cpt]->nbrWin);

    }

    return networks;
}


int main(void)
{
    QueryPerformanceFrequency(&frequency);
    if(ITSTHEFINALCOUNTDOWN() == NULL)
        printf("PAS BON\n");



    return 0;
}

