#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h> //this is for INT_MAX if I use it
#include <unistd.h>

#include <assert.h>
#include <stdbool.h>

#include <execinfo.h>
#include <stdio.h>
#include <ncurses.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <climits>
#include <cmath>

// Adapted from https://stackoverflow.com/questions/9555837/how-to-know-caller-function-when-tracing-assertion-failure
void crash()
{
    void *callstack[128];
    int i, frames = backtrace(callstack, 128);
    char **strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i)
    {
        printw("%s\n", strs[i]);
    }
    free(strs);
    assert(false);
}

/////////////////////////////////////////////////////////////PROTOTYPES///////////////////////////////////////////////
class Gym;
class Map;
class WorldMap;
class Character;
class PC;
class NPC;
class personalPokemon;
struct GQnode;
struct GodQueue;

int calc_Cost(int terrain, int chartype);
int findNextPos(NPC *character, Map *m, WorldMap *WM, PC *player);
int generateCharacters(Map *m, WorldMap *WM, PC *player, int numTrainers);
int enqueueAllChars(GodQueue *GQ, Map *m);
int clearScreen();
int clearScreen_top();
int createPanel(int topRow, int bottomRow, int leftCol, int rightCol);

// CONSTANTS
const int MAX = 42069666;
const int NO_DIRECTION = -1, NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3;
#define KEY_ESC 27
int NUMTRAINERS = 0;

int w_row;
int w_col;
Gym *gyms[9];

typedef enum GymType
{
    PEWTER_GYM,
    CERULEAN_GYM,
    VERMILION_GYM,
    CELADON_GYM,
    FUSHSIA_GYM,
    SAFFRON_GYM,
    CINNABAR_GYM,
    VIRIDIAN_GYM,
    ELITEFOUR_GYM
} GymType;

typedef enum CharacterType
{
    CT_PLAYER,
    CT_HIKER,
    CT_RIVAL,
    CT_SWIMMER,
    CT_PACER,
    CT_WANDERER,
    CT_EXPLORER,
    CT_SENTRY,
    CT_OTHER,
    CT_LEADER

} CharacterType;

typedef enum TerrainType
{
    TT_NO_TERRAIN,
    TT_BOULDER,
    TT_TREE,
    TT_PATH,
    TT_BRIDGE,
    TT_PMART,
    TT_PCENTER,
    TT_TGRASS,
    TT_SGRASS,
    TT_MOUNTAIN,
    TT_FOREST,
    TT_WATER,
    TT_GATE,

    TT_PEWTER,
    TT_CERULEAN,
    TT_VERMILION,
    TT_CELADON,
    TT_FUSHSIA,
    TT_SAFFRON,
    TT_CINNABAR,
    TT_VIRIDIAN,
    TT_ELITEFOUR
} TerrainType;

// function that takes a characterType enum and returns the corresponding character
char CT_to_Char(int CT)
{
    switch (CT)
    {
    case CT_PLAYER:
        return '@';
    case CT_HIKER:
        return 'H';
    case CT_RIVAL:
        return 'R';
    case CT_SWIMMER:
        return 'M';
    case CT_PACER:
        return 'P';
    case CT_WANDERER:
        return 'W';
    case CT_EXPLORER:
        return 'E';
    case CT_SENTRY:
        return 'S';
    case CT_LEADER:
        return 'L';
    default:
        return '!'; // this means there was an error!
    }
}

//////////////////////////////////////////////////////////////////////////////////GOD QUEUE//////////////////////////////////////////////////////
struct GQnode
{
    int row;
    int col;
    int value;
    Character *character;
    struct GQnode *next;
};

struct GodQueue
{
    GQnode *head;
    int length;
};

int GQinit(GodQueue *q)
{
    q->head = NULL;
    q->length = 0;

    return 0; // success
}

int GQenqueue(GodQueue *q, int newRow, int newCol, int newVal, Character *newCharacter)
{
    GQnode *newNode;
    if (!(newNode = (GQnode *)calloc(sizeof(*newNode), 1)))
    {
        return 1; // means failed to make new node
    }
    newNode->row = newRow;
    newNode->col = newCol;
    newNode->value = newVal;
    newNode->character = newCharacter;

    if (q->length == 0)
    { // if queue is empty make new node the front
        q->head = newNode;
    }
    else if (q->head->value > newNode->value)
    {
        newNode->next = q->head;
        q->head = newNode;
    }
    else
    { // queue has nodes in it so put it in the correct place based on its value
        GQnode *temp = q->head;
        // assert(temp);
        while (temp->next != NULL)
        {
            if (temp->next->value >= newNode->value)
            {
                break;
            }
            temp = temp->next;
        }
        newNode->next = temp->next;
        temp->next = newNode;
    }
    q->length++;
    return 0;
}

int GQdequeue(GodQueue *q, GQnode *returnNode)
{
    if (q->length <= 0)
    {
        printw("\nERROR!!!!!: trying to call dequeue on an empty queue!\n");
        return 1; // nothing to dequeue
    }
    returnNode->row = q->head->row;
    returnNode->col = q->head->col;
    returnNode->value = q->head->value;
    returnNode->character = q->head->character;

    if (q->length == 1)
    { // there is a single item in the queue
        free(q->head);
        q->head = NULL; // you removed the last elements
    }
    else
    { // there are multiple items in the queue
        GQnode *temp = q->head;
        q->head = q->head->next; // change the front
        free(temp);
    }
    q->length--;
    return 0; // success
}

int GQdequeue_RC(GodQueue *q, int *retRow, int *retCol)
{

    if (q->length <= 0)
    {
        printw("\nERROR: trying to call dequeue on an empty queue\n");
        crash();
        return 1; // nothing to dequeue
    }
    *retRow = q->head->row;
    *retCol = q->head->col;

    if (q->length == 1)
    { // there is a single item in the queue
        free(q->head);
        q->head = NULL; // you removed the last elements
    }
    else
    { // there are multiple items in the queue
        GQnode *temp = q->head;
        q->head = q->head->next; // change the front
        free(temp);
    }
    q->length--;
    return 0; // success
}

int GQis_empty(GodQueue *q)
{
    return !q->length;
}

int GQsize(GodQueue *q)
{
    return q->length;
}

int GQdequeueAll(GodQueue *q)
{
    GQnode *temp = q->head;
    while (temp != NULL)
    {
        temp = temp->next;
        free(q->head);
        q->length--;
        q->head = temp;
    }
    return 0;
}

int GQdestroy(GodQueue *q)
{
    GQnode *tmp;
    while ((tmp = q->head))
    {
        free(tmp);
        q->head = q->head->next;
    }

    q->length = 0;
    return 0;
}

class WorldMap
{
public:
    Map *mapGrid[401][401];
    int hiker_CM[21][80];
    int rival_CM[21][80];
    GodQueue charQueue;

    Character *player;

    WorldMap() : mapGrid()
    {

        for (int i = 0; i < 401; i++)
        {
            for (int j = 0; j < 401; j++)
            {
                mapGrid[i][j] = NULL;
            }
        }
        GQinit(&(charQueue)); // initialize the character queue

        // Creation of gyms
        // for (int i = 0; i < 9; i++)
        // {
        // gyms[i] = new Gym(i);
        // Gym *newGym = new Gym(1);
        // }
    }

    ~WorldMap()
    {
        for (int i = 0; i < 401; ++i)
        {
            for (int j = 0; j < 401; ++j)
            {
                Map *p = mapGrid[i][j];
                if (p != NULL)
                {
                    free(p);
                }
            }
        }
    }
};

// structure to store path direction probability
struct PathProb
{
    int numV; // stores how many vertical moves need to be made
    int numH; // stores how many horizontal moves need to be made
    int dirV; // tells if the V direction needs to move down (1) or up (-1) [signs were determined by how the grid indexing was layed out]
    int dirH; // tells whether the Hshift needs to be right (1) or left (-1)
};

// initializing the structure to store Path probability
int initPathProb(PathProb *p, int startRow, int startCol, int goalRow, int goalCol)
{
    p->numV = abs(goalRow - startRow);
    p->numH = abs(goalCol - startCol);

    // determining vertical displacement direction
    if (goalRow - startRow > 0)
    {
        p->dirV = 1;
    }
    else
    {
        p->dirV = -1;
    }

    // determining horizontal displacement direction
    if (goalCol - startCol > 0)
    {
        p->dirH = 1;
    }
    else
    {
        p->dirH = -1;
    }

    return 0; // success
}

class Character
{
public:
    int type;
    int row;
    int col;
    int nextRow;
    int nextCol;
    int nextCost;

    std::vector<personalPokemon> party;

    int updateCoords(int newRow, int newCol)
    {
        row = newRow;
        col = newCol;
        return 0;
    }

    int updateNextCoords(int newNextRow, int newNextCol)
    {
        nextRow = newNextRow;
        nextCol = newNextCol;
        return 0;
    }

    virtual ~Character() {}
};

class Gym
{
public:
    int Ggrid[21][40];
    Character *charGrid[21][40];
    std::vector<NPC> leaders;
    int w_row;
    int w_col;
    std::string badge;
    std::string name;
    int terrainType;

    bool leaderSet;

    // Gym() : leaders(0), w_row(0), w_col(0), badge("None")
    // {
    // }

    Gym(int gym)
    {
        leaderSet = false;
        for (int i = 0; i < 21; i++)
        {
            for (int j = 0; j < 40; j++)
            {
                charGrid[i][j] = NULL;
            }
        }

        switch (gym)
        {
        case (0):
        {
            name = "Pewter Gym";
            terrainType = 13;
            int randX, randY;
            do
            {
                randX = rand() % (225 - 175) + 175;
                randY = rand() % (225 - 175) + 175;
            } while ((abs(randX - 200) + abs(200 - randY)) < 1 || (abs(randX - 200) + abs(200 - randY)) > 50);

            w_row = randX;
            w_col = randY;

            std::ifstream file;
            file.open("pewterGym.txt");
            std::string curLine;
            for (int i = 0; i < 21; i++)
            {
                getline(file, curLine);
                for (int j = 0; j < 40; j++)
                {
                    if (curLine[j] == '%')
                    {
                        Ggrid[i][j] = TT_BOULDER;
                    }
                    else if (curLine[j] == '.')
                    {
                        Ggrid[i][j] = TT_SGRASS;
                    }
                    else if (curLine[j] == '~')
                    {
                        Ggrid[i][j] = TT_WATER;
                    }
                    else if (curLine[j] == '#')
                    {
                        Ggrid[i][j] = TT_PATH;
                    }
                    else if (curLine[j] == '^')
                    {
                        Ggrid[i][j] = TT_TREE;
                    }
                    else
                    {
                        Ggrid[i][j] = TT_NO_TERRAIN;
                    }
                }
            }
            badge = "Boulder Badge";
            // NPC *brock;
            // leaders.push_back(*brock);
            // NPC *brock = new NPC(CT_LEADER, 6, "Brock", w_row, w_col);
            // charGrid[brock->row][brock->col] = brock;
            // leaders.push_back(*brock);
            break;
        }
        case (1):
        {
            name = "Cerulean Gym";
            terrainType = 14;
            int randX, randY;
            do
            {
                randX = rand() % (250 - 200) + 200;
                randY = rand() % (250 - 200) + 200;
            } while ((abs(randX - 200) + abs(200 - randY)) < 51 || (abs(randX - 200) + abs(200 - randY)) > 100);

            w_row = randX;
            w_col = randY;

            std::ifstream file;
            file.open("ceruleanGym.txt");
            std::string curLine;
            for (int i = 0; i < 21; i++)
            {
                getline(file, curLine);
                for (int j = 0; j < 40; j++)
                {
                    if (curLine[j] == '%')
                    {
                        Ggrid[i][j] = TT_BOULDER;
                    }
                    else if (curLine[j] == '.')
                    {
                        Ggrid[i][j] = TT_SGRASS;
                    }
                    else if (curLine[j] == '~')
                    {
                        Ggrid[i][j] = TT_WATER;
                    }
                    else if (curLine[j] == '#')
                    {
                        Ggrid[i][j] = TT_PATH;
                    }
                    else if (curLine[j] == '^')
                    {
                        Ggrid[i][j] = TT_TREE;
                    }
                    else
                    {
                        Ggrid[i][j] = TT_NO_TERRAIN;
                    }
                }
            }
            badge = "Cascade Badge";
            // NPC *misty = new NPC(CT_LEADER, 11, "Misty", w_row, w_col);
            // charGrid[misty->row][misty->col] = misty;
            // leaders.push_back(*misty);
            break;
        }
        case (2):
        {
            name = "Vermillion Gym";
            terrainType = 15;
            int randX, randY;
            do
            {
                randX = rand() % (275 - 225) + 225;
                randY = rand() % (275 - 225) + 225;
            } while ((abs(randX - 200) + abs(200 - randY)) < 101 || (abs(randX - 200) + abs(200 - randY)) > 150);

            w_row = randX;
            w_col = randY;

            std::ifstream file;
            file.open("vermillionGym.txt");
            std::string curLine;
            for (int i = 0; i < 21; i++)
            {
                getline(file, curLine);
                for (int j = 0; j < 40; j++)
                {
                    if (curLine[j] == '%')
                    {
                        Ggrid[i][j] = TT_BOULDER;
                    }
                    else if (curLine[j] == '.')
                    {
                        Ggrid[i][j] = TT_SGRASS;
                    }
                    else if (curLine[j] == '~')
                    {
                        Ggrid[i][j] = TT_WATER;
                    }
                    else if (curLine[j] == '#')
                    {
                        Ggrid[i][j] = TT_PATH;
                    }
                    else if (curLine[j] == '^')
                    {
                        Ggrid[i][j] = TT_TREE;
                    }
                    else
                    {
                        Ggrid[i][j] = TT_NO_TERRAIN;
                    }
                }
            }
            badge = "Thunder Badge";
            // NPC *lt_Surge = new NPC(CT_LEADER, 13, "Lt. Surge", w_row, w_col);
            // charGrid[lt_Surge->row][lt_Surge->col] = lt_Surge;
            // leaders.push_back(*lt_Surge);
            break;
        }
        case (3):
        {
            name = "Celadon Gym";
            terrainType = 16;
            int randX, randY;
            do
            {
                randX = rand() % (300 - 275) + 275;
                randY = rand() % (300 - 275) + 275;
            } while ((abs(randX - 200) + abs(200 - randY)) < 151 || (abs(randX - 200) + abs(200 - randY)) > 200);

            w_row = randX;
            w_col = randY;

            std::ifstream file;
            file.open("celadonGym.txt");
            std::string curLine;
            for (int i = 0; i < 21; i++)
            {
                getline(file, curLine);
                for (int j = 0; j < 40; j++)
                {
                    if (curLine[j] == '%')
                    {
                        Ggrid[i][j] = TT_BOULDER;
                    }
                    else if (curLine[j] == '.')
                    {
                        Ggrid[i][j] = TT_SGRASS;
                    }
                    else if (curLine[j] == '~')
                    {
                        Ggrid[i][j] = TT_WATER;
                    }
                    else if (curLine[j] == '#')
                    {
                        Ggrid[i][j] = TT_PATH;
                    }
                    else if (curLine[j] == '^')
                    {
                        Ggrid[i][j] = TT_TREE;
                    }
                    else
                    {
                        Ggrid[i][j] = TT_NO_TERRAIN;
                    }
                }
            }
            badge = "Rainbow Badge";
            // NPC *erika = new NPC(CT_LEADER, 12, "Erika", w_row, w_col);
            // charGrid[erika->row][erika->col] = erika;
            // leaders.push_back(*erika);
            break;
        }
        case (4):
        {
            name = "Fushia Gym";
            terrainType = 17;
            int randX, randY;
            do
            {
                randX = rand() % (325 - 300) + 300;
                randY = rand() % (325 - 300) + 300;
            } while ((abs(randX - 200) + abs(200 - randY)) < 201 || (abs(randX - 200) + abs(200 - randY)) > 250);

            w_row = randX;
            w_col = randY;

            std::ifstream file;
            file.open("fushiaGym.txt");
            std::string curLine;
            for (int i = 0; i < 21; i++)
            {
                getline(file, curLine);
                for (int j = 0; j < 40; j++)
                {
                    if (curLine[j] == '%')
                    {
                        Ggrid[i][j] = TT_BOULDER;
                    }
                    else if (curLine[j] == '.')
                    {
                        Ggrid[i][j] = TT_SGRASS;
                    }
                    else if (curLine[j] == '~')
                    {
                        Ggrid[i][j] = TT_WATER;
                    }
                    else if (curLine[j] == '#')
                    {
                        Ggrid[i][j] = TT_PATH;
                    }
                    else if (curLine[j] == '^')
                    {
                        Ggrid[i][j] = TT_TREE;
                    }
                    else
                    {
                        Ggrid[i][j] = TT_NO_TERRAIN;
                    }
                }
            }
            badge = "Soul Badge";
            // NPC *koga = new NPC(CT_LEADER, 4, "Koga", w_row, w_col);
            // charGrid[koga->row][koga->col] = koga;
            // leaders.push_back(*koga);
            // NPC *janine = new NPC(CT_LEADER, 4, "Janine", w_row, w_col);
            // charGrid[janine->row][janine->col] = janine;
            // leaders.push_back(*janine);
            break;
        }
        case (5):
        {
            name = "Saffron Gym";
            terrainType = 18;
            int randX, randY;
            do
            {
                randX = rand() % (350 - 325) + 325;
                randY = rand() % (350 - 325) + 325;
            } while ((abs(randX - 200) + abs(200 - randY)) < 251 || (abs(randX - 200) + abs(200 - randY)) > 300);

            w_row = randX;
            w_col = randY;

            std::ifstream file;
            file.open("pewterGym.txt");
            std::string curLine;
            for (int i = 0; i < 21; i++)
            {
                getline(file, curLine);
                for (int j = 0; j < 40; j++)
                {
                    if (curLine[j] == '%')
                    {
                        Ggrid[i][j] = TT_BOULDER;
                    }
                    else if (curLine[j] == '.')
                    {
                        Ggrid[i][j] = TT_SGRASS;
                    }
                    else if (curLine[j] == '~')
                    {
                        Ggrid[i][j] = TT_WATER;
                    }
                    else if (curLine[j] == '#')
                    {
                        Ggrid[i][j] = TT_PATH;
                    }
                    else if (curLine[j] == '^')
                    {
                        Ggrid[i][j] = TT_TREE;
                    }
                    else
                    {
                        Ggrid[i][j] = TT_NO_TERRAIN;
                    }
                }
            }
            badge = "Marsh Badge";
            // NPC *sabrina = new NPC(CT_LEADER, 14, "Sabrina", w_row, w_col);
            // charGrid[sabrina->row][sabrina->col] = sabrina;
            // leaders.push_back(*sabrina);
            break;
        }
        case (6):
        {
            name = "Cinnabar Gym";
            terrainType = 19;
            int randX, randY;
            do
            {
                randX = rand() % (375 - 350) + 350;
                randY = rand() % (375 - 350) + 350;
            } while ((abs(randX - 200) + abs(200 - randY)) < 301 || (abs(randX - 200) + abs(200 - randY)) > 350);

            w_row = randX;
            w_col = randY;

            std::ifstream file;
            file.open("pewterGym.txt");
            std::string curLine;
            for (int i = 0; i < 21; i++)
            {
                getline(file, curLine);
                for (int j = 0; j < 40; j++)
                {
                    if (curLine[j] == '%')
                    {
                        Ggrid[i][j] = TT_BOULDER;
                    }
                    else if (curLine[j] == '.')
                    {
                        Ggrid[i][j] = TT_SGRASS;
                    }
                    else if (curLine[j] == '~')
                    {
                        Ggrid[i][j] = TT_WATER;
                    }
                    else if (curLine[j] == '#')
                    {
                        Ggrid[i][j] = TT_PATH;
                    }
                    else if (curLine[j] == '^')
                    {
                        Ggrid[i][j] = TT_TREE;
                    }
                    else
                    {
                        Ggrid[i][j] = TT_NO_TERRAIN;
                    }
                }
            }
            badge = "Volcano Badge";
            // NPC *blane = new NPC(CT_LEADER, 10, "blane", w_row, w_col);
            // charGrid[blane->row][blane->col] = blane;
            // leaders.push_back(*blane);
            break;
        }
        case (7):
        {
            name = "Viridian Gym";
            terrainType = 20;
            int randX, randY;
            do
            {
                randX = rand() % (400 - 375) + 375;
                randY = rand() % (400 - 375) + 375;
            } while ((abs(randX - 200) + abs(200 - randY)) < 350 || (abs(randX - 200) + abs(200 - randY)) > 400);

            w_row = randX;
            w_col = randY;

            std::ifstream file;
            file.open("pewterGym.txt");
            std::string curLine;
            for (int i = 0; i < 21; i++)
            {
                getline(file, curLine);
                for (int j = 0; j < 40; j++)
                {
                    if (curLine[j] == '%')
                    {
                        Ggrid[i][j] = TT_BOULDER;
                    }
                    else if (curLine[j] == '.')
                    {
                        Ggrid[i][j] = TT_SGRASS;
                    }
                    else if (curLine[j] == '~')
                    {
                        Ggrid[i][j] = TT_WATER;
                    }
                    else if (curLine[j] == '#')
                    {
                        Ggrid[i][j] = TT_PATH;
                    }
                    else if (curLine[j] == '^')
                    {
                        Ggrid[i][j] = TT_TREE;
                    }
                    else
                    {
                        Ggrid[i][j] = TT_NO_TERRAIN;
                    }
                }
            }
            badge = "Earth Badge";
            // NPC *giovanni = new NPC(CT_LEADER, 5, "Giovanni", w_row, w_col);
            // charGrid[giovanni->row][giovanni->col] = giovanni;
            // leaders.push_back(*giovanni);
            // NPC *blue = new NPC(CT_LEADER, rand() % 19, "Blue", w_row, w_col);
            // charGrid[blue->row][blue->col] = blue;
            // leaders.push_back(*blue);
            break;
        }
        case (8):
        {
            name = "Final Gym";
            terrainType = 21;
            int randX, randY;
            if (rand() % 2 == 0 ? randX = 0 : randX = 399);
            if (randX == 0 ? randY = 399 : randY = 0);
            // randY = 399
            w_row = randX;
            w_col = randY;

            std::ifstream file;
            file.open("pewterGym.txt");
            std::string curLine;
            for (int i = 0; i < 21; i++)
            {
                getline(file, curLine);
                for (int j = 0; j < 40; j++)
                {
                    if (curLine[j] == '%')
                    {
                        Ggrid[i][j] = TT_BOULDER;
                    }
                    else if (curLine[j] == '.')
                    {
                        Ggrid[i][j] = TT_SGRASS;
                    }
                    else if (curLine[j] == '~')
                    {
                        Ggrid[i][j] = TT_WATER;
                    }
                    else if (curLine[j] == '#')
                    {
                        Ggrid[i][j] = TT_PATH;
                    }
                    else if (curLine[j] == '^')
                    {
                        Ggrid[i][j] = TT_TREE;
                    }
                    else
                    {
                        Ggrid[i][j] = TT_NO_TERRAIN;
                    }
                }
            }

            badge = "Champion Badge";
            // NPC *lorelai = new NPC(CT_LEADER, 15, "Lorelai", w_row, w_col);
            // charGrid[lorelai->row][lorelai->col] = lorelai;
            // leaders.push_back(*lorelai);
            // NPC *bruno = new NPC(CT_LEADER, 2, "Bruno", w_row, w_col);
            // charGrid[bruno->row][bruno->col] = bruno;
            // leaders.push_back(*bruno);
            // NPC *agatha = new NPC(CT_LEADER, 8, "Agatha", w_row, w_col);
            // charGrid[agatha->row][agatha->col] = agatha;
            // leaders.push_back(*agatha);
        }
        }
    }

    int printGym()
    {
        // clear the screen
        clear();

        // mvprintw(30, 0, "Goes in");
        for (int i = 0; i < 21; i++)
        {
            for (int j = 0; j < 40; j++)
            {
                if (charGrid[i][j] != NULL)
                { // if there is a character at the given spot, print the character
                    int curChar = charGrid[i][j]->type;
                    attron(COLOR_PAIR(COLOR_RED));
                    if (curChar < 0 || curChar == CT_OTHER)
                    {
                        mvprintw(0, 0, "ERROR: invalid character type at row:%d col:%d)", i, j);
                        break;
                    }
                    else
                    {
                        if (curChar == CT_PLAYER || curChar == CT_LEADER)
                        {
                            attron(A_BOLD);
                            mvprintw(i + 1, j, "%c", CT_to_Char(curChar));
                            attroff(A_BOLD);
                        }
                        else
                        {
                            mvprintw(i + 1, j, "%c", CT_to_Char(curChar));
                        }
                    }
                    attroff(COLOR_PAIR(COLOR_RED));
                }
                else
                { // else print the terrain
                    int curTerrain = Ggrid[i][j];
                    switch (curTerrain)
                    {
                    case TT_SGRASS:
                        attron(COLOR_PAIR(COLOR_GREEN));
                        mvprintw(i + 1, j, ".");
                        attroff(COLOR_PAIR(COLOR_GREEN));
                        break;
                    case TT_TGRASS:
                        attron(A_BOLD);
                        attron(COLOR_PAIR(COLOR_GREEN));
                        mvprintw(i + 1, j, ":");
                        attroff(COLOR_PAIR(COLOR_GREEN));
                        attroff(A_BOLD);
                        break;
                    case TT_WATER:
                        attron(COLOR_PAIR(COLOR_BLUE));
                        mvprintw(i + 1, j, "~");
                        attroff(COLOR_PAIR(COLOR_BLUE));
                        break;
                    case TT_PATH: // paths are '#' so this continues
                    case TT_GATE: // gates are '#' so this continues
                    case TT_BRIDGE:
                        attron(COLOR_PAIR(COLOR_YELLOW));
                        mvprintw(i + 1, j, "#");
                        attroff(COLOR_PAIR(COLOR_YELLOW));
                        break;
                    case TT_TREE: // trees are '^' so this continues
                    case TT_FOREST:
                        attron(COLOR_PAIR(COLOR_GREEN));
                        mvprintw(i + 1, j, "^");
                        attroff(COLOR_PAIR(COLOR_GREEN));
                        break;
                    case TT_BOULDER: // boulders are '%' so this continues
                    case TT_MOUNTAIN:
                        mvprintw(i + 1, j, "%%");
                        break;
                    case TT_NO_TERRAIN:
                        mvprintw(i + 1, j, "0");
                        break;
                    default:
                        mvprintw(0, 0, "ERROR: invalid terrain type");
                        break;
                    }
                }
            }
        }
        refresh();
        return 0;
    };

    // int playGym(PC)
};

class Map
{
public:
    int Tgrid[21][80];
    Character *charGrid[21][80];
    int globalRow;
    int globalCol;
    int gateN;
    int gateE;
    int gateS;
    int gateW;

    // generates an empty map
    Map(int newNgate, int newEgate, int newSgate, int newWgate, int row, int col)
    {
        for (int i = 0; i < 21; i++)
        {
            for (int j = 0; j < 80; j++)
            {
                Tgrid[i][j] = TT_NO_TERRAIN; // assigning every part of the map grid as 0
                charGrid[i][j] = NULL;       // making every cell of the character map origionally null
            }
        }
        gateN = newNgate;
        gateE = newEgate;
        gateS = newSgate;
        gateW = newWgate;
        globalRow = row;
        globalCol = col;
    }

    int generateBiomes()
    {
        srand(time(NULL));
        GodQueue TT_WATERBQ, mountainBQ, TT_TGRASSBQ1, TT_TGRASSBQ2, TT_SGRASSBQ1, TT_SGRASSBQ2, TT_FORESTBQ;
        GQinit(&TT_WATERBQ);
        GQinit(&mountainBQ);
        GQinit(&TT_TGRASSBQ1);
        GQinit(&TT_TGRASSBQ2);
        GQinit(&TT_SGRASSBQ1);
        GQinit(&TT_SGRASSBQ2);
        GQinit(&TT_FORESTBQ);
        int biomesList[7] = {TT_WATER, TT_MOUNTAIN, TT_TGRASS, TT_TGRASS, TT_SGRASS, TT_SGRASS, TT_FOREST};
        GodQueue BQlist[7] = {TT_WATERBQ, mountainBQ, TT_TGRASSBQ1, TT_TGRASSBQ2, TT_SGRASSBQ1, TT_SGRASSBQ2, TT_FORESTBQ}; // 7 is the number of biomes there are

        // initialize all biomesqueues to empty
        for (int i = 0; i < 7; i++)
        {
            GQinit(&BQlist[i]);
        }

        // seed all of the biomes & place surrounding cells into correct queue if they are in bounds
        int seedRow;
        int seedCol;
        int precedence = 0;
        for (int i = 0; i < 7; i++)
        {                              // for each biome, create a seed
            seedRow = rand() % 19 + 1; // generates a number from 1 to 19
            seedCol = rand() % 78 + 1; // generates a number from 1 to 78
            while (Tgrid[seedRow][seedCol] != TT_NO_TERRAIN)
            { // if there is already a seed where you want to place a seed, you have to find a new location
                seedRow = rand() % 19 + 1;
                seedCol = rand() % 78 + 1;
            }
            Tgrid[seedRow][seedCol] = biomesList[i]; // place the correct character at the seed spot in the board

            // have 4 if statements to see whether surrounding spaces are in bounds. if they are, add them to the correct queue. All have precedence 0 becuase they are the first to be entered (FiFo queue)
            if (seedRow - 1 >= 0)
            {
                GQenqueue(&BQlist[i], seedRow - 1, seedCol, 0, NULL);
            } // check if space above seed is in bounds
            if (seedRow + 1 <= 20)
            {
                GQenqueue(&BQlist[i], seedRow + 1, seedCol, 1, NULL);
            } // checks if space below seed is in bounds
            if (seedCol - 1 >= 0)
            {
                GQenqueue(&BQlist[i], seedRow, seedCol - 1, 2, NULL);
            } // checks if space to left of seed is in bounds
            if (seedCol + 1 <= 79)
            {
                GQenqueue(&BQlist[i], seedRow, seedCol + 1, 3, NULL);
            } // checks if space to right of seed is in bounds
        }

        // Grow biomes from the seeds
        int curRow;
        int curCol;
        precedence = 4; // a number to make sure the BQ dequeues correctly (the number starts at 0 and increase by 1 every time something is enqueued so that ones enqueued sooner have higher precedence)

        while (!(GQis_empty(&BQlist[0]) && GQis_empty(&BQlist[1]) && GQis_empty(&BQlist[2]) && GQis_empty(&BQlist[3]) && GQis_empty(&BQlist[4]) && GQis_empty(&BQlist[5]) && GQis_empty(&BQlist[6])))
        { // while loop that goes till all queues are empty
            for (int i = 0; i < 7; i++)
            {
                if (GQis_empty(&BQlist[i]))
                {
                    continue;
                }
                // for(int j=0; j<25; j++){  //uncommenting this line and the line 10 lines down might make it less jagged? (NOTE: jaggedness is possibly due to the fact that the characters are horizontally closter together than they are vertically)
                if (!GQdequeue_RC(&BQlist[i], &curRow, &curCol))
                { // check to see if current queue is empty
                    if (Tgrid[curRow][curCol] == TT_NO_TERRAIN)
                    {                                          // if the grid space isn't already occupied, make it the correct biome and add its neighbors. otherwise just ignore it and move on
                        Tgrid[curRow][curCol] = biomesList[i]; // make it the correct biome
                        if (curRow - 1 >= 0)
                        {
                            GQenqueue(&BQlist[i], curRow - 1, curCol, precedence++, NULL);
                        } // check if space above seed is in bounds
                        if (curRow + 1 <= 20)
                        {
                            GQenqueue(&BQlist[i], curRow + 1, curCol, precedence++, NULL);
                        } // checks if space below seed is in bounds
                        if (curRow - 1 >= 0)
                        {
                            GQenqueue(&BQlist[i], curRow, curCol - 1, precedence++, NULL);
                        } // checks if space to left of seed is in bounds
                        if (curRow + 1 <= 79)
                        {
                            GQenqueue(&BQlist[i], curRow, curCol + 1, precedence++, NULL);
                        } // checks if space to right of seed is in bounds
                    }
                }
                //}
            }
        }
        return 0;
    }

    int generateBoarder()
    {
        int k = 0;

        // making top boarder
        for (k = 0; k < 80; k++)
        {
            Tgrid[0][k] = TT_BOULDER;
        }
        // making right boarder
        for (k = 0; k < 21; k++)
        {
            Tgrid[k][79] = TT_BOULDER;
        }
        // making bottom boarder
        for (k = 0; k < 80; k++)
        {
            Tgrid[20][k] = TT_BOULDER;
        }
        // making left boarder
        for (k = 0; k < 21; k++)
        {
            Tgrid[k][0] = TT_BOULDER;
        }

        return 0; // successfull
    }

    int generatePaths()
    {
        // creating the exits on the map and a path 1 in from the boarder
        if (gateN != -1)
        {
            Tgrid[0][gateN] = TT_GATE;
            Tgrid[1][gateN] = TT_PATH;
        }
        else
        {
            gateN = 40; // if you are at the top of the map, the path will lead to the middle of the top but not break the wall
        }
        if (gateS != -1)
        {
            Tgrid[20][gateS] = TT_GATE;
            Tgrid[19][gateS] = TT_PATH;
        }
        else
        {
            gateS = 40; // if you are at the bottom of the map, the path will lead to the middle of the bottom but not break the wall
        }
        if (gateW != -1)
        {
            Tgrid[gateW][0] = TT_GATE;
            Tgrid[gateW][1] = TT_PATH;
        }
        else
        {
            gateW = 10; // if you are at the left  of the map, the path will lead to the middle of the west but not break the wall
        }
        if (gateE != -1)
        {
            Tgrid[gateE][79] = TT_GATE;
            Tgrid[gateE][78] = TT_PATH;
        }
        else
        {
            gateE = 10; // if you are at the far right  of the map, the path will lead to middle of east but not break wall
        }

        PathProb pp;
        int curRow;
        int curCol;
        int randDir;

        // make connection from N->S
        curRow = 1;
        curCol = gateN;
        initPathProb(&pp, 1, gateN, 19, gateS); // going from col 1 to col 19 (instead 0 to 20) to avoid breaking boundry anywhere besides the exit cell
        while ((pp.numV + pp.numH) > 0)
        {
            randDir = rand() % (pp.numV + pp.numH);
            if (randDir < pp.numH)
            {
                // go H one unit in correct direction & decrease the H count
                curCol += pp.dirH;
                pp.numH--;
            }
            else
            {
                // go V one unit in correct direction & decrease V count
                curRow += pp.dirV;
                pp.numV--;
            }
            // update the caracter at the given space. Make it a bridge if it is water, otherwise make it a path
            if (Tgrid[curRow][curCol] != TT_WATER)
            {
                Tgrid[curRow][curCol] = TT_PATH;
            }
            else
            {
                Tgrid[curRow][curCol] = TT_BRIDGE;
            }
        }

        // make connection from W->E
        initPathProb(&pp, gateW, 1, gateE, 78); // going from 1 to 78 (instead of 0 to 79) to avoid breaking boundry anywhere besides the exit cell.
        curRow = gateW;
        curCol = 1;
        while ((pp.numV + pp.numH) > 0)
        {
            randDir = rand() % (pp.numV + pp.numH);
            if (randDir < pp.numH)
            {
                // go H one unit in correct direction & decrease the H count
                curCol += pp.dirH;
                pp.numH--;
            }
            else
            {
                // go V one unit in correct direction & decrease V count
                curRow += pp.dirV;
                pp.numV--;
            }
            // update the caracter at the given space. Make it a bridge if it is water, otherwise make it a path
            if (Tgrid[curRow][curCol] != TT_WATER)
            {
                Tgrid[curRow][curCol] = TT_PATH;
            }
            else
            {
                Tgrid[curRow][curCol] = TT_BRIDGE;
            }
        }
        return 0;
    }

    int generateBuildings()
    {
        int randRow;
        int randCol;
        int distance = abs(globalRow - 200) + abs(globalCol - 200); // Manhattan distance from center of board
        int randSpawnNum = rand() % 400;                            // generates a random number from 0 to 399


        if (randSpawnNum >= distance)
        {
            // TO PLACE THE POKEMART
            while (1)
            {
                randRow = rand() % 14 + 3;
                randCol = rand() % 73 + 3;

                // check if the pokeMart can be placed without overlapping a road or bridge but still touching a road. (placed up-right of path)
                if (Tgrid[randRow][randCol] == TT_PATH && Tgrid[randRow][randCol + 1] != TT_PATH && Tgrid[randRow][randCol + 2] != TT_PATH && Tgrid[randRow - 1][randCol + 1] != TT_PATH && Tgrid[randRow - 1][randCol + 2] != TT_PATH &&
                    Tgrid[randRow][randCol + 1] != TT_BRIDGE && Tgrid[randRow][randCol + 2] != TT_BRIDGE && Tgrid[randRow - 1][randCol + 1] != TT_BRIDGE && Tgrid[randRow - 1][randCol + 2] != TT_BRIDGE)
                {
                    Tgrid[randRow][randCol + 1] = TT_PMART;
                    Tgrid[randRow][randCol + 2] = TT_PMART;
                    Tgrid[randRow - 1][randCol + 1] = TT_PMART;
                    Tgrid[randRow - 1][randCol + 2] = TT_PMART;
                    break;
                }

                // check if the pokeMart can be placed without overlapping a road or bridge but still touching a road. (placed up-left of path)
                if (Tgrid[randRow][randCol] == TT_PATH && Tgrid[randRow][randCol - 1] != TT_PATH && Tgrid[randRow][randCol - 2] != TT_PATH && Tgrid[randRow - 1][randCol - 1] != TT_PATH && Tgrid[randRow - 1][randCol - 2] != TT_PATH &&
                    Tgrid[randRow][randCol - 1] != TT_BRIDGE && Tgrid[randRow][randCol - 2] != TT_BRIDGE && Tgrid[randRow - 1][randCol - 1] != TT_BRIDGE && Tgrid[randRow - 1][randCol - 2] != TT_BRIDGE)
                {

                    Tgrid[randRow][randCol - 1] = TT_PMART;
                    Tgrid[randRow][randCol - 2] = TT_PMART;
                    Tgrid[randRow - 1][randCol - 1] = TT_PMART;
                    Tgrid[randRow - 1][randCol - 2] = TT_PMART;
                    break;
                }
            }
        }

        randSpawnNum = rand() % 400; // generates a random number from 0 to 399
        if (randSpawnNum >= distance)
        {
            // TO PLACE THE POKECENTER
            while (1)
            {
                randRow = rand() % 14 + 3;
                randCol = rand() % 73 + 3;
                // the following if statement has to make sure it doesn't overlap a path OR a pokemart that has already been placed
                if (Tgrid[randRow][randCol] == TT_PATH &&
                    Tgrid[randRow][randCol + 1] != TT_PATH && Tgrid[randRow][randCol + 1] != TT_PMART && Tgrid[randRow][randCol + 1] != TT_BRIDGE &&
                    Tgrid[randRow][randCol + 2] != TT_PATH && Tgrid[randRow][randCol + 2] != TT_PMART && Tgrid[randRow][randCol + 2] != TT_BRIDGE &&
                    Tgrid[randRow - 1][randCol + 1] != TT_PATH && Tgrid[randRow - 1][randCol + 1] != TT_PMART && Tgrid[randRow - 1][randCol + 1] != TT_BRIDGE &&
                    Tgrid[randRow - 1][randCol + 2] != TT_PATH && Tgrid[randRow - 1][randCol + 2] != TT_PMART && Tgrid[randRow - 1][randCol + 2] != TT_BRIDGE)
                {

                    Tgrid[randRow][randCol + 1] = TT_PCENTER;
                    Tgrid[randRow][randCol + 2] = TT_PCENTER;
                    Tgrid[randRow - 1][randCol + 1] = TT_PCENTER;
                    Tgrid[randRow - 1][randCol + 2] = TT_PCENTER;
                    break;
                }
                if (Tgrid[randRow][randCol] == TT_PATH &&
                    Tgrid[randRow][randCol - 1] != TT_PATH && Tgrid[randRow][randCol - 1] != TT_PMART && Tgrid[randRow][randCol - 1] != TT_BRIDGE &&
                    Tgrid[randRow][randCol - 2] != TT_PATH && Tgrid[randRow][randCol - 2] != TT_PMART && Tgrid[randRow][randCol - 2] != TT_BRIDGE &&
                    Tgrid[randRow - 1][randCol - 1] != TT_PATH && Tgrid[randRow - 1][randCol - 1] != TT_PMART && Tgrid[randRow - 1][randCol - 1] != TT_BRIDGE &&
                    Tgrid[randRow - 1][randCol - 2] != TT_PATH && Tgrid[randRow - 1][randCol - 2] != TT_PMART && Tgrid[randRow - 1][randCol - 2] != TT_BRIDGE)
                {

                    Tgrid[randRow][randCol - 1] = TT_PCENTER;
                    Tgrid[randRow][randCol - 2] = TT_PCENTER;
                    Tgrid[randRow - 1][randCol - 1] = TT_PCENTER;
                    Tgrid[randRow - 1][randCol - 2] = TT_PCENTER;
                    break;
                }
            }
        }

                for (int i = 0; i < 9; i++)
        {
            if (gyms[i]->w_row == w_row && gyms[i]->w_col == w_col)
            {
                while (1)
                {
                    randRow = rand() % 14 + 3;
                    randCol = rand() % 73 + 3;
                    int terrain = gyms[i]->terrainType;

                    // check if the pokeMart can be placed without overlapping a road or bridge but still touching a road. (placed up-right of path)
                    if (Tgrid[randRow][randCol] == TT_PATH && Tgrid[randRow][randCol + 1] != TT_PATH && Tgrid[randRow][randCol + 2] != TT_PATH && Tgrid[randRow - 1][randCol + 1] != TT_PATH && Tgrid[randRow - 1][randCol + 2] != TT_PATH &&
                        Tgrid[randRow][randCol + 1] != TT_BRIDGE && Tgrid[randRow][randCol + 2] != TT_BRIDGE && Tgrid[randRow - 1][randCol + 1] != TT_BRIDGE && Tgrid[randRow - 1][randCol + 2] != TT_BRIDGE)
                    {
                        Tgrid[randRow][randCol + 1] = terrain;
                        Tgrid[randRow][randCol + 2] = terrain;
                        Tgrid[randRow - 1][randCol + 1] = terrain;
                        Tgrid[randRow - 1][randCol + 2] = terrain;
                        break;
                    }

                    // check if the pokeMart can be placed without overlapping a road or bridge but still touching a road. (placed up-left of path)
                    if (Tgrid[randRow][randCol] == TT_PATH && Tgrid[randRow][randCol - 1] != TT_PATH && Tgrid[randRow][randCol - 2] != TT_PATH && Tgrid[randRow - 1][randCol - 1] != TT_PATH && Tgrid[randRow - 1][randCol - 2] != TT_PATH &&
                        Tgrid[randRow][randCol - 1] != TT_BRIDGE && Tgrid[randRow][randCol - 2] != TT_BRIDGE && Tgrid[randRow - 1][randCol - 1] != TT_BRIDGE && Tgrid[randRow - 1][randCol - 2] != TT_BRIDGE)
                    {

                        Tgrid[randRow][randCol - 1] = terrain;
                        Tgrid[randRow][randCol - 2] = terrain;
                        Tgrid[randRow - 1][randCol - 1] = terrain;
                        Tgrid[randRow - 1][randCol - 2] = terrain;
                        break;
                    }
                }
                break;
            }
        }

        return 0;
    }

    int placeRandomTerrain()
    {
        srand(time(NULL));
        int randNum; // variable to determine whether you should place a random object (every number is 1% chance)
        char curObj; // variable to store what biome the cell you are think of placing an object is
        for (int i = 1; i < 20; i++)
        { // iterates through all spaces on the board (except for boarder)
            for (int j = 1; j < 79; j++)
            {
                randNum = rand() % 150; // generate random number from 0-99 that will give probability of placing object
                curObj = Tgrid[i][j];   // getting what object is at the current location
                if ((randNum == 13 || randNum == 7) && (curObj == TT_SGRASS || curObj == TT_TGRASS))
                { // if number is 13, place a TT_TREE (~1% chance). only allowed to be placed on short or tall grass
                    Tgrid[i][j] = TT_TREE;
                }
                if (randNum == 69 && (curObj == TT_FOREST || curObj == TT_SGRASS || curObj == TT_TGRASS))
                { // if number is 69, place a rock (~1% chance). only allowed to be placed in TT_FOREST, short and tall grass
                    Tgrid[i][j] = TT_BOULDER;
                }
            }
        }

        return 0;
    }

    int printBoard()
    {
        for (int i = 0; i < 21; i++)
        {
            for (int j = 0; j < 80; j++)
            {
                if (charGrid[i][j] != NULL)
                { // if there is a character at the given spot, print the character
                    int curChar = charGrid[i][j]->type;
                    attron(COLOR_PAIR(COLOR_RED));
                    if (curChar < 0 || curChar >= CT_OTHER)
                    {
                        mvprintw(0, 0, "ERROR: invalid character type at row:%d col:%d)", i, j);
                        break;
                    }
                    else
                    {
                        if (curChar == CT_PLAYER)
                        {
                            attron(A_BOLD);
                            mvprintw(i + 1, j, "%c", CT_to_Char(curChar));
                            attroff(A_BOLD);
                        }
                        else
                        {
                            mvprintw(i + 1, j, "%c", CT_to_Char(curChar));
                        }
                    }
                    attroff(COLOR_PAIR(COLOR_RED));
                }
                else
                { // else print the terrain
                    int curTerrain = Tgrid[i][j];
                    switch (curTerrain)
                    {
                    case TT_PEWTER:
                        attron(COLOR_PAIR(COLOR_WHITE));
                        attron(A_BOLD);
                        attron(A_BLINK);
                        mvprintw(i + 1, j, "P");
                        attroff(A_BLINK);
                        attroff(A_BOLD);
                        attroff(COLOR_PAIR(COLOR_WHITE));
                        break;
                    case TT_CERULEAN:
                        attron(COLOR_PAIR(COLOR_WHITE));
                        attron(A_BOLD);
                        attron(A_BLINK);
                        mvprintw(i + 1, j, "C");
                        attroff(A_BLINK);
                        attroff(A_BOLD);
                        attroff(COLOR_PAIR(COLOR_WHITE));
                        break;
                    case TT_VERMILION:
                        attron(COLOR_PAIR(COLOR_WHITE));
                        attron(A_BOLD);
                        attron(A_BLINK);
                        mvprintw(i + 1, j, "V");
                        attroff(A_BLINK);
                        attroff(A_BOLD);
                        attroff(COLOR_PAIR(COLOR_WHITE));
                        break;
                    case TT_CELADON:
                        attron(COLOR_PAIR(COLOR_WHITE));
                        attron(A_BOLD);
                        attron(A_BLINK);
                        mvprintw(i + 1, j, "C");
                        attroff(A_BLINK);
                        attroff(A_BOLD);
                        attroff(COLOR_PAIR(COLOR_WHITE));
                        break;
                    case TT_FUSHSIA:
                        attron(COLOR_PAIR(COLOR_WHITE));
                        attron(A_BOLD);
                        attron(A_BLINK);
                        mvprintw(i + 1, j, "F");
                        attroff(A_BLINK);
                        attroff(A_BOLD);
                        attroff(COLOR_PAIR(COLOR_WHITE));
                        break;
                    case TT_SAFFRON:
                        attron(COLOR_PAIR(COLOR_WHITE));
                        attron(A_BOLD);
                        attron(A_BLINK);
                        mvprintw(i + 1, j, "S");
                        attroff(A_BLINK);
                        attroff(A_BOLD);
                        attroff(COLOR_PAIR(COLOR_WHITE));
                        break;
                    case TT_CINNABAR:
                        attron(COLOR_PAIR(COLOR_WHITE));
                        attron(A_BOLD);
                        attron(A_BLINK);
                        mvprintw(i + 1, j, "C");
                        attroff(A_BLINK);
                        attroff(A_BOLD);
                        attroff(COLOR_PAIR(COLOR_WHITE));
                        break;
                    case TT_VIRIDIAN:
                        attron(COLOR_PAIR(COLOR_WHITE));
                        attron(A_BOLD);
                        attron(A_BLINK);
                        mvprintw(i + 1, j, "V");
                        attroff(A_BLINK);
                        attroff(A_BOLD);
                        attroff(COLOR_PAIR(COLOR_WHITE));
                        break;
                    case TT_ELITEFOUR:
                        attron(COLOR_PAIR(COLOR_WHITE));
                        attron(A_BOLD);
                        attron(A_BLINK);
                        mvprintw(i + 1, j, "E");
                        attroff(A_BLINK);
                        attroff(A_BOLD);
                        attroff(COLOR_PAIR(COLOR_WHITE));
                        break;
                    case TT_PCENTER:
                        attron(COLOR_PAIR(COLOR_MAGENTA));
                        mvprintw(i + 1, j, "C");
                        attroff(COLOR_PAIR(COLOR_MAGENTA));
                        break;
                    case TT_PMART:
                        attron(COLOR_PAIR(COLOR_MAGENTA));
                        mvprintw(i + 1, j, "P");
                        attroff(COLOR_PAIR(COLOR_MAGENTA));
                        break;
                    case TT_SGRASS:
                        attron(COLOR_PAIR(COLOR_GREEN));
                        mvprintw(i + 1, j, ".");
                        attroff(COLOR_PAIR(COLOR_GREEN));
                        break;
                    case TT_TGRASS:
                        attron(A_BOLD);
                        attron(COLOR_PAIR(COLOR_GREEN));
                        mvprintw(i + 1, j, ":");
                        attroff(COLOR_PAIR(COLOR_GREEN));
                        attroff(A_BOLD);
                        break;
                    case TT_WATER:
                        attron(COLOR_PAIR(COLOR_BLUE));
                        mvprintw(i + 1, j, "~");
                        attroff(COLOR_PAIR(COLOR_BLUE));
                        break;
                    case TT_PATH: // paths are '#' so this continues
                    case TT_GATE: // gates are '#' so this continues
                    case TT_BRIDGE:
                        attron(COLOR_PAIR(COLOR_YELLOW));
                        mvprintw(i + 1, j, "#");
                        attroff(COLOR_PAIR(COLOR_YELLOW));
                        break;
                    case TT_TREE: // trees are '^' so this continues
                    case TT_FOREST:
                        attron(COLOR_PAIR(COLOR_GREEN));
                        mvprintw(i + 1, j, "^");
                        attroff(COLOR_PAIR(COLOR_GREEN));
                        break;
                    case TT_BOULDER: // boulders are '%' so this continues
                    case TT_MOUNTAIN:
                        mvprintw(i + 1, j, "%%");
                        break;
                    case TT_NO_TERRAIN:
                        mvprintw(i + 1, j, "0");
                        break;
                    default:
                        mvprintw(0, 0, "ERROR: invalid terrain type");
                        break;
                    }
                }
                // printf(" ");        puts a space between each of the characters
            }
            // printf("\n");  no need for this with ncurses
        }
        // mvprintw(22,0,"0         1         2         3         4         5         6         7         8"); //DELETETHIS
        refresh();
        return 0;
    }

    ~Map(){};
};

/////////////////////////////////////////////////////////////////////POKEDEX/////////////////////////////////////////////////////////////////////////

class Pokemon
{
public:
    int id;
    std::string identifier;
    int species_id;
    int height;
    int weight;
    int base_experience;
    int order;
    int is_default;
};

class Moves
{
public:
    int id;
    std::string identifier;
    int generation_id;
    int type_id;
    int power;
    int pp;
    int accuracy;
    int priority;
    int target_id;
    int damage_class_id;
    int effect_id;
    int effect_chance;
    int contest_type_id;
    int contest_effect_id;
    int super_contest_effect_id;
};

class Pokemon_moves
{
public:
    int pokemon_id;
    int version_group_id;
    int move_id;
    int pokemon_move_method_id;
    int level;
    int order;
};

class Pokemon_species
{
public:
    int id;
    std::string identifier;
    int generation_id;
    int evolves_from_species_id;
    int evolution_chain_id;
    int color_id;
    int shape_id;
    int habitat_id;
    int gender_rate;
    int capture_rate;
    int base_happiness;
    int is_baby;
    int hatch_counter;
    int has_gender_differences;
    int growth_rate_id;
    int forms_switchable;
    int is_legendary;
    int is_mythical;
    int order;
    int conquest_order;
};

class Experience
{
public:
    int growth_rate_id;
    int level;
    int experience;
};

class Type_names
{
public:
    int type_id;
    int local_language_id;
    std::string name;
};

class Pokemon_stats
{
public:
    int pokemon_id;
    int stat_id;
    int base_stat;
    int effort;
};

class Stats
{
public:
    int id;
    int damage_class_id;
    std::string identifier;
    int is_battle_only;
    int game_index;
};

class Pokemon_types
{
public:
    int pokemon_id;
    int type_id;
    int slot;
};

std::vector<std::string> parseLine(std::string curLine)
{
    std::vector<std::string> parsedInfo;
    std::stringstream curStream(curLine);
    std::string curData;

    while (getline(curStream, curData, ','))
    {
        if (curData.length() == 0)
        {
            parsedInfo.push_back("42069666");
        }
        else
        {
            parsedInfo.push_back(curData);
        }
    }
    return parsedInfo;
}

std::ifstream openCSV(const char *filename)
{
    bool fileFound = false;
    std::string home = std::string(getenv("HOME")) + ".poke327/";
    std::vector<std::string> file_paths = {"/share/cs327/pokedex/pokedex/data/csv/", home + "pokedex/pokedex/data/csv/", "/home/manasmathur/VSWorkspace/pokedex/pokedex/data/csv/"};
    std::ifstream file;
    for (auto path : file_paths)
    {
        file.open(path + filename);
        if (file.is_open())
        {
            fileFound = true;
            break;
        }
        else
        {
            file.clear();
        }
    }

    if (!fileFound)
    {
        std::cout << "FILE NOT FOUND" << std::endl;
        throw "FILE NOT FOUND";
    }
    return file;
}

std::vector<Pokemon> parsePokemon()
{
    std::ifstream file = openCSV("pokemon.csv");
    std::vector<Pokemon> returnVector; // make a vector of pokemon objects
    std::string curLine;
    std::string parsedLine;
    getline(file, curLine); // ignore the first line of code
    // iterate through all lines of the file (starting at line 2)
    while (getline(file, curLine))
    {
        Pokemon curPokemon;
        std::vector<std::string> parsedInfo = parseLine(curLine);

        curPokemon.id = stoi(parsedInfo[0]);
        curPokemon.identifier = parsedInfo[1];
        curPokemon.species_id = stoi(parsedInfo[2]);
        curPokemon.height = stoi(parsedInfo[3]);
        curPokemon.weight = stoi(parsedInfo[4]);
        curPokemon.base_experience = stoi(parsedInfo[5]);
        curPokemon.order = stoi(parsedInfo[6]);
        curPokemon.is_default = stoi(parsedInfo[7]);

        returnVector.push_back(curPokemon);
    }
    return returnVector;
}

std::vector<Moves> parseMoves()
{
    std::ifstream file = openCSV("moves.csv");
    std::vector<Moves> returnVector; // make a vector of pokemon objects
    std::string curLine;
    std::string parsedLine;
    getline(file, curLine); // ignore the first line of code
    // iterate through all lines of the file (starting at line 2)
    while (getline(file, curLine))
    {
        Moves curMove;
        std::vector<std::string> parsedInfo = parseLine(curLine);

        curMove.id = stoi(parsedInfo[0]);
        curMove.identifier = parsedInfo[1];
        curMove.generation_id = stoi(parsedInfo[2]);
        curMove.type_id = stoi(parsedInfo[3]);
        curMove.power = stoi(parsedInfo[4]);
        curMove.pp = stoi(parsedInfo[5]);
        curMove.accuracy = stoi(parsedInfo[6]);
        curMove.priority = stoi(parsedInfo[7]);
        curMove.target_id = stoi(parsedInfo[8]);
        curMove.damage_class_id = stoi(parsedInfo[9]);
        curMove.effect_id = stoi(parsedInfo[10]);
        curMove.effect_chance = stoi(parsedInfo[11]);
        curMove.contest_type_id = stoi(parsedInfo[12]);
        curMove.contest_effect_id = stoi(parsedInfo[13]);

        // account for the last index being empty
        if (parsedInfo.size() < 15)
        {
            curMove.super_contest_effect_id = MAX;
        }
        else
        {
            curMove.super_contest_effect_id = stoi(parsedInfo[14]);
        }

        returnVector.push_back(curMove);
    }
    return returnVector;
}

std::vector<Pokemon_moves> parsePokemonMoves()
{
    std::ifstream file = openCSV("pokemon_moves.csv");
    std::vector<Pokemon_moves> returnVector; // make a vector of pokemon objects
    std::string curLine;
    std::string parsedLine;
    getline(file, curLine); // ignore the first line of code
    // iterate through all lines of the file (starting at line 2)
    while (getline(file, curLine))
    {
        Pokemon_moves curPokeMove;
        std::vector<std::string> parsedInfo = parseLine(curLine);

        curPokeMove.pokemon_id = stoi(parsedInfo[0]);
        curPokeMove.version_group_id = stoi(parsedInfo[1]);
        curPokeMove.move_id = stoi(parsedInfo[2]);
        curPokeMove.pokemon_move_method_id = stoi(parsedInfo[3]);
        curPokeMove.level = stoi(parsedInfo[4]);
        // account for if the last element is empty
        if (parsedInfo.size() < 6)
        {
            curPokeMove.order = MAX;
        }
        else
        {
            curPokeMove.order = stoi(parsedInfo[5]);
        }

        returnVector.push_back(curPokeMove);
    }
    return returnVector;
}

std::vector<Pokemon_species> parsePokemonSpecies()
{
    std::ifstream file = openCSV("pokemon_species.csv");
    std::vector<Pokemon_species> returnVector; // make a vector of pokemon objects
    std::string curLine;
    std::string parsedLine;
    getline(file, curLine); // ignore the first line of code
    // iterate through all lines of the file (starting at line 2)
    while (getline(file, curLine))
    {
        Pokemon_species curPokeSpecies;
        std::vector<std::string> parsedInfo = parseLine(curLine);

        curPokeSpecies.id = stoi(parsedInfo[0]);
        curPokeSpecies.identifier = parsedInfo[1];
        curPokeSpecies.generation_id = stoi(parsedInfo[2]);
        curPokeSpecies.evolves_from_species_id = stoi(parsedInfo[3]);
        curPokeSpecies.evolution_chain_id = stoi(parsedInfo[4]);
        curPokeSpecies.color_id = stoi(parsedInfo[5]);
        curPokeSpecies.shape_id = stoi(parsedInfo[6]);
        curPokeSpecies.habitat_id = stoi(parsedInfo[7]);
        curPokeSpecies.gender_rate = stoi(parsedInfo[8]);
        curPokeSpecies.capture_rate = stoi(parsedInfo[9]);
        curPokeSpecies.base_happiness = stoi(parsedInfo[10]);
        curPokeSpecies.is_baby = stoi(parsedInfo[11]);
        curPokeSpecies.hatch_counter = stoi(parsedInfo[12]);
        curPokeSpecies.has_gender_differences = stoi(parsedInfo[13]);
        curPokeSpecies.growth_rate_id = stoi(parsedInfo[14]);
        curPokeSpecies.forms_switchable = stoi(parsedInfo[15]);
        curPokeSpecies.is_legendary = stoi(parsedInfo[16]);
        curPokeSpecies.is_mythical = stoi(parsedInfo[17]);
        curPokeSpecies.order = stoi(parsedInfo[18]);
        // account for the last element being empty
        if (parsedInfo.size() < 20)
        {
            curPokeSpecies.conquest_order = MAX;
        }
        else
        {
            curPokeSpecies.conquest_order = stoi(parsedInfo[19]);
        }

        returnVector.push_back(curPokeSpecies);
    }
    return returnVector;
}

std::vector<Experience> parseExperience()
{
    std::ifstream file = openCSV("experience.csv");
    std::vector<Experience> returnVector; // make a vector of pokemon objects
    std::string curLine;
    std::string parsedLine;
    getline(file, curLine); // ignore the first line of code
    // iterate through all lines of the file (starting at line 2)
    while (getline(file, curLine))
    {
        Experience curExperience;
        std::vector<std::string> parsedInfo = parseLine(curLine);

        curExperience.growth_rate_id = stoi(parsedInfo[0]);
        curExperience.level = stoi(parsedInfo[1]);
        curExperience.experience = stoi(parsedInfo[2]);

        returnVector.push_back(curExperience);
    }
    return returnVector;
}

std::vector<Type_names> parseTypeNames()
{
    std::ifstream file = openCSV("type_names.csv");
    std::vector<Type_names> returnVector; // make a vector of pokemon objects
    std::string curLine;
    std::string parsedLine;
    getline(file, curLine); // ignore the first line of code
    // iterate through all lines of the file (starting at line 2)
    while (getline(file, curLine))
    {
        Type_names curTypeNames;
        std::vector<std::string> parsedInfo = parseLine(curLine);

        curTypeNames.type_id = stoi(parsedInfo[0]);
        curTypeNames.local_language_id = stoi(parsedInfo[1]);
        curTypeNames.name = parsedInfo[2];

        returnVector.push_back(curTypeNames);
    }
    return returnVector;
}

std::vector<Pokemon_stats> parsePokemonStats()
{
    std::ifstream file = openCSV("pokemon_stats.csv");
    std::vector<Pokemon_stats> returnVector; // make a vector of pokemon objects
    std::string curLine;
    std::string parsedLine;
    getline(file, curLine); // ignore the first line of code
    // iterate through all lines of the file (starting at line 2)
    while (getline(file, curLine))
    {
        Pokemon_stats curPokeStats;
        std::vector<std::string> parsedInfo = parseLine(curLine);

        curPokeStats.pokemon_id = stoi(parsedInfo[0]);
        curPokeStats.stat_id = stoi(parsedInfo[1]);
        curPokeStats.base_stat = stoi(parsedInfo[2]);
        curPokeStats.effort = stoi(parsedInfo[3]);

        returnVector.push_back(curPokeStats);
    }
    return returnVector;
}

std::vector<Stats> parseStats()
{
    std::ifstream file = openCSV("stats.csv");
    std::vector<Stats> returnVector; // make a vector of pokemon objects
    std::string curLine;
    std::string parsedLine;
    getline(file, curLine); // ignore the first line of code
    // iterate through all lines of the file (starting at line 2)
    while (getline(file, curLine))
    {
        Stats curStats;
        std::vector<std::string> parsedInfo = parseLine(curLine);

        curStats.id = stoi(parsedInfo[0]);
        curStats.damage_class_id = stoi(parsedInfo[1]);
        curStats.identifier = parsedInfo[2];
        curStats.is_battle_only = stoi(parsedInfo[3]);
        if (parsedInfo.size() < 5)
        {
            curStats.game_index = MAX;
        }
        else
        {
            curStats.game_index = stoi(parsedInfo[4]);
        }

        returnVector.push_back(curStats);
    }
    return returnVector;
}

std::vector<Pokemon_types> parsePokemonTypes()
{
    std::ifstream file = openCSV("pokemon_types.csv");
    std::vector<Pokemon_types> returnVector; // make a vector of pokemon objects
    std::string curLine;
    std::string parsedLine;
    getline(file, curLine); // ignore the first line of code
    // iterate through all lines of the file (starting at line 2)
    while (getline(file, curLine))
    {
        Pokemon_types curPokeTypes;
        std::vector<std::string> parsedInfo = parseLine(curLine);

        curPokeTypes.pokemon_id = stoi(parsedInfo[0]);
        curPokeTypes.type_id = stoi(parsedInfo[1]);
        curPokeTypes.slot = stoi(parsedInfo[2]);

        returnVector.push_back(curPokeTypes);
    }
    return returnVector;
}

std::vector<Pokemon> pokemonVector;
std::vector<Moves> movesVector;
std::vector<Pokemon_moves> pokeMovesVector;
std::vector<Pokemon_species> pokeSpeciesVector;
std::vector<Experience> experienceVector;
std::vector<Type_names> typeNamesVector;
std::vector<Pokemon_stats> pokeStatsVector;
std::vector<Stats> statsVector;
std::vector<Pokemon_types> pokeTypesVector;

////////////////////////////////////////////////////////////////POKEMON GENERATION FUNCTIONS///////////////////////////////////////////////////////////////////

class personalPokemon
{
public:
    std::string identifier;

    int level;
    int totalExp;

    int id;
    int species_id;

    int max_HP;
    int HP;

    int attack;

    int defense;

    int speed;
    int base_speed;

    int special_attack;

    int special_defense;

    int accuracy;

    int evasion;

    int gender; // 1 If male, 0 if femals
    int shiny;  // 1 If shiny, 0 if not
    int base_experience;

    std::vector<Moves> moves;
    std::vector<Pokemon_stats> pokeStatsTemp;
    std::vector<int> pokeTypes;

    /*
        CURRENT ISSUES:
            1. Manhatten distance could be off?
    */

    // constructor for PC Pokemon
    // Outside of constructor, will generate a random number index which will correlate to a specific pokemon, given that index, the constructor will make a pokemon object based off the index
    personalPokemon(int index)
    {
        id = pokemonVector[index].id;
        species_id = pokemonVector[index].species_id;
        identifier = pokemonVector[index].identifier;
        base_experience = pokemonVector[index].base_experience;
        level = 1;
        totalExp = level ^ 3;

        // Generates a single move for the pokemon
        std::vector<Pokemon_moves> pokeMovesTemp;
        bool breakWhileLoop = false;
        bool breakForLoop = false;

        do
        {
            for (Pokemon_moves pokeMove : pokeMovesVector)
            {
                if (pokeMove.pokemon_id == species_id)
                {
                    breakForLoop = true;
                }

                if (pokeMove.pokemon_id == species_id && pokeMove.pokemon_move_method_id == 1 && pokeMove.level <= level)
                {
                    pokeMovesTemp.push_back(pokeMove);
                }

                if (breakForLoop == true && pokeMove.pokemon_id != species_id)
                {
                    break;
                }
            }

            if (pokeMovesVector.size() < 1)
            {
                level++;
                pokeMovesVector.clear();
            }
            else
            {
                breakWhileLoop = true;
            }
        } while (!breakWhileLoop);

        index = rand() % pokeMovesTemp.size();
        moves.push_back(movesVector[pokeMovesTemp[index].move_id - 1]);

        // Generate IVs

        breakForLoop = false;

        for (Pokemon_stats pokeStat : pokeStatsVector)
        {
            if (pokeStat.pokemon_id == id)
            {
                breakForLoop = true;
                pokeStatsTemp.push_back(pokeStat);
            }

            if (breakForLoop == true && pokeStat.pokemon_id != id)
            {
                break;
            }
        }

        // Generate types
        breakForLoop = false;

        for (Pokemon_types pokeType : pokeTypesVector)
        {

            if (pokeType.pokemon_id == id)
            {
                pokeTypes.push_back(pokeType.type_id);
                breakForLoop = true;
            }

            if (breakForLoop == true && pokeType.pokemon_id != id)
            {
                break;
            }
        }

        max_HP = HP = floor((((pokeStatsTemp[0].base_stat + (rand() % 15)) * 2) * level) / 100) + level + 10;
        attack = floor(((pokeStatsTemp[1].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        defense = floor(((pokeStatsTemp[2].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        special_attack = floor(((pokeStatsTemp[3].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        special_defense = floor(((pokeStatsTemp[4].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        speed = floor(((pokeStatsTemp[5].base_stat + (rand() % 15) * 2) * level) / 100) + 5;

        base_speed = pokeStatsTemp[5].base_stat;

        // Generate Shiny
        shiny = (rand() % 8191) == 0 ? 1 : 0;

        // Generate Gender
        gender = (rand() % 2) == 0 ? 1 : 0;
    }

    // constructor for gym leader pokemon
    personalPokemon(int y, int x, int type_id)
    {
        std::vector<Pokemon_types> posPokemon;
        for (Pokemon_types pokemonType : pokeTypesVector)
        {
            if (pokemonType.type_id == type_id)
            {
                posPokemon.push_back(pokemonType);
            }
        }
        int random = rand() % (int)posPokemon.size();
        Pokemon_types selectedPokemonType = posPokemon[random];

        for (Pokemon pokemon : pokemonVector)
        {
            if (selectedPokemonType.pokemon_id == pokemon.id)
            {
                id = pokemon.id;
                species_id = pokemon.species_id;
                identifier = pokemon.identifier;
                base_experience = pokemon.base_experience;
                break;
            }
        }

        int MD = ((abs(x - 200) + abs(200 - y)));

        if (MD <= 2)
        {
            MD = 1;
        }
        else if (MD == 400)
        {
            MD = 398;
        }
        // mvprintw(26, 0, "YO YO YO %d", MD);
        level = MD <= 200 ? (rand() % ((MD / 2) - 1)) + 1 : (rand() % (100 - ((MD - 200) / 2))) + ((MD - 200) / 2);

        totalExp = level ^ 3;
        // Generate Moves
        std::vector<Pokemon_moves> pokeMovesTemp;
        bool breakWhileLoop = false;
        bool breakForLoop = false;
        do
        {
            for (Pokemon_moves pokeMove : pokeMovesVector)
            {
                if (pokeMove.pokemon_id == species_id)
                {
                    breakForLoop = true;
                }

                if (pokeMove.pokemon_id == species_id && pokeMove.pokemon_move_method_id == 1 && pokeMove.level <= level)
                {
                    pokeMovesTemp.push_back(pokeMove);
                }

                if (breakForLoop == true && pokeMove.pokemon_id != species_id)
                {
                    break;
                }
            }

            if (pokeMovesVector.size() < 1)
            {
                level++;
                pokeMovesVector.clear();
            }
            else
            {
                breakWhileLoop = true;
            }
        } while (!breakWhileLoop);

        int index = rand() % pokeMovesTemp.size();
        moves.push_back(movesVector[pokeMovesTemp[index].move_id - 1]);
        pokeMovesTemp.erase(pokeMovesTemp.begin() + index);

        if (pokeMovesTemp.size() > 1)
        {
            index = rand() % pokeMovesTemp.size();
            moves.push_back(movesVector[pokeMovesTemp[index].move_id - 1]);
        }

        // Generate IVs

        breakForLoop = false;

        for (Pokemon_stats pokeStat : pokeStatsVector)
        {
            if (pokeStat.pokemon_id == id)
            {
                breakForLoop = true;
                pokeStatsTemp.push_back(pokeStat);
            }

            if (breakForLoop == true && pokeStat.pokemon_id != id)
            {
                break;
            }
        }

        // Generate types
        breakForLoop = false;

        for (Pokemon_types pokeType : pokeTypesVector)
        {

            if (pokeType.pokemon_id == id)
            {
                pokeTypes.push_back(pokeType.type_id);
                breakForLoop = true;
            }

            if (breakForLoop == true && pokeType.pokemon_id != id)
            {
                break;
            }
        }
        max_HP = HP = floor((((pokeStatsTemp[0].base_stat + (rand() % 15)) * 2) * level) / 100) + level + 10;
        attack = floor(((pokeStatsTemp[1].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        defense = floor(((pokeStatsTemp[2].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        special_attack = floor(((pokeStatsTemp[3].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        special_defense = floor(((pokeStatsTemp[4].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        speed = floor(((pokeStatsTemp[5].base_stat + (rand() % 15) * 2) * level) / 100) + 5;

        base_speed = pokeStatsTemp[5].base_stat;

        // Generate Shiny
        shiny = (rand() % 8191) == 0 ? 1 : 0;

        // Generate Gender
        gender = (rand() % 2) == 0 ? 1 : 0;

        // destructor
    }

    // constructor for NPC/Wild Pokemon
    personalPokemon(int y, int x)
    {
        int index = rand() % 1091;
        id = pokemonVector[index].id;
        species_id = pokemonVector[index].species_id;
        identifier = pokemonVector[index].identifier;
        base_experience = pokemonVector[index].base_experience;

        int MD = ((abs(x - 200) + abs(200 - y)));

        if (MD <= 2)
        {
            MD = 1;
        }
        else if (MD == 400)
        {
            MD = 398;
        }
        // mvprintw(26, 0, "YO YO YO %d", MD);
        level = MD <= 200 ? (rand() % ((MD / 2) - 1)) + 1 : (rand() % (100 - ((MD - 200) / 2))) + ((MD - 200) / 2);

        totalExp = level ^ 3;
        // Generate Moves
        std::vector<Pokemon_moves> pokeMovesTemp;
        bool breakWhileLoop = false;
        bool breakForLoop = false;
        do
        {
            for (Pokemon_moves pokeMove : pokeMovesVector)
            {
                if (pokeMove.pokemon_id == species_id)
                {
                    breakForLoop = true;
                }

                if (pokeMove.pokemon_id == species_id && pokeMove.pokemon_move_method_id == 1 && pokeMove.level <= level)
                {
                    pokeMovesTemp.push_back(pokeMove);
                }

                if (breakForLoop == true && pokeMove.pokemon_id != species_id)
                {
                    break;
                }
            }

            if (pokeMovesVector.size() < 1)
            {
                level++;
                pokeMovesVector.clear();
            }
            else
            {
                breakWhileLoop = true;
            }
        } while (!breakWhileLoop);

        index = rand() % pokeMovesTemp.size();
        moves.push_back(movesVector[pokeMovesTemp[index].move_id - 1]);
        pokeMovesTemp.erase(pokeMovesTemp.begin() + index);

        if (pokeMovesTemp.size() > 1)
        {
            index = rand() % pokeMovesTemp.size();
            moves.push_back(movesVector[pokeMovesTemp[index].move_id - 1]);
        }

        // Generate IVs

        breakForLoop = false;

        for (Pokemon_stats pokeStat : pokeStatsVector)
        {
            if (pokeStat.pokemon_id == id)
            {
                breakForLoop = true;
                pokeStatsTemp.push_back(pokeStat);
            }

            if (breakForLoop == true && pokeStat.pokemon_id != id)
            {
                break;
            }
        }

        // Generate types
        breakForLoop = false;

        for (Pokemon_types pokeType : pokeTypesVector)
        {

            if (pokeType.pokemon_id == id)
            {
                pokeTypes.push_back(pokeType.type_id);
                breakForLoop = true;
            }

            if (breakForLoop == true && pokeType.pokemon_id != id)
            {
                break;
            }
        }
        max_HP = HP = floor((((pokeStatsTemp[0].base_stat + (rand() % 15)) * 2) * level) / 100) + level + 10;
        attack = floor(((pokeStatsTemp[1].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        defense = floor(((pokeStatsTemp[2].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        special_attack = floor(((pokeStatsTemp[3].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        special_defense = floor(((pokeStatsTemp[4].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        speed = floor(((pokeStatsTemp[5].base_stat + (rand() % 15) * 2) * level) / 100) + 5;

        base_speed = pokeStatsTemp[5].base_stat;

        // Generate Shiny
        shiny = (rand() % 8191) == 0 ? 1 : 0;

        // Generate Gender
        gender = (rand() % 2) == 0 ? 1 : 0;

        // destructor
    }

    int gainExp(personalPokemon opponent, bool wild)
    {
        float ownershipType = wild ? 1 : 1.5;
        float baseOppExp = (float)opponent.base_experience;
        float lvlOpp = (float)opponent.level;

        int exp = floor(((baseOppExp * lvlOpp) / 7) * ownershipType);

        totalExp += exp;
        if (level < 99)
        {
            level = std::cbrt(totalExp);
            max_HP = HP = floor((((pokeStatsTemp[0].base_stat + (rand() % 15)) * 2) * level) / 100) + level + 10;
            attack = floor(((pokeStatsTemp[1].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
            defense = floor(((pokeStatsTemp[2].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
            special_attack = floor(((pokeStatsTemp[3].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
            special_defense = floor(((pokeStatsTemp[4].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
            speed = floor(((pokeStatsTemp[5].base_stat + (rand() % 15) * 2) * level) / 100) + 5;
        }

        return exp;
    };
};

class items
{
public:
    std::string name;
    int quantity;

    items(std::string name, int quantity)
    {
        this->name = name;
        this->quantity = quantity;
    }
};

int attack(personalPokemon *attacker, Moves attackerMove, personalPokemon *opponent, Moves opponentMove, bool properAttack)
{
    int power;
    if (attackerMove.power >= 1000)
    {
        power = 1;
    }
    else
    {
        power = attackerMove.power;
    }
    int attack = attacker->attack;
    int defense = attacker->defense;
    int crit = (floor(attacker->base_speed / 2) <= 255 && floor(attacker->base_speed / 2) >= 0) ? 1.5 : 1;
    int STAB = 1;
    int random = (rand() % (100 - 85) + 85);

    // Sets STAB
    if (attacker->pokeTypes[0] == attackerMove.type_id)
    {
        STAB = 1.5;
    }
    else if ((int)attacker->pokeTypes.size() == 2)
    {
        if (attacker->pokeTypes[1] == attackerMove.type_id)
        {
            STAB = 1.5;
        }
    }

    mvprintw(13, 13, "%s used %s! Press space to continue", attacker->identifier.c_str(), attackerMove.identifier.c_str());

    int usrKey;
    while (1)
    {
        usrKey = getch();
        if (usrKey == ' ')
        {
            break;
        }
    }

    int hit;

    if (rand() % 100 < attackerMove.accuracy)
    {
        mvprintw(13, 10, "*                                                         *");

        hit = (((2 * (float)attacker->level) / 5 + 2) * (float)power * ((float)attack / (float)defense) / 50 + 2) * (float)crit * ((float)random / 100) * (float)STAB * 1;
        opponent->HP -= floor(hit);
        mvprintw(13, 13, "%s hit and dealt %d damage! Press space to continue", attackerMove.identifier.c_str(), hit);
        while (1)
        {
            usrKey = getch();
            if (usrKey == ' ')
            {
                break;
            }
        }
    }
    else
    {
        mvprintw(13, 10, "*                                                          *");
        mvprintw(13, 13, "%s missed! Press space to continue", attackerMove.identifier.c_str());
        while (1)
        {
            usrKey = getch();
            if (usrKey == ' ')
            {
                break;
            }
        }
    }

    if (opponent->HP > 0 && properAttack)
    {

        if (opponentMove.power >= 1000)
        {
            power = 1;
        }
        else
        {
            power = opponentMove.power;
        }
        attack = opponent->attack;
        defense = opponent->defense;
        crit = (floor(opponent->base_speed / 2) <= 255 && floor(opponent->base_speed / 2) >= 0) ? 1.5 : 1;
        STAB = 1;
        random = (rand() % (100 - 85) + 85);

        if (attacker->pokeTypes[0] == attackerMove.type_id)
        {
            STAB = 1.5;
        }
        else if ((int)attacker->pokeTypes.size() == 2)
        {
            if (attacker->pokeTypes[1] == attackerMove.type_id)
            {
                STAB = 1.5;
            }
        }

        if (rand() % 100 < opponentMove.accuracy)
        {
            mvprintw(13, 10, "*                                                         *");

            hit = (((2 * (float)opponent->level) / 5 + 2) * (float)power * ((float)attack / (float)defense) / 50 + 2) * (float)crit * ((float)random / 100) * (float)STAB * 1;
            attacker->HP -= floor(hit);
            mvprintw(13, 13, "%s hit and dealt %d damage! Press space to continue", opponentMove.identifier.c_str(), hit);
            while (1)
            {
                usrKey = getch();
                if (usrKey == ' ')
                {
                    break;
                }
            }
        }
        else
        {
            mvprintw(13, 10, "*                                                          *");
            mvprintw(13, 13, "%s missed! Press space to continue", opponentMove.identifier.c_str());
            while (1)
            {
                usrKey = getch();
                if (usrKey == ' ')
                {
                    break;
                }
            }
        }
    }

    if (opponent->HP < 0)
    {
        opponent->HP = 0;
    }

    if (attacker->HP < 0)
    {
        attacker->HP = 0;
    }

    return 0;
}

int choosePokemon(std::vector<personalPokemon> party, int useCase) // 0 if choosing pokemon
                                                                   // 1 if healing pokemon
                                                                   // 2 if reviving pokemon
{

    int curPokemon;

    createPanel(12, 20, 10, 69);

    mvprintw(13, 13, "Choose a Pokemon: Press '0' to quit");

    int usrKey;
    while (1)
    {
        usrKey = getch();
        if (usrKey == ' ')
        {
            break;
        }
    }

    for (int i = 0; i < (int)party.size(); i++)
    {
        if (party[i].HP > 0)
        {
            // print in green
            attron(A_BOLD);
            attron(COLOR_PAIR(COLOR_GREEN));
            mvprintw(14 + i, 13, "%d. %s", i + 1, party[i].identifier.c_str());
            attroff(COLOR_PAIR(COLOR_GREEN));
            attroff(A_BOLD);
        }
        else
        {
            attron(COLOR_PAIR(COLOR_RED));
            mvprintw(14 + i, 13, "%d. %s", i + 1, party[i].identifier.c_str());
            attroff(COLOR_PAIR(COLOR_RED));
        }
    }

    int usrNum;
    while (1)
    {
        usrKey = getch();
        usrNum = usrKey - 48;

        if (usrNum == 0)
        {
            return -1;
        }

        if (usrNum > 0 && usrNum <= (int)party.size())
        {
            if (useCase == 0 && party[usrNum - 1].HP > 0)
            {
                curPokemon = usrNum - 1;
                break;
            }
            else if (useCase == 0 && party[usrNum - 1].HP <= 0)
            {
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "%s is unable to battle. Press space", party[usrNum - 1].identifier.c_str());

                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        break;
                    }
                }
            }

            if (useCase == 1 && party[usrNum - 1].HP < party[usrNum - 1].max_HP && party[usrNum - 1].HP > 0)
            {
                curPokemon = usrNum - 1;
                break;
            }
            else if (useCase == 1 && party[usrNum - 1].HP >= party[usrNum - 1].max_HP)
            {
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "%s is already at full health. Press Space", party[usrNum - 1].identifier.c_str());

                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        break;
                    }
                }
            }

            if (useCase == 2 && party[usrNum - 1].HP <= 0)
            {
                curPokemon = usrNum - 1;
                break;
            }
            else if (useCase == 2 && party[usrNum - 1].HP > 0)
            {
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "%s is already alive. Press space", party[usrNum - 1].identifier.c_str());

                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        break;
                    }
                }
            }
        }
    }

    return curPokemon;
}

class NPC : public Character
{
public:
    int direction;
    int isDefeated;
    std::string name;

    // : name(leaderName), isDefeated(0), row(2), col(rand() % 38 + 1), direction(NO_DIRECTION),
    NPC(int NPCtype, int type_id, std::string leaderName, int w_row, int w_col)
    {
        name = leaderName;
        type = NPCtype;
        row = 8;
        col = rand() % 38 + 1;
        direction = NO_DIRECTION;
        isDefeated = 0;

        do
        {
            personalPokemon npcPokemon(w_row, w_col, type_id);
            party.push_back(npcPokemon);
        } while ((rand() % 100 <= 60) && (party.size() < 6));
    }

    NPC(int NPCtype, Map *m, WorldMap *WM, PC *player)
    {

        int randRow;
        int randCol;
        do
        {
            // generate random (row,col)
            randRow = rand() % 19 + 1;                                                                            // generates a row within the border of the world (1 to 19)
            randCol = rand() % 78 + 1;                                                                            // generates a col within the border of the world (1 to 78)
        } while (calc_Cost(m->Tgrid[randRow][randCol], NPCtype) == MAX || m->charGrid[randRow][randCol] != NULL); // keep generating random points while the random spot has unsuitable terrain or already has a character
        type = NPCtype;
        row = randRow;
        col = randCol;

        switch (type)
        {
        case CT_PLAYER:
        {
            name = "Player";
            break;
        }
        case CT_HIKER:
        {
            name = "Hiker";
            break;
        }
        case CT_RIVAL:
        {
            name = "Rival";
            break;
        }
        case CT_SWIMMER:
        {
            name = "Swimmer";
            break;
        }
        case CT_PACER:
        {
            name = "Pacer";
            break;
        }
        case CT_WANDERER:
        {
            name = "Wanderer";
            break;
        }
        case CT_EXPLORER:
        {
            name = "Explorer";
            break;
        }
        case CT_SENTRY:
        {
            name = "Sentry";
            break;
        }
        case CT_OTHER:
        {
            name = "Other";
            break;
        }
        }

        // set direction
        if (NPCtype == CT_WANDERER || NPCtype == CT_PACER || NPCtype == CT_EXPLORER)
        {
            direction = rand() % 4; // generate a random direction 0=NORTH to 3= WEST
        }
        else
        {
            direction = NO_DIRECTION;
        }

        // set next position
        findNextPos(this, m, WM, player);

        isDefeated = 0;               // makes the NPCs initially not defeated
        m->charGrid[row][col] = this; // put new Character into the Map at the correct spot

        do
        {
            personalPokemon npcPokemon(w_row, w_col);
            party.push_back(npcPokemon);
        } while ((rand() % 100 <= 60) && (party.size() < 6));

        GQenqueue(&(WM->charQueue), row, col, nextCost, this); // enqueue the character
    }

    int moveOneDir(Map *m)
    {
        if (direction == NORTH)
        {
            this->updateNextCoords(row - 1, col);
            nextCost = calc_Cost(m->Tgrid[row - 1][col], type);
        }
        else if (direction == EAST)
        {
            this->updateNextCoords(row, col + 1);
            nextCost = calc_Cost(m->Tgrid[row][col + 1], type);
        }
        else if (direction == SOUTH)
        {
            this->updateNextCoords(row + 1, col);
            nextCost = calc_Cost(m->Tgrid[row + 1][col], type);
        }
        else if (direction == WEST)
        {
            this->updateNextCoords(row, col - 1);
            nextCost = calc_Cost(m->Tgrid[row][col - 1], type);
        }
        return 0;
    }

    ~NPC() {}
};

class PC : public Character
{
public:
    std::vector<items> bag;
    int numBadges;

    // constructor to initialize the PC at a random road coordinate on the map
    PC(WorldMap *WM, Map startMap, int start_w_row, int start_w_col)
    {
        type = CT_PLAYER;
        int randRow = (rand() % 19) + 1; // generates row from 1 to 20 so you don't spawn in gate
        int randCol = (rand() % 78) + 1; // generates col from 1 to 79 so you don't spawn in gate
        while (startMap.Tgrid[randRow][randCol] != TT_PATH)
        { // if you aren't on a road, get another random position
            randRow = (rand() % 19) + 1;
            randCol = (rand() % 78) + 1;
        }
        row = randRow;
        col = randCol;
        nextRow = randRow;
        nextCol = randCol;
        nextCost = calc_Cost(startMap.Tgrid[randRow][randCol], CT_PLAYER);
        w_row = start_w_row;
        w_col = start_w_col;

        bag.push_back(items("Pokeball", 10));
        bag.push_back(items("Potion", 10));
        bag.push_back(items("Revive", 10));

        WM->player = this; // put player into world map
    }

    int usePokeball(personalPokemon *pokemon)
    {
        if (bag[0].quantity > 0)
        {
            if ((int)party.size() < 6)
            {
                bag[0].quantity--;
                party.push_back(*pokemon);
                return 0;
            }
            else
            {
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "You already have 6 pokemon in your party.");
                int usrKey;
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        return 2;
                    }
                }
            }
        }
        else
        {
            mvprintw(13, 10, "*                                                         *");
            mvprintw(13, 13, "You do not have any Pokeballs.");
            int usrKey;
            while (1)
            {
                usrKey = getch();
                if (usrKey == ' ')
                {
                    return 1;
                }
            }
        }
    }

    int usePotion(personalPokemon *pokemon)
    {
        if (bag[1].quantity > 0)
        {
            if (pokemon->HP < pokemon->max_HP)
            {
                bag[1].quantity--;
                pokemon->HP += 20;
                if (pokemon->HP > pokemon->max_HP)
                {
                    pokemon->HP = pokemon->max_HP;
                }

                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "%s has been healed", pokemon->identifier.c_str());
                int usrKey;
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        return 1;
                    }
                }
            }
            else
            {
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "This pokemon is already at full health.");
                int usrKey;
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        return 1;
                    }
                }
            }
        }
        else
        {
            mvprintw(13, 10, "*                                                         *");
            mvprintw(13, 13, "You do not have any Potions.");
            int usrKey;
            while (1)
            {
                usrKey = getch();
                if (usrKey == ' ')
                {
                    return 0;
                }
            }
        }
    }

    int useRevive(personalPokemon *pokemon)
    {
        if (bag[2].quantity > 0)
        {
            if (pokemon->HP == 0)
            {
                bag[2].quantity--;
                pokemon->HP = pokemon->max_HP / 2;

                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "%s has been revived.", pokemon->identifier.c_str());
                int usrKey;
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        return 1;
                    }
                }

                return 0;
            }
            else
            {
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "This pokemon is already alive.");
                int usrKey;
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        return 1;
                    }
                }
            }
        }
        else
        {
            mvprintw(13, 10, "*                                                         *");
            mvprintw(13, 13, "You do not have any Revives.");
            int usrKey;
            while (1)
            {
                usrKey = getch();
                if (usrKey == ' ')
                {
                    return 1;
                }
            }
        }
    }

    int battle(NPC *opponent)
    {

        createPanel(3, 20, 10, 69);
        mvprintw(12, 10, "***********************************************************");
        mvprintw(13, 13, "%s wants to battle you! Press space to continue", opponent->name.c_str());
        // mvprintw(23, 0, "isDefeated: %d", opponent->isDefeated);

        int trainerCurPokemon = 0;

        // mvprintw(22, 0, "Opponent party size: %d", (int)opponent->party.size());

        int usrKey;
        while (1)
        {
            usrKey = getch();
            if (usrKey == ' ')
            {
                break;
            }
        }

        for (int i = 0; i < (int)opponent->party.size(); i++)
        {
            if (opponent->party[i].HP > 0)
            {
                trainerCurPokemon = i;
                break;
            }
        }

        int curPokemon = -1;

        // for (int i = 0; i < (int)party.size(); i++)
        // {
        //     mvprintw(23 + i, 15, "%s HP: %d, LVL: %d", party[i].identifier.c_str(), party[i].HP, party[i].level);
        // }

        for (int i = 0; i < (int)party.size(); i++) // Determines a pokemon to send out
        {
            if (party[i].HP > 0)
            {
                curPokemon = i;
                break;
            }
        }

        if (curPokemon == -1)
        {
            mvprintw(0, 0, "No pokemon available!");
            return 1; // No pokemon available!
        }

        mvprintw(13, 11, "                                                         ");
        mvprintw(13, 13, "Go.. %s! Press space to continue.", party[curPokemon].identifier.c_str());

        // Displays pokemon stats
        mvprintw(4, 13, "%s ", opponent->party[trainerCurPokemon].identifier.c_str());
        mvprintw(5, 13, "HP: %d ", opponent->party[trainerCurPokemon].HP);
        mvprintw(6, 13, "Level: %d ", opponent->party[trainerCurPokemon].level);

        mvprintw(9, 40, "%s ", party[curPokemon].identifier.c_str());
        mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
        mvprintw(11, 40, "Level: %d ", party[curPokemon].level);

        while (1)
        {
            usrKey = getch();
            if (usrKey == ' ')
            {
                break;
            }
        }

        mvprintw(13, 11, "                                                         ");

        int randomNum;
        // bool over;
        Moves wildPokemonMove;
        Moves personalMove;

        int personalMoveKey;
        int usrNum;

        int chosen;
        while (1) // Battle sequence
        {
            for (int i = 0; i < (int)opponent->party.size(); i++)
            {
                if (opponent->party[i].HP > 0)
                {
                    trainerCurPokemon = i;
                    break;
                }
            }

            if (party[curPokemon].HP <= 0) // If your current pokemon has fainted
            {
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "Your %s fainted! Press space to continue.", party[curPokemon].identifier.c_str());
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        break;
                    }
                }

                chosen = 0;
                for (int i = 0; i < (int)party.size(); i++) // Checks to see if there are available pokemon to fight in your party
                {
                    if (party[i].HP > 0)
                    {
                        chosen++;
                    }
                }
                if (chosen > 0)
                {

                    int status = choosePokemon(party, 0);
                    if (status == -1)
                    {
                        break;
                    }
                    else
                    {
                        curPokemon = status;
                    }

                    createPanel(12, 20, 10, 69);

                    mvprintw(13, 13, "Go.. %s! Press space to continue", party[curPokemon].identifier.c_str());

                    usrKey = 0;
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }

                    mvprintw(9, 10, "*                                                          *");
                    mvprintw(9, 40, "%s       ", party[curPokemon].identifier.c_str());
                    mvprintw(10, 40, "HP: %d    ", party[curPokemon].HP);
                    mvprintw(11, 40, "Level: %d    ", party[curPokemon].level);
                }
                else // If all of your pokemon are unavailable to fight
                {
                    mvprintw(13, 10, "*                                                         *");
                    mvprintw(13, 13, "You have no more pokemon to switch to!");
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }
                    return 0;
                }
            }

            createPanel(12, 20, 10, 69);
            mvprintw(15, 13, "1. Fight");
            mvprintw(17, 13, "2. Bag");
            mvprintw(15, 22, "3. Pokemon");
            mvprintw(17, 22, "4. Run");

            randomNum = rand() % (int)opponent->party[trainerCurPokemon].moves.size();
            wildPokemonMove = opponent->party[trainerCurPokemon].moves[randomNum]; // Generates a random move for the opponent pokemon

            int status;

            usrKey = getch();
            switch (usrKey)
            {
            case ' ':
            {
                mvprintw(13, 0, "*                                                        *");
                mvprintw(13, 13, "Invalid Input. Press space to continue.");
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        break;
                    }
                }
                break;
            }
            case '1': // User wants to fight
            {
                createPanel(12, 20, 33, 69);
                for (int i = 0; i < (int)party[curPokemon].moves.size(); i++)
                {
                    mvprintw(14 + i, 35, "%d. %s", i + 1, party[curPokemon].moves[i].identifier.c_str());
                }

                while (1)
                {
                    personalMoveKey = getch();
                    usrNum = personalMoveKey - 48;

                    if (usrNum <= (int)opponent->party[trainerCurPokemon].moves.size() && usrNum > 0)
                    {
                        personalMove = party[curPokemon].moves[usrNum - 1];
                        break;
                    }
                }

                for (int i = 0; i < 7; i++)
                {
                    mvprintw(13 + i, 11, "                                                         ");
                }

                if (personalMove.priority > wildPokemonMove.priority) // If the user pokemon's move has a higher priority
                {
                    attack(&party[curPokemon], personalMove, &opponent->party[trainerCurPokemon], wildPokemonMove, true);
                    mvprintw(5, 13, "HP: %d ", opponent->party[trainerCurPokemon].HP);
                    mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                }
                else if (wildPokemonMove.priority > personalMove.priority) // If the opponent pokemon's move has a higher priority
                {
                    attack(&opponent->party[trainerCurPokemon], wildPokemonMove, &party[curPokemon], personalMove, true);
                    mvprintw(5, 13, "HP: %d ", opponent->party[trainerCurPokemon].HP);
                    mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                }
                else
                {
                    if (party[curPokemon].speed > opponent->party[trainerCurPokemon].speed) // If the user pokemon's speed is higher
                    {
                        attack(&party[curPokemon], personalMove, &opponent->party[trainerCurPokemon], wildPokemonMove, true);
                        mvprintw(5, 13, "HP: %d ", opponent->party[trainerCurPokemon].HP);
                        mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                    }
                    else if (opponent->party[trainerCurPokemon].speed > party[curPokemon].speed) // If the opponent pokemon's speed is higher
                    {
                        attack(&opponent->party[trainerCurPokemon], wildPokemonMove, &party[curPokemon], personalMove, true);
                        mvprintw(5, 13, "HP: %d ", opponent->party[trainerCurPokemon].HP);
                        mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                    }
                    else
                    {
                        randomNum = rand() % 2; // If the pokemon's speed is the same, a random number is generated to determine who attacks first
                        if (randomNum == 0)
                        {
                            attack(&party[curPokemon], personalMove, &opponent->party[trainerCurPokemon], wildPokemonMove, true);
                            mvprintw(5, 13, "HP: %d ", opponent->party[trainerCurPokemon].HP);
                            mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                        }
                        else
                        {
                            attack(&opponent->party[trainerCurPokemon], wildPokemonMove, &party[curPokemon], personalMove, true);
                            mvprintw(5, 13, "HP: %d ", opponent->party[trainerCurPokemon].HP);
                            mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                        }
                    }

                    if (opponent->party[trainerCurPokemon].HP <= 0) // If the current opponent's pokemon has fainted
                    {
                        // over = true;
                        mvprintw(13, 10, "*                                                         *");
                        mvprintw(13, 13, "%s has been defeated! Press space to continue.", opponent->party[trainerCurPokemon].identifier.c_str());
                        while (1)
                        {
                            usrKey = getch();
                            if (usrKey == ' ')
                            {
                                break;
                            }
                        }

                        if (trainerCurPokemon == (int)opponent->party.size() - 1) // If you have beaten all of the trainer's pokemon
                        {
                            mvprintw(13, 10, "*                                                         *");
                            mvprintw(13, 13, "%s: Aww man! You beat me!", opponent->name.c_str());
                            while (1)
                            {
                                usrKey = getch();
                                if (usrKey == ' ')
                                {
                                    break;
                                }
                            }
                            int exp = party[curPokemon].gainExp(opponent->party[trainerCurPokemon], false);
                            mvprintw(13, 10, "*                                                         *");
                            mvprintw(13, 13, "%s has gained %d exp!", party[curPokemon].identifier.c_str(), exp);
                            while (1)
                            {
                                usrKey = getch();
                                if (usrKey == ' ')
                                {
                                    break;
                                }
                            }

                            opponent->isDefeated = 1;
                            // m->charGrid[opponent->row][opponent->col] = NULL;

                            // REMOVED FOR TESTING

                            return 5;
                        }
                        else // Trainer has more available pokemon to fight
                        {
                            trainerCurPokemon++;
                            mvprintw(13, 10, "*                                                         *");
                            mvprintw(13, 13, "%s: %s, I choose you!", opponent->name.c_str(), opponent->party[trainerCurPokemon].identifier.c_str());
                            mvprintw(4, 13, "%s ", opponent->party[trainerCurPokemon].identifier.c_str());
                            mvprintw(5, 13, "HP: %d ", opponent->party[trainerCurPokemon].HP);
                            mvprintw(6, 13, "Level: %d ", opponent->party[trainerCurPokemon].level);
                        }
                    }
                    else if (party[curPokemon].HP <= 0) // If your current pokemon has fainted
                    {
                        mvprintw(13, 10, "*                                                         *");
                        mvprintw(13, 13, "Your %s fainted! Press space to continue.", party[curPokemon].identifier.c_str());
                        while (1)
                        {
                            usrKey = getch();
                            if (usrKey == ' ')
                            {
                                break;
                            }
                        }

                        chosen = 0;
                        for (int i = 0; i < (int)party.size(); i++) // Checks to see if there are available pokemon to fight in your party
                        {
                            if (party[i].HP > 0)
                            {
                                chosen++;
                            }
                        }
                        if (chosen > 0)
                        {

                            status = choosePokemon(party, 0);
                            if (status == -1)
                            {
                                break;
                            }
                            else
                            {
                                curPokemon = status;
                            }

                            createPanel(12, 20, 10, 69);

                            mvprintw(13, 13, "Go.. %s! Press space to continue", party[curPokemon].identifier.c_str());

                            usrKey = 0;
                            while (1)
                            {
                                usrKey = getch();
                                if (usrKey == ' ')
                                {
                                    break;
                                }
                            }

                            mvprintw(9, 10, "*                                                          *");
                            mvprintw(9, 40, "%s       ", party[curPokemon].identifier.c_str());
                            mvprintw(10, 40, "HP: %d    ", party[curPokemon].HP);
                            mvprintw(11, 40, "Level: %d    ", party[curPokemon].level);
                        }
                        else // If all of your pokemon are unavailable to fight
                        {
                            mvprintw(13, 10, "*                                                         *");
                            mvprintw(13, 13, "You have no more pokemon to switch to!");
                            while (1)
                            {
                                usrKey = getch();
                                if (usrKey == ' ')
                                {
                                    break;
                                }
                            }
                            return 0;
                        }
                    }
                }

                break;
            }
            case '2': // User chooses to use an item from your bag
            {
                createPanel(12, 20, 10, 69);
                mvprintw(13, 13, "Choose an item from your bag: ");

                for (int i = 0; i < (int)bag.size(); i++)
                {
                    attron(COLOR_PAIR(COLOR_YELLOW));
                    attron(A_BLINK);
                    mvprintw(15 + i, 13, "%d. %s : %d", i + 1, bag[i].name.c_str(), bag[i].quantity);
                    attroff(COLOR_PAIR(COLOR_YELLOW));
                    attroff(A_BLINK);
                }

                while (1)
                {
                    usrKey = getch();
                    usrNum = usrKey - 48;
                    switch (usrNum)
                    {
                    case 1:
                    {
                        mvprintw(13, 13, "*                                                          *");
                        mvprintw(13, 13, "You can't catch their pokemon! Press space to continue.");
                        while (1)
                        {
                            usrKey = getch();
                            if (usrKey == ' ')
                            {
                                break;
                            }
                        }
                        break;
                    }

                    case 2:
                    {
                        status = choosePokemon(party, 1);
                        if (status == -1)
                        {
                            break;
                        }
                        else
                        {
                            usePotion(&party[status]);
                        }
                        mvprintw(9, 10, "*                                                          *");
                        mvprintw(9, 40, "%s", party[curPokemon].identifier.c_str());
                        mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                        mvprintw(11, 40, "Level: %d ", party[curPokemon].level);
                        createPanel(12, 20, 10, 69);
                        break;
                    }

                    case 3:
                    {
                        status = choosePokemon(party, 2);
                        if (status == -1)
                        {
                            break;
                        }
                        else
                        {
                            useRevive(&party[status]);
                        }
                        mvprintw(9, 10, "*                                                          *");
                        mvprintw(9, 40, "%s", party[curPokemon].identifier.c_str());
                        mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                        mvprintw(11, 40, "Level: %d ", party[curPokemon].level);
                        createPanel(12, 20, 10, 69);
                        break;
                    }
                    }
                    break;
                }
                break;
            }
            case '3':
            {
                chosen = 0;
                for (int i = 0; i < (int)party.size(); i++)
                {
                    if (party[i].HP > 0)
                    {
                        chosen++;
                    }
                }
                if (chosen > 0)
                {
                    status = choosePokemon(party, 0);
                    if (status == -1)
                    {
                        break;
                    }
                    else
                    {
                        curPokemon = status;
                    }

                    createPanel(12, 20, 10, 69);

                    mvprintw(13, 13, "Go.. %s! Press space to continue", party[curPokemon].identifier.c_str());

                    usrKey = 0;
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }

                    mvprintw(9, 10, "*                                                          *");
                    mvprintw(9, 40, "%s", party[curPokemon].identifier.c_str());
                    mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                    mvprintw(11, 40, "Level: %d ", party[curPokemon].level);
                }
                else
                {
                    // over = true;
                    mvprintw(13, 10, "*                                                         *");
                    mvprintw(13, 13, "You have no more pokemon to switch to!");
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }
                    return 0;
                }

                break;
            }
            case '4':
            {
                mvprintw(13, 13, "You ran away from %s! Press space to continue.", opponent->name.c_str());
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        break;
                    }
                }
                return 0;
                // // over = true;
                // mvprintw(13, 10, "*                                                         *");
                // mvprintw(13, 13, "You cannot run away from a trainer battle!");
                // while (1)
                // {
                //     usrKey = getch();
                //     if (usrKey == ' ')
                //     {
                //         break;
                //     }
                // }

                // break;
            }
            default:
            {
                mvprintw(13, 0, "*                                                         *");
                mvprintw(13, 13, "Invalid input. Please try again.");
            }
            }

            if (usrKey != '1' && usrKey != '4')
            {
                // Have the NPC pokemon throw its move after you've switched the pokemon, or grabbed the bag.
                attack(&opponent->party[trainerCurPokemon], wildPokemonMove, &party[curPokemon], personalMove, false);
                mvprintw(9, 10, "*                                                          *");
                mvprintw(9, 40, "%s", party[curPokemon].identifier.c_str());
                mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                mvprintw(11, 40, "Level: %d ", party[curPokemon].level);
            }
        }
        return 0;
    }

    int encounterWildPokemon()
    {
        personalPokemon *wildPokemon = new personalPokemon(w_row, w_col);

        createPanel(3, 20, 10, 69);
        mvprintw(12, 10, "***********************************************************");
        mvprintw(13, 13, "A wild %s appeared! Press space to continue.", wildPokemon->identifier.c_str());

        int usrKey;
        while (1)
        {
            usrKey = getch();
            if (usrKey == ' ')
            {
                break;
            }
        }

        int curPokemon = -1;

        for (int i = 0; i < (int)party.size(); i++) // Determines a pokemon to send out
        {
            if (party[i].HP > 0)
            {
                curPokemon = i;
                break;
            }
        }

        if (curPokemon == -1)
        {
            mvprintw(0, 0, "No pokemon available!");
            return 1; // No pokemon available!
        }

        mvprintw(13, 11, "                                                         ");
        mvprintw(13, 13, "Go.. %s! Press space to continue.", party[curPokemon].identifier.c_str());

        // Displays pokemon stats
        mvprintw(4, 13, "%s ", wildPokemon->identifier.c_str());
        mvprintw(5, 13, "HP: %d ", wildPokemon->HP);
        mvprintw(6, 13, "Level: %d ", wildPokemon->level);

        mvprintw(9, 40, "%s ", party[curPokemon].identifier.c_str());
        mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
        mvprintw(11, 40, "Level: %d ", party[curPokemon].level);

        while (1)
        {
            usrKey = getch();
            if (usrKey == ' ')
            {
                break;
            }
        }

        mvprintw(13, 11, "                                                         ");

        int randomNum;
        // bool over;
        Moves wildPokemonMove;
        Moves personalMove;

        int personalMoveKey;
        int usrNum;

        int chosen;
        while (1)
        {

            if (wildPokemon->HP <= 0)
            {
                // over = true;
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "You defeated the wild %s! Press space to continue.", wildPokemon->identifier.c_str());
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        break;
                    }
                }
                int exp = party[curPokemon].gainExp(*wildPokemon, true);
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "%s has gained %d exp!", party[curPokemon].identifier.c_str(), exp);
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        break;
                    }
                }

                delete wildPokemon;
                return 0;
            }
            else if (party[curPokemon].HP <= 0)
            {
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "Your %s fainted! Press space to continue.", party[curPokemon].identifier.c_str());
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        break;
                    }
                }

                chosen = 0;
                for (int i = 0; i < (int)party.size(); i++)
                {
                    if (party[i].HP > 0)
                    {
                        chosen++;
                    }
                }
                if (chosen > 0)
                {

                    int status = choosePokemon(party, 0);
                    if (status == -1)
                    {
                        break;
                    }
                    else
                    {
                        curPokemon = status;
                    }

                    createPanel(12, 20, 10, 69);

                    mvprintw(13, 13, "Go.. %s! Press space to continue", party[curPokemon].identifier.c_str());

                    usrKey = 0;
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }

                    mvprintw(9, 10, "*                                                          *");
                    mvprintw(9, 40, "%s       ", party[curPokemon].identifier.c_str());
                    mvprintw(10, 40, "HP: %d    ", party[curPokemon].HP);
                    mvprintw(11, 40, "Level: %d    ", party[curPokemon].level);
                }
                else
                {
                    mvprintw(13, 10, "*                                                         *");
                    mvprintw(13, 13, "You have no more pokemon to switch to!");
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }
                    return 0;
                }
            }

            createPanel(12, 20, 10, 69);
            mvprintw(15, 13, "1. Fight");
            mvprintw(17, 13, "2. Bag");
            mvprintw(15, 22, "3. Pokemon");
            mvprintw(17, 22, "4. Run");

            randomNum = rand() % (int)wildPokemon->moves.size();
            wildPokemonMove = wildPokemon->moves[randomNum];

            int status;

            usrKey = getch();
            switch (usrKey)
            {
            case ' ':
            {
                mvprintw(13, 10, "*                                                         *");
                mvprintw(13, 13, "Invalid Input. Press space to continue.");
                while (1)
                {
                    usrKey = getch();
                    if (usrKey == ' ')
                    {
                        break;
                    }
                }
                break;
            }
            case '1':
            {
                createPanel(12, 20, 33, 69);
                for (int i = 0; i < (int)party[curPokemon].moves.size(); i++)
                {
                    mvprintw(14 + i, 35, "%d. %s", i + 1, party[curPokemon].moves[i].identifier.c_str());
                }

                while (1)
                {
                    personalMoveKey = getch();
                    usrNum = personalMoveKey - 48;

                    if (usrNum <= (int)wildPokemon->moves.size() && usrNum > 0)
                    {
                        personalMove = party[curPokemon].moves[usrNum - 1];
                        break;
                    }
                }

                for (int i = 0; i < 7; i++)
                {
                    mvprintw(13 + i, 11, "                                                         ");
                }

                if (personalMove.priority > wildPokemonMove.priority)
                {
                    attack(&party[curPokemon], personalMove, wildPokemon, wildPokemonMove, true);
                    mvprintw(5, 13, "HP: %d ", wildPokemon->HP);
                    mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                }
                else if (wildPokemonMove.priority > personalMove.priority)
                {
                    attack(wildPokemon, wildPokemonMove, &party[curPokemon], personalMove, true);
                    mvprintw(5, 13, "HP: %d ", wildPokemon->HP);
                    mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                }
                else
                {
                    if (party[curPokemon].speed > wildPokemon->speed)
                    {
                        attack(&party[curPokemon], personalMove, wildPokemon, wildPokemonMove, true);
                        mvprintw(5, 13, "HP: %d ", wildPokemon->HP);
                        mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                    }
                    else if (wildPokemon->speed > party[curPokemon].speed)
                    {
                        attack(wildPokemon, wildPokemonMove, &party[curPokemon], personalMove, true);
                        mvprintw(5, 13, "HP: %d ", wildPokemon->HP);
                        mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                    }
                    else
                    {
                        randomNum = rand() % 2;
                        if (randomNum == 0)
                        {
                            attack(&party[curPokemon], personalMove, wildPokemon, wildPokemonMove, true);
                            mvprintw(5, 13, "HP: %d ", wildPokemon->HP);
                            mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                        }
                        else
                        {
                            attack(wildPokemon, wildPokemonMove, &party[curPokemon], personalMove, true);
                            mvprintw(5, 13, "HP: %d ", wildPokemon->HP);
                            mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                        }
                    }

                    if (wildPokemon->HP <= 0)
                    {
                        // over = true;
                        mvprintw(13, 10, "*                                                         *");
                        mvprintw(13, 13, "You defeated the wild %s! Press space to continue.", wildPokemon->identifier.c_str());
                        while (1)
                        {
                            usrKey = getch();
                            if (usrKey == ' ')
                            {
                                break;
                            }
                        }
                        int exp = party[curPokemon].gainExp(*wildPokemon, true);
                        mvprintw(13, 10, "*                                                         *");
                        mvprintw(13, 13, "%s has gained %d exp!", party[curPokemon].identifier.c_str(), exp);
                        while (1)
                        {
                            usrKey = getch();
                            if (usrKey == ' ')
                            {
                                break;
                            }
                        }

                        delete wildPokemon;
                        return 0;
                    }
                    else if (party[curPokemon].HP <= 0)
                    {
                        mvprintw(13, 10, "*                                                         *");
                        mvprintw(13, 13, "Your %s fainted! Press space to continue.", party[curPokemon].identifier.c_str());
                        while (1)
                        {
                            usrKey = getch();
                            if (usrKey == ' ')
                            {
                                break;
                            }
                        }

                        chosen = 0;
                        for (int i = 0; i < (int)party.size(); i++)
                        {
                            if (party[i].HP > 0)
                            {
                                chosen++;
                            }
                        }
                        if (chosen > 0)
                        {

                            status = choosePokemon(party, 0);
                            if (status == -1)
                            {
                                break;
                            }
                            else
                            {
                                curPokemon = status;
                            }

                            createPanel(12, 20, 10, 69);

                            mvprintw(13, 13, "Go.. %s! Press space to continue", party[curPokemon].identifier.c_str());

                            usrKey = 0;
                            while (1)
                            {
                                usrKey = getch();
                                if (usrKey == ' ')
                                {
                                    break;
                                }
                            }

                            mvprintw(9, 10, "*                                                          *");
                            mvprintw(9, 40, "%s       ", party[curPokemon].identifier.c_str());
                            mvprintw(10, 40, "HP: %d    ", party[curPokemon].HP);
                            mvprintw(11, 40, "Level: %d    ", party[curPokemon].level);
                        }
                        else
                        {
                            mvprintw(13, 10, "*                                                         *");
                            mvprintw(13, 13, "You have no more pokemon to switch to!");
                            while (1)
                            {
                                usrKey = getch();
                                if (usrKey == ' ')
                                {
                                    break;
                                }
                            }
                            return 0;
                        }
                    }
                }

                break;
            }
            case '2':
            {
                createPanel(12, 20, 10, 69);
                mvprintw(13, 13, "Choose an item from your bag: ");

                for (int i = 0; i < (int)bag.size(); i++)
                {
                    attron(COLOR_PAIR(COLOR_YELLOW));
                    attron(A_BLINK);
                    mvprintw(15 + i, 13, "%d. %s : %d", i + 1, bag[i].name.c_str(), bag[i].quantity);
                    attroff(COLOR_PAIR(COLOR_YELLOW));
                    attroff(A_BLINK);
                }

                while (1)
                {
                    usrKey = getch();
                    usrNum = usrKey - 48;
                    switch (usrNum)
                    {
                    case 1:
                    {
                        int status = usePokeball(wildPokemon);

                        if (status == 2)
                        {
                            mvprintw(13, 10, "*                                                         *");
                            mvprintw(13, 13, "%s has fled! Press space to continue", wildPokemon->identifier.c_str());
                            while (1)
                            {
                                usrKey = getch();
                                if (usrKey == ' ')
                                {
                                    return 0;
                                }
                            }
                        }
                        else if (status == 0)
                        {

                            while (1)
                            {

                                mvprintw(13, 10, "*                                                          *");
                                mvprintw(13, 13, "You have thrown a pokeball");
                                usrKey = getch();
                                if (usrKey == ' ')
                                {
                                    break;
                                }
                            }

                            mvprintw(13, 10, "*                                                         *");
                            mvprintw(13, 13, "You have caught %s! Press space to continue", wildPokemon->identifier.c_str());
                            while (1)
                            {
                                usrKey = getch();
                                if (usrKey == ' ')
                                {
                                    return 0;
                                }
                            }
                        }
                        break;
                    }

                    case 2:
                    {
                        status = choosePokemon(party, 1);
                        if (status == -1)
                        {
                            break;
                        }
                        else
                        {
                            usePotion(&party[status]);
                        }
                        mvprintw(9, 10, "*                                                          *");
                        mvprintw(9, 40, "%s", party[curPokemon].identifier.c_str());
                        mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                        mvprintw(11, 40, "Level: %d ", party[curPokemon].level);
                        createPanel(12, 20, 10, 69);
                        break;
                    }

                    case 3:
                    {
                        status = choosePokemon(party, 2);
                        if (status == -1)
                        {
                            break;
                        }
                        else
                        {
                            useRevive(&party[status]);
                        }
                        mvprintw(9, 10, "*                                                          *");
                        mvprintw(9, 40, "%s", party[curPokemon].identifier.c_str());
                        mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                        mvprintw(11, 40, "Level: %d ", party[curPokemon].level);
                        createPanel(12, 20, 10, 69);
                        break;
                    }
                    }
                    break;
                }
                break;
            }
            case '3':
            {
                chosen = 0;
                for (int i = 0; i < (int)party.size(); i++)
                {
                    if (party[i].HP > 0)
                    {
                        chosen++;
                    }
                }
                if (chosen > 0)
                {
                    status = choosePokemon(party, 0);
                    if (status == -1)
                    {
                        break;
                    }
                    else
                    {
                        curPokemon = status;
                    }

                    createPanel(12, 20, 10, 69);

                    mvprintw(13, 13, "Go.. %s! Press space to continue", party[curPokemon].identifier.c_str());

                    usrKey = 0;
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }

                    mvprintw(9, 10, "*                                                          *");
                    mvprintw(9, 40, "%s", party[curPokemon].identifier.c_str());
                    mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                    mvprintw(11, 40, "Level: %d ", party[curPokemon].level);
                }
                else
                {
                    // over = true;
                    mvprintw(13, 10, "*                                                         *");
                    mvprintw(13, 13, "You have no more pokemon to switch to!");
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }
                    return 0;
                }

                break;
            }

            case '4':
            {
                // over = true;
                mvprintw(13, 10, "*                                                         *");

                if (rand() % 100 < 75)
                {
                    mvprintw(13, 13, "You ran away from the wild %s! Press space to continue.", wildPokemon->identifier.c_str());
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }
                    return 0;
                }
                else
                {
                    mvprintw(13, 13, "You have failed to run away!");
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }
                    break;
                }
            }
            default:
            {
                mvprintw(13, 0, "*                                                         *");
                mvprintw(13, 13, "Invalid input. Please try again.");
            }
            }

            if (usrKey != '1' && usrKey != '4')
            {
                // Have the NPC pokemon throw its move after you've switched the pokemon, or grabbed the bag.
                attack(wildPokemon, wildPokemonMove, &party[curPokemon], personalMove, false);
                mvprintw(9, 10, "*                                                          *");
                mvprintw(9, 40, "%s", party[curPokemon].identifier.c_str());
                mvprintw(10, 40, "HP: %d ", party[curPokemon].HP);
                mvprintw(11, 40, "Level: %d ", party[curPokemon].level);
            }
        }

        delete wildPokemon;

        return 0;
    }

    /*
        int gymMatch(Gym *curGym)
        {
            // Throw dialogue

            // Choose leader to battle

            // Battle
        }
        */
    // returns 1 if the player is by the water and 0 if it is not
    int playerByWater(Map *m)
    {
        for (int i = -1; i < 2; i++)
        {
            for (int j = -1; j < 2; j++)
            {
                if (m->Tgrid[row + i][col + j] == TT_BRIDGE || m->Tgrid[row + i][col + j] == TT_WATER)
                {
                    return 1;
                }
            }
        }
        return 0;
    }

    ~PC() {}
};

///////////////////////////////////////////////////////////N-Curses////////////////////////////////////////////////////////////////////////////

int initTerminal()
{
    initscr();         // makes screen blank and sets it up for ncurses
    raw();             // takes uninterupted input
    noecho();          // doesn't show what you are typing
    curs_set(0);       // curs_set() is an ncursee function that sets the cursor visibility. 0 means invisible, 1 means visible, 2 means very visible
    keypad(stdscr, 1); // allows you to use keys
    start_color();
    init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
    return 0;
}

int clearScreen()
{
    for (int i = 1; i < 22; i++)
    {
        for (int j = 0; j < 80; j++)
        {
            mvprintw(i, j, " ");
        }
    }
    return 0;
}

int clearScreen_top()
{
    for (int i = 0; i < 80; i++)
    {
        mvprintw(0, i, " ");
    }
    return 0;
}

int createPanel(int topRow, int bottomRow, int leftCol, int rightCol)
{
    for (int i = topRow; i <= bottomRow; i++)
    {
        for (int j = leftCol; j <= rightCol; j++)
        {
            if (i == topRow || i == bottomRow || j == leftCol || j == rightCol)
            {
                mvprintw(i, j, "*");
            }
            else
            {
                mvprintw(i, j, " ");
            }
        }
    }
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Function to completely generate a map
Map *generateNewMap(WorldMap *WM, int row, int col)
{
    int newNgate;
    int newEgate;
    int newSgate;
    int newWgate;

    // possibly check to see if THIS map is out of bounds and whether a map already exists here in the world map (should be checked in main though)

    // Generate North Gate
    if (row == 0)
    { // you are at the top of the world so don't put a gate
        newNgate = -1;
    }
    else if (WM->mapGrid[row - 1][col] != NULL)
    { // north map exists so take the map's southern gate column number
        newNgate = WM->mapGrid[row - 1][col]->gateS;
    }
    else
    {                                 // no north map. create random gate location
        newNgate = (rand() % 78) + 1; // generate a random number from 1 to 78 for which COL to put the north enterance
    }
    // Generate East Gate
    if (col == 400)
    { // you are at the far right of the world so don't put a gate
        newEgate = -1;
    }
    else if (WM->mapGrid[row][col + 1] != NULL)
    { // East map exists so take the map's west gate row number
        newEgate = WM->mapGrid[row][col + 1]->gateW;
    }
    else
    {                                 // no East map. create random gate location
        newEgate = (rand() % 19) + 1; // generate a random number from 1 to 19 for which row to put the east enterance
    }
    // Generate South Gate
    if (row == 400)
    { // you are at the far bottom of the world so don't put a gate
        newSgate = -1;
    }
    else if (WM->mapGrid[row + 1][col] != NULL)
    { // South map exists so take the map's northern gate column number
        newSgate = WM->mapGrid[row + 1][col]->gateN;
    }
    else
    {                                 // no South map. create random gate location
        newSgate = (rand() % 78) + 1; // generate a random number from 1 to 78 for which col to put the south enterance
    }
    // Generate West Gate
    if (col == 0)
    { // you are at the far left of the world so don't put a gate
        newWgate = -1;
    }
    else if (WM->mapGrid[row][col - 1] != NULL)
    { // West map exists so take the map's east gate row number
        newWgate = WM->mapGrid[row][col - 1]->gateE;
    }
    else
    {                                 // no West map. create random gate location
        newWgate = (rand() % 19) + 1; // generate a random number from 1 to 19 for which row to put the West enterance
    }

    Map *newMap = new Map(newNgate, newEgate, newSgate, newWgate, row, col);
    newMap->generateBoarder();
    newMap->generateBiomes();
    newMap->generatePaths();
    newMap->placeRandomTerrain();
    newMap->generateBuildings();

    return newMap;
}

// calculate the cost to move to the given cell of a map
int calc_Cost(int terrain, int chartype)
{
    int costs[9][22] =
        // None,Bldr,tree,path,Bridge,PMrt,Cntr,TGras,SGras,Mtn,Forst,Wtr,Gate,pewter, cerulean, vermilion, celadon, fushia, saffron, cinnabar, viridian, elitefour
        {
            {MAX, MAX, MAX, 10, 10, 10, 10, 20, 10, MAX, MAX, MAX, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},             // PC
            {MAX, MAX, MAX, 10, 10, 50, 50, 15, 10, 15, 15, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX},     // Hiker
            {MAX, MAX, MAX, 10, 10, 50, 50, 20, 10, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX},   // Rival
            {MAX, MAX, MAX, MAX, 7, MAX, MAX, MAX, MAX, MAX, MAX, 7, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX}, // Swimmer
            {MAX, MAX, MAX, 10, 10, 50, 50, 20, 10, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX},   // Pacer
            {MAX, MAX, MAX, 10, 10, 50, 50, 20, 10, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX},   // Wanderer
            {MAX, MAX, MAX, 10, 10, 50, 50, 20, 10, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX},   // Explorer
            {MAX, MAX, MAX, 10, 10, 50, 50, 20, 10, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX},   // other
        };
    return costs[chartype][terrain];
    return 0;
}

// generate a cost map
int genCostMap(int costMap[21][80], int NPCtype, Map *terr_map, PC pc)
{
    int visMap[21][80];
    GQnode retNode;
    int neighborRow;
    int neighborCol;
    int newCost;

    // set all cells of the cost map to infinity and the vis map all to 0
    for (int i = 0; i < 21; i++)
    {
        for (int j = 0; j < 80; j++)
        {
            costMap[i][j] = MAX;
            visMap[i][j] = 0;
        }
    }
    costMap[pc.row][pc.col] = 0;
    costMap[pc.row][pc.col] = 0;
    // initialize my priority queue
    GodQueue q;
    GQinit(&q);
    GQenqueue(&q, pc.row, pc.col, 0, NULL);
    while (!GQis_empty(&q))
    {
        GQdequeue(&q, &retNode);

        // iterate through all neighbors of the dequeued node
        for (int m = -1; m <= 1; m++)
        {
            for (int n = -1; n <= 1; n++)
            {
                neighborRow = retNode.row + m;
                neighborCol = retNode.col + n;
                if ((m != 0 || n != 0) && neighborRow >= 0 && neighborRow <= 20 && neighborCol >= 0 && neighborCol <= 79)
                { // we want to ignore the cell we had just dequeued as well as any nodes out of bounds (in bounds has row indexes of 0 to 20 and column indexes of 0 to 79)
                    newCost = retNode.value + calc_Cost((terr_map->Tgrid[neighborRow][neighborCol]), NPCtype);
                    if (newCost < costMap[neighborRow][neighborCol])
                    {
                        costMap[neighborRow][neighborCol] = newCost;
                    }
                    if (visMap[neighborRow][neighborCol] == 0)
                    {                                                                                     // if the neighbor hasn't yet been visited before
                        GQenqueue(&q, neighborRow, neighborCol, costMap[neighborRow][neighborCol], NULL); // add it to the queue
                        visMap[neighborRow][neighborCol] = 1;                                             // mark it as visited
                    }
                }
            }
        }
    }
    return 0; // success
}

int enqueueAllChars(GodQueue *GQ, Map *m)
{
    for (int i = 0; i < 21; i++)
    {
        for (int j = 0; j < 80; j++)
        {
            if (m->charGrid[i][j] != NULL)
            {
                GQenqueue(GQ, i, j, m->charGrid[i][j]->nextCost, m->charGrid[i][j]); // enqueue each of the characters to the character queue with values equal to the cost to move to their "next row" and "next column"
            }
        }
    }
    return 0;
}

////////////////////////////////////////////////CHARACTER MOVEMENT FUNCTIONS////////////////////////////////////////////////////////////////////////

// method to fly
int move_to(WorldMap *WM, int destRow, int destCol, PC *player)
{
    if (destRow > 400 || destRow < 0 || destCol > 400 || destCol < 0)
    {
        mvprintw(0, 0, "ERROR: The coords (%d, %d) are out of bounds!\n", 200 - destRow, destCol - 200); // the -200 and 200- to print off the UI coordinates from the array coordinates
        return 1;                                                                                        // ERROR OUT OF BOUNDS
    }
    Map *curMap;
    GQdequeueAll(&(WM->charQueue));                                       // clear the character queue
    WM->mapGrid[w_row][w_col]->charGrid[player->row][player->col] = NULL; // remove the player from the current map
    w_row = destRow;
    w_col = destCol;

    if (WM->mapGrid[destRow][destCol] == NULL)
    { // if there is no map
        WM->mapGrid[destRow][destCol] = generateNewMap(WM, destRow, destCol);
        curMap = WM->mapGrid[destRow][destCol];
        // place PC on a random path on the map
        int randRow;
        int randCol;
        do
        {
            randRow = (rand() % 19) + 1;
            randCol = (rand() % 79) + 1;
        } while (curMap->Tgrid[randRow][randCol] != TT_PATH || curMap->charGrid[randRow][randCol] != NULL);
        player->updateCoords(randRow, randCol);
        player->updateNextCoords(randRow, randCol);
        curMap->charGrid[player->row][player->col] = player;                             // place the player on the new map
        GQenqueue(&(WM->charQueue), player->row, player->col, player->nextCost, player); // enqueue the player to the character queue
        generateCharacters(curMap, WM, player, NUMTRAINERS);
    }
    else
    { // MAP ALREADY EXISTS
        curMap = WM->mapGrid[destRow][destCol];
        // place PC on a random path on the map
        int randRow;
        int randCol;
        do
        {
            randRow = (rand() % 19) + 1;
            randCol = (rand() % 79) + 1;
        } while (curMap->Tgrid[randRow][randCol] != TT_PATH || curMap->charGrid[randRow][randCol] != NULL);
        player->updateCoords(randRow, randCol);
        player->updateNextCoords(randRow, randCol);
        curMap->charGrid[player->row][player->col] = player;                             // place the player on the new map
        GQenqueue(&(WM->charQueue), player->row, player->col, player->nextCost, player); // enqueue the player to the character queue
        enqueueAllChars(&(WM->charQueue), curMap);
    }
    curMap->printBoard();

    return 0;
}

// method to move maps through a gate
int enterGate(Map *m, WorldMap *WM, PC *player)
{
    Map *curMap;
    GQdequeueAll(&(WM->charQueue));               // clear the character queue
    m->charGrid[player->row][player->col] = NULL; // remove the player from the current map
    if (player->row == 0)
    {            // North Gate
        w_row--; // move player North in the world
        if (WM->mapGrid[w_row][w_col] != NULL)
        { // MAP ALREADY EXISTS
            curMap = WM->mapGrid[w_row][w_col];
            player->updateCoords(19, curMap->gateS);             // placing the PC one cell above the south gate and at the column of the south gate
            player->updateNextCoords(19, curMap->gateS);         // updating the next coords to match the current coords
            curMap->charGrid[player->row][player->col] = player; // place the player on the new map
            enqueueAllChars(&(WM->charQueue), curMap);           // enqueue all characters on the new map
        }
        else
        { // MAP DOESN'T EXIST
            WM->mapGrid[w_row][w_col] = generateNewMap(WM, w_row, w_col);
            curMap = WM->mapGrid[w_row][w_col];
            player->updateCoords(19, curMap->gateS);                                         // placing the PC one cell above the south gate and at the column of the south gate
            player->updateNextCoords(19, curMap->gateS);                                     // updating the next coords to match the current coords
            curMap->charGrid[player->row][player->col] = player;                             // place the player on the new map
            GQenqueue(&(WM->charQueue), player->row, player->col, player->nextCost, player); // enqueue the player to the character queue
            generateCharacters(curMap, WM, player, NUMTRAINERS);
        }
    }
    else if (player->row == 20)
    { // South Gate
        w_row++;
        if (WM->mapGrid[w_row][w_col] != NULL)
        { // MAP ALREADY EXISTS
            curMap = WM->mapGrid[w_row][w_col];
            player->updateCoords(1, curMap->gateN);              // placing the PC one cell below the North gate and at the column of the North gate
            player->updateNextCoords(1, curMap->gateN);          // updating the next coords to match the current coords
            curMap->charGrid[player->row][player->col] = player; // place the player on the new map
            enqueueAllChars(&(WM->charQueue), curMap);           // enqueue all characters on the new map
        }
        else
        { // MAP DOESN'T EXIST
            WM->mapGrid[w_row][w_col] = generateNewMap(WM, w_row, w_col);
            curMap = WM->mapGrid[w_row][w_col];
            player->updateCoords(1, curMap->gateN);                                          // placing the PC one cell below the North gate and at the column of the North gate
            player->updateNextCoords(1, curMap->gateN);                                      // updating the next coords to match the current coords
            curMap->charGrid[player->row][player->col] = player;                             // place the player on the new map
            GQenqueue(&(WM->charQueue), player->row, player->col, player->nextCost, player); // enqueue the player to the character queue
            generateCharacters(curMap, WM, player, NUMTRAINERS);
        }
    }
    else if (player->col == 0)
    { // West Gate
        w_col++;
        if (WM->mapGrid[w_row][w_col] != NULL)
        { // MAP ALREADY EXISTS
            curMap = WM->mapGrid[w_row][w_col];
            player->updateCoords(curMap->gateE, 78);             // placing the PC on the row of the East Gate at the column to the left of the east Gate
            player->updateNextCoords(curMap->gateE, 78);         // updating the next coords to match the current coords
            curMap->charGrid[player->row][player->col] = player; // place the player on the new map
            enqueueAllChars(&(WM->charQueue), curMap);           // enqueue all characters on the new map
        }
        else
        { // MAP DOESN'T EXIST
            WM->mapGrid[w_row][w_col] = generateNewMap(WM, w_row, w_col);
            curMap = WM->mapGrid[w_row][w_col];
            player->updateCoords(curMap->gateE, 78);                                         // placing the PC on the row of the East Gate at the column to the left of the east Gate
            player->updateNextCoords(curMap->gateE, 78);                                     // updating the next coords to match the current coords
            curMap->charGrid[player->row][player->col] = player;                             // place the player on the new map
            GQenqueue(&(WM->charQueue), player->row, player->col, player->nextCost, player); // enqueue the player to the character queue
            generateCharacters(curMap, WM, player, NUMTRAINERS);
        }
    }
    else if (player->col == 79)
    { // East Gate
        w_col--;
        if (WM->mapGrid[w_row][w_col] != NULL)
        { // MAP ALREADY EXISTS
            curMap = WM->mapGrid[w_row][w_col];
            player->updateCoords(curMap->gateW, 1);              // placing the PC on the row of the West Gate at the column to the right of the east Gate
            player->updateNextCoords(curMap->gateW, 1);          // updating the next coords to match the current coords
            curMap->charGrid[player->row][player->col] = player; // place the player on the new map
            enqueueAllChars(&(WM->charQueue), curMap);           // enqueue all characters on the new map
        }
        else
        { // MAP DOESN'T EXIST
            WM->mapGrid[w_row][w_col] = generateNewMap(WM, w_row, w_col);
            curMap = WM->mapGrid[w_row][w_col];
            player->updateCoords(curMap->gateW, 1);                                          // placing the PC on the row of the West Gate at the column to the right of the east Gate
            player->updateNextCoords(curMap->gateW, 1);                                      // updating the next coords to match the current coords
            curMap->charGrid[player->row][player->col] = player;                             // place the player on the new map
            GQenqueue(&(WM->charQueue), player->row, player->col, player->nextCost, player); // enqueue the player to the character queue
            generateCharacters(curMap, WM, player, NUMTRAINERS);
        }
    }
    else
    {
        mvprintw(0, 0, "INVALID GATE LOCATION IN 'enterGate()' FUNCTION)");
    }

    curMap->printBoard();
    return 0;
}

// takes a character object and finds the spot it will move next
int findNextPos(NPC *character, Map *m, WorldMap *WM, PC *player)
{
    if (character->type == CT_HIKER)
    {
        genCostMap(WM->hiker_CM, CT_HIKER, m, *player);
        int nextVal = MAX;
        for (int i = -1; i < 2; i++)
        {
            for (int j = -1; j < 2; j++)
            {
                if (!(i == 0 && j == 0))
                {
                    if (WM->hiker_CM[character->row + i][character->col + j] < nextVal)
                    {
                        nextVal = WM->hiker_CM[character->row + i][character->col + j];
                        character->updateNextCoords(character->row + i, character->col + j);
                        character->nextCost = calc_Cost(m->Tgrid[character->nextRow][character->nextCol], CT_HIKER);
                    }
                }
            }
        }
        return 0;
    }
    else if (character->type == CT_RIVAL)
    {
        genCostMap(WM->rival_CM, CT_RIVAL, m, *player);
        int nextVal = MAX;
        for (int i = -1; i < 2; i++)
        {
            for (int j = -1; j < 2; j++)
            {
                if (!(i == 0 && j == 0))
                {
                    if (WM->rival_CM[character->row + i][character->col + j] < nextVal)
                    {
                        nextVal = WM->rival_CM[character->row + i][character->col + j];
                        character->updateNextCoords(character->row + i, character->col + j);
                        character->nextCost = calc_Cost(m->Tgrid[character->nextRow][character->nextCol], CT_RIVAL);
                    }
                }
            }
        }
        return 0;
    }
    else if (character->type == CT_PACER)
    {
        // CHECK COST TO MOVE IN YOUR CURRENT DIRECTION, OR OPPOSITE DIRECTION OF CURRENT DIRECTION IS NOT GOOD
        if (character->direction == NORTH)
        {
            if (calc_Cost(m->Tgrid[character->row - 1][character->col], CT_PACER) != MAX)
            {
                character->updateNextCoords(character->row - 1, character->col);
                character->nextCost = calc_Cost(m->Tgrid[character->row - 1][character->col], CT_PACER);
                return 0;
            }
            else if (calc_Cost(m->Tgrid[character->row + 1][character->col], CT_PACER) != MAX)
            {
                character->direction = 2;
                character->updateNextCoords(character->row + 1, character->col);
                character->nextCost = calc_Cost(m->Tgrid[character->row + 1][character->col], CT_PACER);
                return 0;
            }
        }

        else if (character->direction == EAST)
        {
            if (calc_Cost(m->Tgrid[character->row][character->col + 1], CT_PACER) != MAX)
            {
                character->updateNextCoords(character->row, character->col + 1);
                character->nextCost = calc_Cost(m->Tgrid[character->row][character->col + 1], CT_PACER);
                return 0;
            }
            else if (calc_Cost(m->Tgrid[character->row][character->col - 1], CT_PACER) != MAX)
            {
                character->direction = 3;
                character->updateNextCoords(character->row, character->col - 1);
                character->nextCost = calc_Cost(m->Tgrid[character->row][character->col - 1], CT_PACER);
                return 0;
            }
        }

        else if (character->direction == SOUTH)
        {
            if (calc_Cost(m->Tgrid[character->row + 1][character->col], CT_PACER) != MAX)
            {
                character->updateNextCoords(character->row + 1, character->col);
                character->nextCost = calc_Cost(m->Tgrid[character->row + 1][character->col], CT_PACER);
                return 0;
            }
            else if (calc_Cost(m->Tgrid[character->row - 1][character->col], CT_PACER) != MAX)
            {
                character->direction = 0;
                character->updateNextCoords(character->row - 1, character->col);
                character->nextCost = calc_Cost(m->Tgrid[character->row - 1][character->col], CT_PACER);
                return 0;
            }
        }

        else if (character->direction == WEST)
        {
            if (calc_Cost(m->Tgrid[character->row][character->col - 1], CT_PACER) != MAX)
            {
                character->updateNextCoords(character->row, character->col - 1);
                character->nextCost = calc_Cost(m->Tgrid[character->row][character->col - 1], CT_PACER);
                return 0;
            }
            else if (calc_Cost(m->Tgrid[character->row][character->col + 1], CT_PACER) != MAX)
            {
                character->direction = 1;
                character->updateNextCoords(character->row, character->col + 1);
                character->nextCost = calc_Cost(m->Tgrid[character->row][character->col + 1], CT_PACER);
                return 0;
            }
        }

        // IF CT_PACER CANT GO FORWARD OR BACKWARD, KEEP CURRENT SPOT
        character->updateNextCoords(character->row, character->col);
        character->nextCost = calc_Cost(m->Tgrid[character->row][character->col], CT_PACER);
        return 0;
    }
    else if (character->type == CT_WANDERER)
    {
        int numIterations = 0;
        do
        {
            character->moveOneDir(m);
            numIterations++;
            if (calc_Cost(m->Tgrid[character->nextRow][character->nextCol], CT_WANDERER) == MAX || m->Tgrid[character->nextRow][character->nextCol] != m->Tgrid[character->row][character->col])
            {
                character->direction = rand() % 4; // choose a random direction
            }

        } while ((character->nextCost == MAX || m->Tgrid[character->nextRow][character->nextCol] != m->Tgrid[character->row][character->col]) && numIterations < 15); // the 15 is so that if by chance it is stuck in a 1x1 hole, it doesn't go into an infinite loop

        // if the reason that the above loop broke was because the wanderer is stuck in a 1 by 1 hole, make it stay where it is
        if (numIterations == 15)
        {
            character->updateNextCoords(character->row, character->col);
            character->nextCost = calc_Cost(m->Tgrid[character->row][character->col], CT_WANDERER);
        }
        return 0;
    }
    else if (character->type == CT_EXPLORER)
    {
        int numIterations = 0;
        do
        {
            character->moveOneDir(m);
            numIterations++;
            if (character->nextCost == MAX)
            {
                character->direction = rand() % 4;
            }

        } while ((calc_Cost(m->Tgrid[character->nextRow][character->nextCol], CT_EXPLORER) == MAX) && numIterations < 15); // the 15 is so that if by chance it is stuck in a 1x1 hole, it doesn't go into an infinite loop

        // if the reason that the above loop broke was because the wanderer is stuck in a 1 by 1 hole, make it stay where it is
        if (numIterations == 5)
        {
            character->updateNextCoords(character->row, character->col);
            character->nextCost = calc_Cost(m->Tgrid[character->row][character->col], CT_EXPLORER);
        }
        return 0;
    }
    else if (character->type == CT_SWIMMER)
    {
        if (player->playerByWater(m))
        { // if the player is by the water, make swimmer path to player
            int rowDist = player->row - character->row;
            int colDist = player->col - character->col;
            if (rowDist > 0 && calc_Cost(m->Tgrid[character->row + 1][character->col], CT_SWIMMER) != MAX)
            { // If player is down and swimmer can move down, move down
                character->updateNextCoords(character->row + 1, character->col);
                character->nextCost = calc_Cost(m->Tgrid[character->nextRow][character->nextCol], CT_SWIMMER);
            }
            else if (rowDist < 0 && calc_Cost(m->Tgrid[character->row - 1][character->col], CT_SWIMMER) != MAX)
            { // If player is up and swimmer can move up, move up
                character->updateNextCoords(character->row - 1, character->col);
                character->nextCost = calc_Cost(m->Tgrid[character->nextRow][character->nextCol], CT_SWIMMER);
            }
            else if (colDist > 0 && calc_Cost(m->Tgrid[character->row][character->col + 1], CT_SWIMMER) != MAX)
            { // If player is right and swimmer can move right, move right
                character->updateNextCoords(character->row, character->col + 1);
                character->nextCost = calc_Cost(m->Tgrid[character->nextRow][character->nextCol], CT_SWIMMER);
            }
            else if (colDist < 0 && calc_Cost(m->Tgrid[character->row][character->col - 1], CT_SWIMMER) != MAX)
            { // If player is left and swimmer can move left, move left
                character->updateNextCoords(character->row, character->col - 1);
                character->nextCost = calc_Cost(m->Tgrid[character->nextRow][character->nextCol], CT_SWIMMER);
            }
            else
            { // else, keep swimmer where he is
                character->updateNextCoords(character->row, character->col);
                character->nextCost = calc_Cost(m->Tgrid[character->row][character->col], CT_SWIMMER);
            }
        }
        else
        { // else randomly move the swimmer
            int numIterations = 0;
            character->direction = rand() % 4;
            do
            {
                character->moveOneDir(m);
                numIterations++;
                if (character->nextCost == MAX)
                {
                    character->direction = rand() % 4; // move in a random direction
                }

            } while ((calc_Cost(m->Tgrid[character->nextRow][character->nextCol], CT_SWIMMER) == MAX) && numIterations < 15); // the 15 is so that if the swimmer is stuck in a 1x1 hole, you dont go into an infinite loop

            // if the reason that the above loop broke was because the swimmer is stuck in a 1 by 1 hole, make it stay where it is
            if (numIterations == 15)
            {
                character->updateNextCoords(character->row, character->col);
                character->nextCost = calc_Cost(m->Tgrid[character->row][character->col], CT_SWIMMER);
            }
        }
        return 0;
    }
    else if (character->type == CT_SENTRY)
    {
        character->updateNextCoords(character->row, character->col);
        character->nextCost = calc_Cost(m->Tgrid[character->nextRow][character->nextCol], CT_SENTRY);
        return 0;
    }
    else
    {
        printw("INVALID CHARACTER TYPE (%d) PASSED to findNextPos() function", character->type);
        return 1; // wasn't passed a valid character type
    }
}

int generateCharacters(Map *m, WorldMap *WM, PC *player, int numTrainers)
{

    if (numTrainers < 0)
    {
        return 1; // invalid number of trainers
    }

    int randNPC;
    // generate random trainers
    while (numTrainers > 2)
    {
        randNPC = rand() % (CT_OTHER - 1) + 1; // generates a random number from 1 to CT_OTHER
        new NPC(randNPC, m, WM, player);
        numTrainers--;
    }

    // make sure that there are always a hiker and rival
    switch (numTrainers)
    {
    case 2:
        new NPC(CT_HIKER, m, WM, player);
        new NPC(CT_RIVAL, m, WM, player);
        break;
    case 1:
        new NPC(CT_RIVAL, m, WM, player);
        break;
    }
    return 0;
}

// returns 0 if successful and player did NOT go. returns 1 if failed, returns 2 if player went
int moveCharacters(Map *m, WorldMap *WM, PC *player, int *retVal)
{
    GQnode *retNode;

    if (!(retNode = (GQnode *)calloc(sizeof(*retNode), 1)))
    {
        return 1; // couldn't make node
    }
    GQdequeue(&(WM->charQueue), retNode);

    if (retNode->character->type != CT_PLAYER)
    {                                                          // if dequeued character is not the player
        NPC *curNPC = dynamic_cast<NPC *>(retNode->character); // cast it to an NPC

        // If NPC is already dead, just remove and ignore it (could happen when player walks into an enemy)
        if (curNPC->isDefeated == 1)
        {
            m->charGrid[curNPC->row][curNPC->col] = NULL;
            free(retNode);
            return 0;
        }

        // if the character is an NPC and is moving into the player, battle that NPC
        if (curNPC->nextCol == player->col && curNPC->nextRow == player->row && curNPC->isDefeated == 0)
        {
            // enter battle with the NPC
            player->battle(curNPC);
        }
        // dealing with swimmer who attack the player if they are in an adjacent cell to the player
        else if (curNPC->type == CT_SWIMMER && abs(curNPC->nextCol - player->col) <= 1 && abs(curNPC->nextRow - player->row) <= 1 && curNPC->isDefeated == 0)
        {
            // if the swimmer is in an adjacent cell to the player, attack the player
            player->battle(curNPC);
        }

        // only move if there is no LIVE character already in the spot you want to go to
        else if (m->charGrid[curNPC->nextRow][curNPC->nextCol] == NULL || dynamic_cast<NPC *>(m->charGrid[curNPC->nextRow][curNPC->nextCol])->isDefeated == 1)
        { // I CAN DYNAMIC CAST THE CHARACTER HERE BECUASE PREVIOUSLY I CHECKED IF IT WAS THE PC AND IF IT WAS, I WOULD BATTLE IT
            // remove the character from the charMap and update node's coords
            m->charGrid[curNPC->row][curNPC->col] = NULL;
            curNPC->updateCoords(curNPC->nextRow, curNPC->nextCol);
        }
        else if (curNPC->type != CT_PACER)
        {
            curNPC->direction = rand() % 4; // chose another random direction if the character is not a pacer because pacers can only go back and forth
        }

        // if the character isnt dead, update nodes next coords and next cost (this is done inside the findNextPos function). Then reenqueue the node and update the charMap
        if (curNPC->isDefeated != 1)
        {
            findNextPos(curNPC, m, WM, player);
            GQenqueue(&(WM->charQueue), curNPC->row, curNPC->col, retNode->value + curNPC->nextCost, curNPC); // the character is enqueued with a value of its old value PLUS the new cost to move to the new terrain
            m->charGrid[curNPC->row][curNPC->col] = curNPC;
        }
    }
    else
    { // if the ret node is the player, deal with it in main
        *retVal = retNode->value;
        free(retNode);
        return 2; // player went
        // printGQ(&(WM->charQueue)); //FOR TESTING REMOVETHIS
    }
    free(retNode);
    return 0;
}

///////////////////////////////////////////////////////////////////UI FUNCTIONS//////////////////////////////////////////////////////////////////////////////////

int displayTrainerList(WorldMap *WM, Map *m, PC *player)
{
    clearScreen_top();
    createPanel(3, 20, 10, 69);
    mvprintw(4, 35, "TRAINER LIST");
    int numTrainers = 0; // keep track of how many trainers have been put in the array (and what index the next trainer should be input at)

    // first count the number of trainers on the map
    for (int i = 0; i < 21; i++)
    {
        for (int j = 0; j < 80; j++)
        {
            if (m->charGrid[i][j] != NULL && m->charGrid[i][j]->type != CT_PLAYER)
            {
                numTrainers++;
            }
        }
    }

    // create an array of pointers to trainers and add all the trainers on the map to that array
    Character *trainers[numTrainers];
    int trainersAdded = 0;
    for (int i = 0; i < 21; i++)
    {
        for (int j = 0; j < 80; j++)
        {
            if (m->charGrid[i][j] != NULL && m->charGrid[i][j]->type != CT_PLAYER)
            {
                trainers[trainersAdded] = m->charGrid[i][j];
                trainersAdded++;
            }
        }
    }

    int startIndex = 0;
    int usrKey;
    int Xdist, Ydist;
    Character currentCharacter;
    do
    {
        for (int i = 0; i < 11; i++)
        { // only print 11 characters to the screen at a time
            if (i + startIndex >= numTrainers)
            { // if you reach the end of your trainer array, stop printing
                break;
            }
            currentCharacter = *trainers[startIndex + i];

            // calculate distance from player to trainer
            Xdist = currentCharacter.col - player->col;
            Ydist = currentCharacter.row - player->row;

            // PRINT STUFF
            // print character type
            mvprintw(i + 6, 30, "%c ", CT_to_Char(currentCharacter.type));

            // print North/South info
            if (Ydist > 0)
            {
                printw("%d South, ", abs(Ydist));
            }
            else
            {
                printw("%d North, ", abs(Ydist));
            }
            // print East/West info
            if (Xdist > 0)
            {
                printw("%d East", abs(Xdist));
            }
            else
            {
                printw("%d West", abs(Xdist));
            }
        }

        usrKey = getch();
        if (usrKey == KEY_DOWN && startIndex + 11 < numTrainers)
        {
            createPanel(3, 20, 10, 69);
            mvprintw(4, 35, "TRAINER LIST");
            startIndex += 11;
        }
        else if (usrKey == KEY_UP && startIndex > 0)
        {
            createPanel(3, 20, 10, 69);
            mvprintw(4, 35, "TRAINER LIST");
            startIndex -= 11;
        }
    } while (usrKey != KEY_ESC);
    clearScreen();
    clearScreen_top();
    return 0;
}

int playGym(Gym *gym, PC *player, WorldMap *WM, Map *m)
{
    // gym->printGym();
    GQdequeueAll(&(WM->charQueue));
    int tmpRow = player->row;
    int tmpCol = player->col;

    m->charGrid[player->row][player->col] = NULL;
    player->updateCoords(20, 20);
    player->updateNextCoords(20, 20);
    gym->charGrid[20][20] = player;
    for (NPC leader : gym->leaders)
    {
        GQenqueue(&(WM->charQueue), leader.row, leader.col, leader.nextCost, &leader);
    }
    GQenqueue(&(WM->charQueue), player->row, player->col, 0, player);
    // enqueueAllChars(&(WM->charQueue), );
    // mvprintw(29, 0, "Size of the queue: %d", WM->charQueue.length);
    // for (int i = 0; i < WM->charQueue.length; i++)
    // {
    // mvprintw(30 + i, 0, "Character %s", wm->);
    // }
    gym->printGym();

    bool left = false;
    int key;
    // int skipLastTurn;
    int numDef = 0;
    while (!left)
    {
        mvprintw(22, 0, "%d", numDef);
        // curMap = WM.mapGrid[w_row][w_col];
        key = getch();
        // skipLastTurn = 0;
        int usrKey, usrNum, status;
        switch (key)
        {
        case 'B':
        {
            createPanel(12, 20, 10, 69);
            mvprintw(13, 13, "Choose an item from your bag: ");

            for (int i = 0; i < (int)player->bag.size(); i++)
            {
                attron(COLOR_PAIR(COLOR_YELLOW));
                attron(A_BLINK);
                mvprintw(15 + i, 13, "%d. %s : %d", i + 1, player->bag[i].name.c_str(), player->bag[i].quantity);
                attroff(COLOR_PAIR(COLOR_YELLOW));
                attroff(A_BLINK);
            }

            while (1)
            {
                usrKey = getch();
                usrNum = usrKey - 48;
                switch (usrNum)
                {
                case 1:
                {
                    mvprintw(13, 13, "*                                                          *");
                    mvprintw(13, 13, "You can't use a pokeball!! Press space to continue.");
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }
                    break;
                }

                case 2:
                {
                    status = choosePokemon(player->party, 1);
                    if (status == -1)
                    {
                        break;
                    }
                    else
                    {
                        player->usePotion(&player->party[status]);
                    }
                    break;
                }

                case 3:
                {
                    status = choosePokemon(player->party, 2);
                    if (status == -1)
                    {
                        break;
                    }
                    else
                    {
                        player->useRevive(&player->party[status]);
                    }

                    break;
                }
                }
                break;
            }
            break;
        }

        case '7':
        case 'y':
            // attempt to move PC one cell to the upper left (should check for capability in the move characters function)
            if (calc_Cost(gym->Ggrid[player->row - 1][player->col - 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move up left");
                player->updateNextCoords(player->row - 1, player->col - 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move up left");
            }
            break;
        case '8':
        case 'k':
            // attempt to move PC one cell up (should check for capability in the move characters function)
            if (calc_Cost(gym->Ggrid[player->row - 1][player->col], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move up");
                player->updateNextCoords(player->row - 1, player->col);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move up");
            }
            break;
        case '9':
        case 'u':
            // attempt to move PC one cell to the upper right (should check for capability in the move characters function)
            if (calc_Cost(gym->Ggrid[player->row - 1][player->col + 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move up right");
                player->updateNextCoords(player->row - 1, player->col + 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move up right");
            }
            break;
        case '6':
        case 'l':
            // attempt to move PC one cell to the right (should check for capability in the move characters function)
            if (calc_Cost(gym->Ggrid[player->row][player->col + 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move right");
                player->updateNextCoords(player->row, player->col + 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move right");
            }
            break;
        case '3':
        case 'n':
            // attempt to move PC one cell to the lower right (should check for capability in the move characters function)
            if (calc_Cost(gym->Ggrid[player->row + 1][player->col + 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move down right");
                player->updateNextCoords(player->row + 1, player->col + 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move down right");
            }
            break;
        case '2':
        case 'j':
            // attempt to move PC one cell down (should check for capability in the move characters function)
            if (calc_Cost(gym->Ggrid[player->row + 1][player->col], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move down");
                player->updateNextCoords(player->row + 1, player->col);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move down");
            }
            break;
        case '1':
        case 'b':
            // attempt to move PC one cell to the lower left
            if (calc_Cost(gym->Ggrid[player->row + 1][player->col - 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move down left");
                player->updateNextCoords(player->row + 1, player->col - 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move down left");
            }
            break;
        case '4':
        case 'h':
            // attempt to move PC one cell to the left
            if (calc_Cost(gym->Ggrid[player->row][player->col - 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move left");
                player->updateNextCoords(player->row, player->col - 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move left");
            }
            break;
        case '5':
        case ' ':
        case '.':
            // rest for a turn. NPCs still move
            clearScreen_top();
            mvprintw(0, 0, "rest");
            break;
        case KEY_LEFT:
        {
            left = true;
        }
        default:
            // print out key that was pressed
            clearScreen_top();
            mvprintw(0, 0, "Unknown Key: %d", key);
            // skipLastTurn = 1;
        }

        // Dealing with PC movement
        if (gym->charGrid[player->nextRow][player->nextCol] != NULL && gym->charGrid[player->nextRow][player->nextCol]->type != CT_PLAYER && dynamic_cast<NPC *>(gym->charGrid[player->nextRow][player->nextCol])->isDefeated == 0)
        { // I CAN DYNAMIC CAST THE CHARACTER HERE BECUASE EARLIER IN THE CONDITIONAL I CHECKED IF IT WAS THE PC
            int status = player->battle(dynamic_cast<NPC *>(gym->charGrid[player->nextRow][player->nextCol]));
            if (status == 5)
            {
                numDef++;
            }
        }
        // only move if there is no LIVE character already in the spot you want to go to
        else
        {
            // remove the character from the charMap and update node's coords
            gym->charGrid[player->row][player->col] = NULL;
            player->updateCoords(player->nextRow, player->nextCol);
        }

        GQenqueue(&(WM->charQueue), player->row, player->col, player->nextCost, player); // the character is enqueued with a value of its old value PLUS the new cost to move to the new terrain
        gym->charGrid[player->row][player->col] = player;

        // while (skipLastTurn == 0 && moveCharacters(curMap, &WM, &player, &retVal) != 2)
        // { // the move characters function returns 0 if an NPC is dequeued and a -1 if it fails
        //   // curMap->printBoard();
        //   // mvprintw(25,0, "%d", rand()%100); refresh(); FORTESTING
        // }
        gym->printGym();

        if (numDef >= (int)gym->leaders.size())
        {
            createPanel(3, 20, 10, 69);
            mvprintw(12, 10, "***********************************************************");
            mvprintw(13, 13, "Tough battle, but you've done well!");
            while (1)
            {
                usrKey = getch();
                if (usrKey == ' ')
                {
                    break;
                }
            }
            mvprintw(13, 10, "*                                                         *");
            mvprintw(13, 13, "You have obtained the %s!", gym->badge.c_str());
            while (1)
            {
                usrKey = getch();
                if (usrKey == ' ')
                {
                    break;
                }
            }
            left = true;
            player->numBadges++;
        }
    }
    gym->charGrid[player->row][player->col] = NULL;
    GQdequeueAll(&(WM->charQueue));
    m->charGrid[tmpRow][tmpCol] = player;
    player->updateCoords(tmpRow, tmpCol);
    player->updateNextCoords(tmpRow, tmpCol);
    enqueueAllChars(&(WM->charQueue), m);
    /*
    GQdequeueAll(&(WM->charQueue));
        int tmpRow = player->row;
        int tmpCol = player->col;

        m->charGrid[player->row][player->col] = NULL;
        player->updateCoords(10, 10);
        player->updateNextCoords(10, 10);
        gym->charGrid[10][10] = player;
        for (NPC leader : gym->leaders)
        {
            GQenqueue(&(WM->charQueue), leader.row, leader.col, leader.nextCost, &leader);
        }
        GQenqueue(&(WM->charQueue), player->row, player->col, 0, player);*/

    return 0;
}

int enterBuilding(WorldMap *WM, Map *m, PC *player, int buildingType)
{
    int usrKey;
    clearScreen_top();
    if (buildingType == TT_PCENTER)
    {
        createPanel(3, 20, 10, 69);

        mvprintw(4, 15, "welcome to the Pokecenter! press '<' to leave");

        for (int i = 0; i < (int)player->party.size(); i++)
        {
            player->party[i].HP = player->party[i].max_HP;
        }
        do
        {
            usrKey = getch();
        } while (usrKey != KEY_LEFT);
        clearScreen_top();
        mvprintw(0, 0, "you left the pokecenter");
        return 0;
    }
    else if (buildingType == TT_PMART)
    {
        createPanel(3, 20, 10, 69);
        mvprintw(4, 15, "welcome to the PokeMart! press '<' to leave");
        for (int i = 0; i < (int)player->party.size(); i++)
        {
            player->bag[i].quantity = 10;
        }
        do
        {
            usrKey = getch();
        } while (usrKey != KEY_LEFT);
        clearScreen_top();
        mvprintw(0, 0, "you left the PokeMart");
        return 0;
    }
    else if (buildingType >= 13 && buildingType <= 21)
    {
        // createPanel(3, 43, 10, 69);
        // bool matchDone = false;
        mvprintw(25, 0, "Attempts to enter the gym");
        // Gym *curGym;
        if (buildingType == TT_PEWTER)
        {
            if (gyms[0]->leaderSet == false)
            {
                NPC *brock = new NPC(CT_LEADER, 6, "Brock", w_row, w_col);
                gyms[0]->charGrid[brock->row][brock->col] = brock;
                gyms[0]->leaders.push_back(*brock);
                gyms[0]->leaderSet = true;
            }

            playGym(gyms[0], player, WM, m);

            // curGym = gyms[0];
            // mvprintw(26, 0, "Has created the leader and has placed it in the charGrid");
        }

        else if (buildingType == TT_CERULEAN)
        {
            // mvprintw(4, 15, "welcome to the Cerulean Gym! press '<' to leave");
            if (gyms[1]->leaderSet == false)
            {
                NPC *misty = new NPC(CT_LEADER, 11, "Misty", w_row, w_col);
                gyms[1]->charGrid[misty->row][misty->col] = misty;
                gyms[1]->leaders.push_back(*misty);
                gyms[1]->leaderSet = true;
            }

            playGym(gyms[1], player, WM, m);
        }

        else if (buildingType == TT_VERMILION)
        {
            // mvprintw(4, 15, "welcome to the Vermilion Gym! press '<' to leave");
            if (gyms[2]->leaderSet == false)
            {
                NPC *lt_Surge = new NPC(CT_LEADER, 13, "Lt. Surge", w_row, w_col);
                gyms[2]->charGrid[lt_Surge->row][lt_Surge->col] = lt_Surge;
                gyms[2]->leaders.push_back(*lt_Surge);
                gyms[2]->leaderSet = true;
            }

            playGym(gyms[2], player, WM, m);
        }
        else if (buildingType == TT_CELADON)
        {
            if (gyms[3]->leaderSet == false)
            {
                NPC *erika = new NPC(CT_LEADER, 12, "Erika", w_row, w_col);
                gyms[3]->charGrid[erika->row][erika->col] = erika;
                gyms[3]->leaders.push_back(*erika);
                gyms[3]->leaderSet = true;
            }

            playGym(gyms[3], player, WM, m);
        }
        else if (buildingType == TT_FUSHSIA)
        {
            if (gyms[4]->leaderSet == false)
            {
                NPC *koga = new NPC(CT_LEADER, 4, "Koga", w_row, w_col);
                gyms[4]->charGrid[koga->row][koga->col] = koga;
                gyms[4]->leaders.push_back(*koga);
                NPC *janine = new NPC(CT_LEADER, 4, "Janine", w_row, w_col);
                gyms[4]->charGrid[janine->row][janine->col] = janine;
                gyms[4]->leaders.push_back(*janine);
                gyms[4]->leaderSet = true;
            }

            playGym(gyms[4], player, WM, m);
        }
        else if (buildingType == TT_SAFFRON)
        {
            if (gyms[5]->leaderSet == false)
            {
                NPC *sabrina = new NPC(CT_LEADER, 14, "Sabrina", w_row, w_col);
                gyms[5]->charGrid[sabrina->row][sabrina->col] = sabrina;
                gyms[5]->leaders.push_back(*sabrina);
                gyms[5]->leaderSet = true;
            }

            playGym(gyms[5], player, WM, m);
        }
        else if (buildingType == TT_CINNABAR)
        {
            if (gyms[6]->leaderSet == false)
            {
                NPC *blane = new NPC(CT_LEADER, 10, "blane", w_row, w_col);
                gyms[6]->charGrid[blane->row][blane->col] = blane;
                gyms[6]->leaders.push_back(*blane);
                gyms[6]->leaderSet = true;
            }

            playGym(gyms[6], player, WM, m);
        }
        else if (buildingType == TT_VIRIDIAN)
        {
            if (gyms[7]->leaderSet == false)
            {
                NPC *giovanni = new NPC(CT_LEADER, 5, "Giovanni", w_row, w_col);
                gyms[7]->charGrid[giovanni->row][giovanni->col] = giovanni;
                gyms[7]->leaders.push_back(*giovanni);
                NPC *blue = new NPC(CT_LEADER, rand() % 19, "Blue", w_row, w_col);
                gyms[7]->charGrid[blue->row][blue->col] = blue;
                gyms[7]->leaders.push_back(*blue);
                gyms[7]->leaderSet = true;
            }

            playGym(gyms[7], player, WM, m);
        }
        else if (buildingType == TT_ELITEFOUR)
        {
            if (gyms[8]->leaderSet == false)
            {
                NPC *lorelai = new NPC(CT_LEADER, 15, "Lorelai", w_row, w_col);
                gyms[8]->charGrid[lorelai->row][lorelai->col] = lorelai;
                gyms[8]->leaders.push_back(*lorelai);
                NPC *bruno = new NPC(CT_LEADER, 2, "Bruno", w_row, w_col);
                gyms[8]->charGrid[bruno->row][bruno->col] = bruno;
                gyms[8]->leaders.push_back(*bruno);
                NPC *agatha = new NPC(CT_LEADER, 8, "Agatha", w_row, w_col);
                gyms[8]->charGrid[agatha->row][agatha->col] = agatha;
                gyms[8]->leaders.push_back(*agatha);
                gyms[8]->leaderSet = true;
            }

            playGym(gyms[8], player, WM, m);
        }

        // mvprintw(27, 0, "Attempts to print gym");
        // curGym->printGym();
        // do
        // {
        //     usrKey = getch();
        // } while (usrKey != KEY_LEFT);
    }

    else
    {
        clearScreen_top();
        mvprintw(0, 0, "INVALID BUILDING TYPE");
        return 1; // invalid building type
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////MAIN/////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    srand(time(NULL));
    int gameOver = 0; // keep track of when the game should end
    int key;
    int skipLastTurn = 0;
    int retVal = 0;
    Map *curMap;

    // Gym testGym;
    // testGym.gymGen(1, 50);

    // initialize the number of trainers
    if (argc == 1)
    {
        NUMTRAINERS = 10;
    }
    else if (argc == 2)
    {
        NUMTRAINERS = atoi(argv[1]);
    }
    else
    {
        printf("ERROR: Too many arguments in command line.");
    }

    // Parse all the info from the CSV files
    pokemonVector = parsePokemon();
    movesVector = parseMoves();
    pokeMovesVector = parsePokemonMoves();
    pokeSpeciesVector = parsePokemonSpecies();
    experienceVector = parseExperience();
    typeNamesVector = parseTypeNames();
    pokeStatsVector = parsePokemonStats();
    statsVector = parseStats();
    pokeTypesVector = parsePokemonTypes();

    // for (int i = 0; i < 21; i++)
    // {
    //     for (int j = 0; j < 40; j++)
    //     {
    //         std::cout << CT_to_char(pewterGym->Ggrid[i][j]);
    //     }
    //     std::cout << std::endl;
    // }
    // return 0;

    // for ncurses
    initTerminal();

    // pewterGym->printBoard();

    // create the first map
    WorldMap WM;

    Gym *pewterGym = new Gym(0);
    Gym *ceruleanGym = new Gym(1);
    Gym *vermilionGym = new Gym(2);
    Gym *celadonGym = new Gym(3);
    Gym *fuchsiaGym = new Gym(4);
    Gym *saffronGym = new Gym(5);
    Gym *cinnabarGym = new Gym(6);
    Gym *viridianGym = new Gym(7);
    Gym *eliteFour = new Gym(8);
    gyms[0] = pewterGym;
    gyms[1] = ceruleanGym;
    gyms[2] = vermilionGym;
    gyms[3] = celadonGym;
    gyms[4] = fuchsiaGym;
    gyms[5] = saffronGym;
    gyms[6] = cinnabarGym;
    gyms[7] = viridianGym;
    gyms[8] = eliteFour;

    // mvprintw(24, 0, "(%d, %d)", (200 - gyms[0]->w_row), (gyms[0]->w_col - 200));

    // mvprintw(24, 0, "(%d, %d)", gyms[0]->w_row, gyms[0]->w_col);

    WM.mapGrid[200][200] = generateNewMap(&WM, 200, 200);

    // initialize the player
    PC player(&WM, *WM.mapGrid[200][200], 200, 200);
    GQenqueue(&(WM.charQueue), player.row, player.col, 0, &player);
    WM.mapGrid[w_row][w_col]->charGrid[player.row][player.col] = &player; // putting the player character into the current map

    generateCharacters(WM.mapGrid[w_row][w_col], &WM, &player, NUMTRAINERS);
    // print board
    WM.mapGrid[w_row][w_col]->printBoard();

    int randomPk1 = rand() % 1091;
    int randomPk2 = rand() % 1091;
    int randomPk3 = rand() % 1091;

    createPanel(3, 20, 10, 69);

    mvprintw(5, 13, "Welcome to Kanto! Please pick a starter Pokemon:");
    mvprintw(7, 13, "1: %s", pokemonVector[randomPk1].identifier.c_str());
    mvprintw(9, 13, "2: %s", pokemonVector[randomPk2].identifier.c_str());
    mvprintw(11, 13, "3: %s", pokemonVector[randomPk3].identifier.c_str());

    int choice;
    bool chosen = false;

    while (!chosen)
    {
        key = getch();
        switch (key)
        {
        case '1':
            choice = randomPk1;
            chosen = true;
            break;
        case '2':
            choice = randomPk2;
            chosen = true;
            break;
        case '3':
            choice = randomPk3;
            chosen = true;
            break;
        default:
            mvprintw(14, 13, "Please choose a valid option.");
        }
    }

    personalPokemon starterPokemon(choice);
    player.party.push_back(starterPokemon);

    while (!gameOver)
    {
        curMap = WM.mapGrid[w_row][w_col];
        key = getch();
        skipLastTurn = 0;
        int usrKey, usrNum, status;
        switch (key)
        {
        case 'P':
        {
            createPanel(3, 20, 10, 69);
            mvprintw(4, 35, "Pokemon List");
            for (int i = 0; i < (int)player.party.size(); i++)
            {
                mvprintw(6 + i, 12, "%s, HP: %d, LVL: %d", player.party[i].identifier.c_str(), player.party[i].HP, player.party[i].level);
            }
            while (1)
            {
                usrKey = getch();
                if (usrKey == 'P')
                {
                    break;
                }
            }
            break;
        }
        case 'G':
        {
            createPanel(3, 20, 10, 69);
            mvprintw(4, 35, "Gym List");
            for (int i = 0; i < 9; i++)
            {
                mvprintw(6 + i, 12, "%s: (%d, %d)", gyms[i]->name.c_str(), (200 - gyms[i]->w_row), (gyms[i]->w_col - 200));
            }
            while (1)
            {
                usrKey = getch();
                if (usrKey == 'G')
                {
                    break;
                }
            }
            break;
        }
        case 'M':
        {
            while ((int)player.party.size() < 6)
            {
                personalPokemon *randomPkmn = new personalPokemon(399, 399);
                player.party.push_back(*randomPkmn);
            }
            break;
        }
        case 'B':
        {
            createPanel(12, 20, 10, 69);
            mvprintw(13, 13, "Choose an item from your bag: ");

            for (int i = 0; i < (int)player.bag.size(); i++)
            {
                attron(COLOR_PAIR(COLOR_YELLOW));
                attron(A_BLINK);
                mvprintw(15 + i, 13, "%d. %s : %d", i + 1, player.bag[i].name.c_str(), player.bag[i].quantity);
                attroff(COLOR_PAIR(COLOR_YELLOW));
                attroff(A_BLINK);
            }

            while (1)
            {
                usrKey = getch();
                usrNum = usrKey - 48;
                switch (usrNum)
                {
                case 1:
                {
                    mvprintw(13, 13, "*                                                          *");
                    mvprintw(13, 13, "You can't use a pokeball!! Press space to continue.");
                    while (1)
                    {
                        usrKey = getch();
                        if (usrKey == ' ')
                        {
                            break;
                        }
                    }
                    break;
                }

                case 2:
                {
                    status = choosePokemon(player.party, 1);
                    if (status == -1)
                    {
                        break;
                    }
                    else
                    {
                        player.usePotion(&player.party[status]);
                    }
                    break;
                }

                case 3:
                {
                    status = choosePokemon(player.party, 2);
                    if (status == -1)
                    {
                        break;
                    }
                    else
                    {
                        player.useRevive(&player.party[status]);
                    }

                    break;
                }
                }
                break;
            }
            break;
        }

        case '7':
        case 'y':
            // attempt to move PC one cell to the upper left (should check for capability in the move characters function)
            if (calc_Cost(curMap->Tgrid[player.row - 1][player.col - 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move up left");
                player.updateNextCoords(player.row - 1, player.col - 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move up left");
            }
            break;
        case '8':
        case 'k':
            // attempt to move PC one cell up (should check for capability in the move characters function)
            if (calc_Cost(curMap->Tgrid[player.row - 1][player.col], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move up");
                player.updateNextCoords(player.row - 1, player.col);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move up");
            }
            break;
        case '9':
        case 'u':
            // attempt to move PC one cell to the upper right (should check for capability in the move characters function)
            if (calc_Cost(curMap->Tgrid[player.row - 1][player.col + 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move up right");
                player.updateNextCoords(player.row - 1, player.col + 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move up right");
            }
            break;
        case '6':
        case 'l':
            // attempt to move PC one cell to the right (should check for capability in the move characters function)
            if (calc_Cost(curMap->Tgrid[player.row][player.col + 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move right");
                player.updateNextCoords(player.row, player.col + 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move right");
            }
            break;
        case '3':
        case 'n':
            // attempt to move PC one cell to the lower right (should check for capability in the move characters function)
            if (calc_Cost(curMap->Tgrid[player.row + 1][player.col + 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move down right");
                player.updateNextCoords(player.row + 1, player.col + 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move down right");
            }
            break;
        case '2':
        case 'j':
            // attempt to move PC one cell down (should check for capability in the move characters function)
            if (calc_Cost(curMap->Tgrid[player.row + 1][player.col], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move down");
                player.updateNextCoords(player.row + 1, player.col);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move down");
            }
            break;
        case '1':
        case 'b':
            // attempt to move PC one cell to the lower left
            if (calc_Cost(curMap->Tgrid[player.row + 1][player.col - 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move down left");
                player.updateNextCoords(player.row + 1, player.col - 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move down left");
            }
            break;
        case '4':
        case 'h':
            // attempt to move PC one cell to the left
            if (calc_Cost(curMap->Tgrid[player.row][player.col - 1], CT_PLAYER) != MAX)
            {
                clearScreen_top();
                mvprintw(0, 0, "move left");
                player.updateNextCoords(player.row, player.col - 1);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "can't move left");
            }
            break;
        case KEY_RIGHT:
            // attempt to enter a pokemart or pokemon center. Works only if standing
            // on a building. Leads to a user interface for the appropriate building.
            // you may simply add a placeholder for this now, which you exit with a '<'

            if (curMap->Tgrid[player.row][player.col] == TT_PMART)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering pokemart");
                enterBuilding(&WM, curMap, &player, TT_PMART);
            }
            else if (curMap->Tgrid[player.row][player.col] == TT_PCENTER)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering pokecenter");
                enterBuilding(&WM, curMap, &player, TT_PCENTER);
            }
            else if (curMap->Tgrid[player.row][player.col] == TT_PEWTER)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering Pewter Gym");
                enterBuilding(&WM, curMap, &player, TT_PEWTER);
            }
            else if (curMap->Tgrid[player.row][player.col] == TT_CERULEAN && player.numBadges >= 1)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering Cerulean Gym");
                enterBuilding(&WM, curMap, &player, TT_CERULEAN);
            }
            else if (curMap->Tgrid[player.row][player.col] == TT_VERMILION && player.numBadges >= 2)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering Vermilion Gym");
                enterBuilding(&WM, curMap, &player, TT_VERMILION);
            }
            else if (curMap->Tgrid[player.row][player.col] == TT_CELADON && player.numBadges >= 3)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering Celadon Gym");
                enterBuilding(&WM, curMap, &player, TT_CELADON);
            }
            else if (curMap->Tgrid[player.row][player.col] == TT_FUSHSIA && player.numBadges >= 4)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering Fushia Gym");
                enterBuilding(&WM, curMap, &player, TT_FUSHSIA);
            }
            else if (curMap->Tgrid[player.row][player.col] == TT_SAFFRON && player.numBadges >= 5)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering Saffron Gym");
                enterBuilding(&WM, curMap, &player, TT_SAFFRON);
            }
            else if (curMap->Tgrid[player.row][player.col] == TT_CINNABAR && player.numBadges >= 6)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering Cinnabar Gym");
                enterBuilding(&WM, curMap, &player, TT_CINNABAR);
            }
            else if (curMap->Tgrid[player.row][player.col] == TT_VIRIDIAN && player.numBadges >= 7)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering Viridian Gym");
                enterBuilding(&WM, curMap, &player, TT_VIRIDIAN);
            }
            else if (curMap->Tgrid[player.row][player.col] == TT_ELITEFOUR && player.numBadges >= 8)
            {
                clearScreen_top();
                mvprintw(0, 0, "entering Elite Four Gym");
                enterBuilding(&WM, curMap, &player, TT_ELITEFOUR);
            }
            else
            {
                clearScreen_top();
                mvprintw(0, 0, "Cannot enter building, get more badges you noob.");
            }
            break;
        case '5':
        case ' ':
        case '.':
            // rest for a turn. NPCs still move
            clearScreen_top();
            mvprintw(0, 0, "rest");
            break;
        case 't':
            // display a list of trainers on the map, with their symbol and position relative
            // to the PC (e.g.: "r, 2 north and 14 west").
            displayTrainerList(&WM, curMap, &player);
            skipLastTurn = 1;
            break;
        case 'Q':
            gameOver = 1;
            skipLastTurn = 1;
            break;
        case 'f':
            clearScreen_top();
            mvprintw(0, 0, "Enter a map to fly to [world_row, world_col]: ");
            refresh();
            int destRow;
            int destCol;
            scanf("%d %d", &destRow, &destCol);
            if (move_to(&WM, 200 - destRow, destCol + 200, &player) != 1)
            { // if you succesfuly fly to a location, print that location. Else the function already prints an error. The offsets of 200 are to make (0,0) the center of the world
                clearScreen_top();
                mvprintw(0, 0, "Flying to (%d, %d)", destRow, destCol);
                curMap = WM.mapGrid[w_row][w_col];
            }
            curMap->printBoard();
            refresh();
            break;

        default:
            // print out key that was pressed
            clearScreen_top();
            mvprintw(0, 0, "Unknown Key: %d", key);
            skipLastTurn = 1;
        }

        // Dealing with PC movement
        if (curMap->charGrid[player.nextRow][player.nextCol] != NULL && curMap->charGrid[player.nextRow][player.nextCol]->type != CT_PLAYER && dynamic_cast<NPC *>(curMap->charGrid[player.nextRow][player.nextCol])->isDefeated == 0)
        { // I CAN DYNAMIC CAST THE CHARACTER HERE BECUASE EARLIER IN THE CONDITIONAL I CHECKED IF IT WAS THE PC
            player.battle(dynamic_cast<NPC *>(curMap->charGrid[player.nextRow][player.nextCol]));
        }
        // only move if there is no LIVE character already in the spot you want to go to
        else
        {
            // remove the character from the charMap and update node's coords
            curMap->charGrid[player.row][player.col] = NULL;
            player.updateCoords(player.nextRow, player.nextCol);
        }

        GQenqueue(&(WM.charQueue), player.row, player.col, retVal + player.nextCost, &player); // the character is enqueued with a value of its old value PLUS the new cost to move to the new terrain
        curMap->charGrid[player.row][player.col] = &player;
        curMap->printBoard();
        if (curMap->Tgrid[player.row][player.col] == TT_GATE)
        {
            enterGate(curMap, &WM, &player);
            curMap = WM.mapGrid[w_row][w_col];
        }
        else if (curMap->Tgrid[player.row][player.col] == TT_TGRASS)
        {
            if ((rand() % 10) == 1)
            {
                bool encounter = true;
                for (int i = -1; i < 2; i++)
                {
                    for (int j = -1; j < 2; j++)
                    {
                        if ((!(i == 0 && j == 0)) && curMap->charGrid[player.row + i][player.col + j] != NULL)
                        {
                            encounter = false;
                        }
                    }
                }
                if (encounter)
                {
                    player.encounterWildPokemon();
                }
            }
        }

        curMap->printBoard();
        // loop to make sure the other NPC's move until its the players turn again (nothing happens when its not the players turn)
        while (skipLastTurn == 0 && moveCharacters(curMap, &WM, &player, &retVal) != 2)
        { // the move characters function returns 0 if an NPC is dequeued and a -1 if it fails
          // curMap->printBoard();
          // mvprintw(25,0, "%d", rand()%100); refresh(); FORTESTING
        }
        curMap->printBoard();
    }

    endwin();
    return 0;
}