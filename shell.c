#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "shell.h"

int cd(char *argv[]);
char ogPath[PATH_MAX];
char *path;


void readInput(char *input){ //callum
    if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) { //reads user input, checks if input is null
        setenv("PATH", ogPath, 1);
        printf("Restored path on exit: %s\n", getenv("PATH"));
        printf("Exiting shell...\n");
        exit(0); 
    }

    // Replace new line character fgets includes, with a null terminator
	input[strcspn(input, "\n")] = '\0';
}


int parseInput(char *input, char *argv[]){ //callum
    int argc = 0; //initialises argc as int value
    for(int i = 0; input[i] != '\0'; i++){
        if(strchr("&;<>", input[i]) != NULL){
            if (i > 0 && strchr("&;<>", input[i-1]) != NULL){
                fprintf(stderr, "Invalid token: %c%c\n", input[i-1], input[i]);
                argv[0] = NULL;
                return 0;
            }
        }
    }
	
    char *token = strtok(input, " \t;&><|\n"); //splits input into tokens, along with set delimiters

    while (token != NULL && argc < MAX_ARGS - 1){ //while loop that checks if there is an existing token and space for arguments
	    argv[argc++] = token; //increases argc
        token = strtok(NULL, " \t;&><|\n"); //set next token
    }
    argv[argc] = NULL; //mark end of arguments with NULL
    return argc;
}


void addHistory(char *input, struct commandHistory *history){//Don
    if(input == NULL || input [0] == '\0') return;
    if (strlen(input) >= MAX_INPUT_SIZE) return; 

    // Copy string to the current array slot

    // Calculate next array space
    strncpy(history->commands[history->nextFree], input, MAX_INPUT_SIZE - 1);

    history->commands[history->nextFree][MAX_INPUT_SIZE-1] = '\0';
    history->nextFree = (history->nextFree + 1) % MAX_COMMANDS_HISTORY;

    // Increment number of commands
    if (history->noOfCommands < MAX_COMMANDS_HISTORY) {
        history->noOfCommands ++;
    }
}


int isHistoryInvocation(char *input){//Don
    if (input[0] == '!'){
        return 1;
    }
    return 0;
}

int getHistoryIndex(const char *input, int start, struct commandHistory *history){//Don
    char *endptr;
    long value = strtol(input + start, &endptr, 10);
    if (*endptr != '\0' || value < 0 || value > history->noOfCommands)
    {
        return -1;
    }
    return value;
}

void runHistoryInvocation(char *input, struct commandHistory *history) {
    if (history->noOfCommands == 0) {
        printf("No commands in history.\n");
        input[0] = '\0';
        return;
    }

    int index = -1;

    // !!
    if (strncmp(input, "!!", 2) == 0 && (input[2] == '\0' || input[2] == ' ' || input[2] == '\t')) {
        index = (history->nextFree - 1 + MAX_COMMANDS_HISTORY) % MAX_COMMANDS_HISTORY;
    }

    // !-N
    else if (input[0] == '!' && input[1] == '-') {
        char *num = input + 2;

        if (*num == '\0' || strspn(num, "0123456789") != strlen(num)) {
            printf("Invalid use of history.\n");
            input[0] = '\0';
            return;
        }

        int n = atoi(num);
        if (n < 0 || n > history->noOfCommands) {
            printf("Invalid use of history.\n");
            input[0] = '\0';
            return;
        }

        if (n == 0) n = 1;
        index = (history->nextFree - n + MAX_COMMANDS_HISTORY) % MAX_COMMANDS_HISTORY;
    }

    // !N
    else if (input[0] == '!') {
        char *num = input + 1;

        if (*num == '\0' || strspn(num, "0123456789") != strlen(num)) {
            printf("Invalid use of history.\n");
            input[0] = '\0';
            return;
        }

        int n = atoi(num);
        if (n <= 0 || n > history->noOfCommands) {
            printf("Invalid use of history.\n");
            input[0] = '\0';
            return;
        }

        if (history->noOfCommands == MAX_COMMANDS_HISTORY)
            index = (history->nextFree - 1 + n) % MAX_COMMANDS_HISTORY;
        else
            index = n - 1;
    }

    if (index < 0) {
        printf("Invalid use of history.\n");
        input[0] = '\0';
        return;
    }

    strncpy(input, history->commands[index], MAX_INPUT_SIZE - 1);
    input[MAX_INPUT_SIZE - 1] = '\0';
}
        
void printHistory(struct commandHistory *history){//Don

    if (history->noOfCommands < 1){
        printf("No commands in history.\n");
        return;
    }

    int currentIndex = 0;
    for (int i = 0; i < history->noOfCommands; i++){
        // Case 1 array full so we need to loop or have looped so we increment taking this into account
        if (history->noOfCommands == MAX_COMMANDS_HISTORY){
            currentIndex = ((history->nextFree + i + MAX_COMMANDS_HISTORY) % MAX_COMMANDS_HISTORY);
        } else { // Case 2 we just increment the index
            currentIndex = i;
        }
        printf("%d %s\n", i + 1, history->commands[currentIndex]);
    }
}

void saveHistory(struct commandHistory *history){   // Joseph
    char path[PATH_MAX];
    char *home = getenv("HOME");
    if (home == NULL){
        perror("Home not set");
        return;
    }
    snprintf(path, sizeof(path), "%s/.hist_list", home);
    FILE *pF = fopen(path, "w");
    if (pF== NULL){
        perror("Unable to open file!");
        return;
    }

    int currentIndex = 0;
    for(int i = 0; i < history->noOfCommands; i++){
        if (history->noOfCommands == MAX_COMMANDS_HISTORY) {  // If history is full, treat it as a circular queue
            currentIndex = ((history-> nextFree + i + MAX_COMMANDS_HISTORY) %MAX_COMMANDS_HISTORY );
        } else {
            currentIndex = i;
        }

        fprintf(pF,"%s\n", history->commands[currentIndex]); //write information into file
    }

    fclose(pF);

}

int loadHistory(struct commandHistory *history){           // Joseph
    char path[PATH_MAX];
    char *home = getenv("HOME");

    if (home == NULL) {
        perror("HOME not set");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/.hist_list", home);

    FILE *pF = fopen(path, "r");
    
    if(pF == NULL){             //checks if the file is there
        //perror("Unable to open file!");
        return 0;
    }
    char buffer[MAX_INPUT_SIZE];
    history->noOfCommands = 0;
    history->nextFree = 0;

    while(fgets(buffer, sizeof(buffer), pF) != NULL){
        buffer[strcspn(buffer, "\n")] = '\0';

        if(buffer[0] == '\0' || strspn(buffer, "\t") == strlen(buffer)) continue;

        bool valid = true;
        for (int i = 0; buffer[i] != '\0'; i++){
            if((unsigned char)buffer[i] < 32 && buffer[i] != '\t' && buffer[i] != ' '){
                valid = false;
                break;
            }
        }
        if (!valid) continue;

        strncpy(history->commands[history->nextFree], buffer, MAX_INPUT_SIZE -1);

        history->commands[history->nextFree][MAX_INPUT_SIZE - 1] = '\0';

        history->nextFree = (history->nextFree + 1) % MAX_COMMANDS_HISTORY;

        if(history->noOfCommands < MAX_COMMANDS_HISTORY)
        history->noOfCommands++;
     
    }

    fclose(pF);

    return 0;
}

/*
void printTokens(char* argv[], int argc){ //james and don
    printf("%d Token(s) found: \n" , argc); //will output how many tokens found using the argument counter
    for (int i = 0; i < argc; i++) { //create loop to loop through however big "argc" is
   	    printf(" Token: \"%s\"\n", argv[i]); //print out the found token
   	}
}
*/


existingResultType existing(char *argv[], int argc, struct commandHistory *history, char *input ){ //callum james and joseph
    if (argc > 0 && strcmp(argv[0], "exit") == 0){
        saveHistory(history);
        return RESULT_EXISTING_EXIT;
    }
    if (argc > 0 && strcmp(argv[0], "getpath") == 0){
        if (argc > 1){
            printf("getpath takes no arguments. please use: getpath\n");
            return RESULT_EXISTING_BUILT_IN;
        }
        getpath();
        return RESULT_EXISTING_BUILT_IN;
    }
    if (argc > 0 && strcmp(argv[0], "setpath") == 0){
        if (argc < 2){
            printf("setpath requires a path argument. please use: setpath <path>\n");
            return RESULT_EXISTING_BUILT_IN;
        }
        if (argc > 2){
            printf("setpath takes only one argument. please use: setpath <path>\n");
            return RESULT_EXISTING_BUILT_IN;
        }
        setpath(argv);
        return RESULT_EXISTING_BUILT_IN;
    }
    if (argc > 0 && strcmp(argv[0], "cd") == 0){
    	cd(argv);
        return RESULT_EXISTING_BUILT_IN;
    }
    if (argc > 0 && strcmp(argv[0], "history") == 0)
    {
        if (argc > 1){
            printf("history takes no arguments. please use: history\n");
            return RESULT_EXISTING_BUILT_IN;
        }
        printHistory(history);
        return RESULT_EXISTING_BUILT_IN;
    }
    if (argc > 0 && strcmp(argv[0], "alias") == 0){
        if (argc == 1){ // if argc = 1 - which just contains the phrase "alias" - display
            displayAliases();
            return RESULT_EXISTING_BUILT_IN;
        } 
        if (argc == 2){ // if 2 arguments - contains "alias <aliasName>" but no command
            printf("Invalid alias command. Please use: alias <name>='<command>'\n"); //return error
            return RESULT_EXISTING_BUILT_IN;
        }
        //else it if the argc is greater than 2:
        char *namePosition = strstr(input + strlen("alias "), argv[1]); // skips past "alias" and finds the position of the name
        char *commandPosition = namePosition + strlen(argv[1]) + 1; //extract command - skipping past the alias name
        createAlias(argv[1], commandPosition); //we call alias with the newly stored "proper" command
        return RESULT_EXISTING_BUILT_IN;
    }
    
    if (argc > 0 && strcmp(argv[0], "unalias") == 0){
        if (argc != 2){
            printf("Invalid unalias command. Please use: unalias <name>\n");
            return RESULT_EXISTING_BUILT_IN;
        }
        removeAlias(argv[1]); //call remove function
        return RESULT_EXISTING_BUILT_IN; 
    }

    if (argc > 0 && strcmp(argv[0], "save") == 0)
    {
        saveHistory(history);
        return RESULT_EXISTING_BUILT_IN;
    }
    if (argc > 0 && strcmp(argv[0], "load") == 0)
    {
        loadHistory(history);
        loadAlias();
        return RESULT_EXISTING_BUILT_IN;
    }

	return 0;
}


void execute(char *argv[]){ //callum
    pid_t pid = fork();
	if (pid == -1){
		perror("fork");
		return;
    }

	if (pid == 0){
	    if (execvp(argv[0], argv) == -1){
			perror(argv[0]);
			exit(EXIT_FAILURE);
		}
	} else {
		int status;
		waitpid(pid, &status, 0);
	}
  }


void getpath(){ //callum and kieran
    char *currentPath = getenv("PATH");
    if (currentPath != NULL){
        printf("%s\n", currentPath);
    }
}


void setpath(char *argv[]){ //callum and kieran
    setenv("PATH", argv[1], 1);
}


int cd(char *argv[]){ //everyone
	if (argv[1] == NULL){
            char *home = getenv("HOME");
            if (home == NULL){
                fprintf(stderr, "cd: HOME not set \n");
                return 1;
            }
            if (chdir(home) == -1) {
                perror("cd");
                return 1;
            }

        } else {
            struct stat path_stat;

            if (argv[2] != NULL) {
                fprintf(stderr, "cd: too many arguments. Please use: cd [directory]\n");
                return 1;
            }
            if (stat(argv[1], &path_stat) != 0) {
                perror(argv[1]);
                return 1;
            }
            if (!S_ISDIR(path_stat.st_mode)) {
                fprintf(stderr, "%s: Not a directory\n", argv[1]);
                return 1;
            }

            if (chdir(argv[1]) == -1) {
                perror(argv[1]);
                return 1;
            }
            
        }
        return 0;
}

void createAlias(char *aliasName, char *command){ //James
    //need to check if alias already exists - if it does we overide it
    for (int i = 0; i < aliasCount; i++){
        if (strcmp(aliases[i][0], aliasName) ==0){ //if the strings are equal then:
            printf("Alias already exists, overriding now \n");
            strncpy(aliases[i][1], command, MAX_INPUT_SIZE); //copy the command to the alias array - removing the old command in its place
            return;
        }
    }
    //we have hit the maximum amount of aliases:
    if (aliasCount >= MAX_ALIAS){
        printf("Maximum number of aliases reached \n");
        return;
    }
    //copy the alias name into the first dimension
    strncpy(aliases[aliasCount][0], aliasName, MAX_INPUT_SIZE);
    //copy the command into the second dimension
    strncpy(aliases[aliasCount][1], command, MAX_INPUT_SIZE);
    aliasCount++; //increment alias count
}

void removeAlias(char *aliasName){ //James
    for (int i = 0; i < aliasCount; i++){
        if (strcmp(aliases[i][0], aliasName) == 0){
            // if true, we remove the alias:
            for (int j = i; j < aliasCount - 1; j++){ //we utilise shifting to the left to overwrite the old place of the chosen alias
                strncpy(aliases[j][0], aliases[j + 1][0], MAX_INPUT_SIZE);
                strncpy(aliases[j][1], aliases[j + 1][1], MAX_INPUT_SIZE);
            }
            aliasCount--; //decrement alias count
            printf("Alias removed \n");
            return;
        }
    }
    printf("Alias not found \n");
}

void displayAliases(){ //James
    if (aliasCount == 0){ //if there is no aliases:
        printf("No aliases have been created \n");
        return;
    }
    for (int i = 0; i < aliasCount; i++){ //loop through full alias count and display each element
        printf("Alias: %s  - Command Stored: %s \n" , aliases[i][0], aliases[i][1]); 
    }
    }


char *aliasLookup(char *aliasName){ //James
    int i = 0;

    while (i < aliasCount){ // iiterate through all alias names - if it matches return a pointer to the stored command
        if (strcmp(aliases[i][0], aliasName) == 0){
            return aliases[i][1]; //return command
        }
        i++;
    }

    return NULL;

}

void saveAlias() { //kieran

    char path [PATH_MAX];
    char *home = getenv("HOME");

    if (home == NULL) {

        perror("HOME not set");
        return;
    }

    snprintf(path, sizeof(path), "%s/.aliases", home);
    FILE *pF = fopen(path, "w");

    if (pF == NULL) {

        perror("Unable to open aliases file for writing");
        return;
    }

    for (int i = 0; i < aliasCount; i++) {

        fprintf(pF, "%s %s\n", aliases[i][0], aliases[i][1]);
    }

    fclose(pF);
}

int loadAlias(){
    char path[PATH_MAX];
    char *home = getenv("HOME"); 

    if(home==NULL){
        perror("HOME not set");
        return -1;
    }

    snprintf(path,sizeof(path), "%s/.aliases", home);

    FILE *pF = fopen(path, "r");

    if(pF == NULL){
        return 0;
    }

    char buffer[MAX_INPUT_SIZE];
    aliasCount = 0; 

    while(fgets(buffer,sizeof(buffer),pF)!=NULL){
        buffer[strcspn(buffer,"\n")] ='\0';

        if(buffer[0] =='\0'|| strspn(buffer, "\t")== strlen(buffer)) 
        continue;

        bool valid=true;
    for(int i =0; buffer[i] != '\0'; i++){
        if((unsigned char)buffer[i] < 32 && buffer[i] != '\t' && buffer[i] != ' '){
            valid=false;
            break;
        }
    }
    if(!valid) continue; 

    char *spacePos = strchr(buffer, ' ');
    if(spacePos != NULL) {
        *spacePos='\0';
        char *aliasName = buffer;
        char *command= spacePos+1;

        createAlias(aliasName,command);
    }
}

fclose(pF);
return 0;
}


int main(void){ //everyone
    char input[MAX_INPUT_SIZE]; //initialises value input as array as big as the max input size
    char *argv[MAX_ARGS];
    struct commandHistory history = {0};

    path = getenv("PATH"); //callum
    if (path != NULL){
        strncpy(ogPath, path, sizeof(ogPath));
        ogPath[sizeof(ogPath) -1] = '\0';
    }

    char *home = getenv("HOME"); //kieran
    if (home != NULL){
        if (chdir(home) != 0) {
            perror("chdir");
        }
    }

    char cwd[PATH_MAX]; //kieran
    if (getcwd(cwd, sizeof(cwd)) != NULL){
        printf("Your wish is my I/O\n");
        printf("The starting directory: %s\n", cwd);
    } else {
        perror("getcwd");
    }
    loadHistory(&history);
    loadAlias();


    while(1){ //infinite loop - Callum
	    printf("mysh> "); //prints this
	    readInput(input);

        //need to save a copy before input is parsed 
        char inputCopy[MAX_INPUT_SIZE];
        strncpy(inputCopy, input, MAX_INPUT_SIZE);

        // Command History: Don
        if (!isHistoryInvocation(input)){
            addHistory(input, &history);
        } else {
            // Update input with command. If invocation is invalid return empty input.
            runHistoryInvocation(input, &history);
        }
        
       
	    int argc = parseInput(input, argv);
	    if (argc == 0) continue;

        // take out comment for stage 1 testing
           //printTokens(argv, argc);




        //James
        char *aliasCommand = NULL; //if our command is not any essential commands, proceed to alias lookup
        if (strcmp(argv[0], "exit") != 0 && strcmp(argv[0], "alias") != 0 && strcmp(argv[0], "unalias") != 0 && strcmp(argv[0], "history") != 0)
            aliasCommand = aliasLookup(argv[0]);


            //HANDLING ALIAS EXECUTION:
        if (aliasCommand != NULL){ //if an alias was found:
            char newInput[MAX_INPUT_SIZE];
            strncpy(newInput, aliasCommand, MAX_INPUT_SIZE);


            for (int i = 1; argv[i] != NULL; i++){ //Loop through until we hit NULL - start at 1 to skip the alias name
                strcat(newInput, " "); // add space between the command and the arguments
                strcat(newInput, argv[i]); //add arguments to the end of the command
            }


            int newArgc = parseInput(newInput, argv); //tokenize the full command
            existingResultType aliasResult = existing(argv, newArgc, &history, newInput);
            if (aliasResult == RESULT_EXISTING_EXIT) break; //if result = exit, break from loop
            if (aliasResult != RESULT_EXISTING_BUILT_IN) execute(argv); //else execute the built in command
            continue;
        }



        //everyone
        existingResultType returnVal = existing(argv, argc, &history, inputCopy);
        if(returnVal == RESULT_EXISTING_EXIT)
            break;
        if (returnVal == RESULT_EXISTING_BUILT_IN){
            continue;
        }
	    execute(argv);

    }

    setenv("PATH", ogPath, 1);
    printf("Restored path on exit: %s\n", getenv("PATH"));
    printf("Exiting shell...\n");
    saveHistory(&history);
    saveAlias();
    return 0;

}