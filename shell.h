
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>

#define MAX_INPUT_SIZE 512
#define MAX_ARGS 32
#define MAX_COMMANDS_HISTORY 5

typedef enum {
    RESULT_EXISTING_UKNOWN = 0,
    RESULT_EXISTING_EXIT = 1,
    RESULT_EXISTING_BUILT_IN = 2
} existingResultType;


struct commandHistory
{
    char commands[MAX_COMMANDS_HISTORY][MAX_INPUT_SIZE];
    int nextFree;
    int noOfCommands;
};


//Parsing Implementation:
void readInput(char *input);
int parseInput(char *input, char *argv[]);
void printTokens(char *argv[], int argc);
existingResultType existing(char *argv[], int argc, struct commandHistory *history, char *input);
void execute(char *argv[]);

//Path Implementation:
void getpath();
void setpath(char *argv[]);

//cd Implementation:
int cd(char *argv[]);

//History Implementation:

void addHistory(char *input, struct commandHistory *history);
int isHistoryInvocation(char *input);
void runHistoryInvocation(char *input, struct commandHistory *history);
void printHistory(struct commandHistory *history);
void saveHistory(struct commandHistory *history);
int loadHistory();


//Alias Implementation:

#define MAX_ALIAS 100

char aliases[MAX_ALIAS][2][MAX_INPUT_SIZE]; // [alias index][0 for alias name, 1 for command
int aliasCount = 0; //can be used by functions without needing to pass in

void createAlias(char *aliasName, char *command);
void removeAlias(char *aliasName);
void displayAliases();
char *aliasLookup(char *aliasName);
void saveAlias();
int loadAlias();

