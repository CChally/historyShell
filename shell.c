/*=========================================================================================================
	Assignment 1 - Question 2: My Shell

	Name: shell.c

	Written by: Brett Challice 1/17/2024

	Purpose: This program is a shell interface that accepts user commands and executes each command in 
		 a separate process. It gives the user a osh> prompt to collect the command. This program
		 also implements a history feature allowing the user to see the past 5 commands, as well as
		 primitives ie. (!! and ! [num]) to execute the last entered command, or a command number in history
		 respectfully.

		ex. ! 19 executes the 19th command in history
		    !! executes the last command entered

		 The shell offers 2 custom commands: "history", and "exit"

	Usage: gcc -o sh shell.c (Compile)
	       ./sh => (Execute)

	Description of Parameters:
	-> N/A

	Subroutines/libraries required:
	-> See include statements


-------------------------------------------------------------------------------------------------------------

*/

// Includes
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>

// Prototypes
void printHistory(void);
int translate(char userInput[], char* args[], int* isBackground,int isHistoryCall);

// Macros
#define MAX_LINE 80 // The maximum length command
#define HIST_SIZE 5 // History size
#define TRUE 1
#define FALSE 0

// Globals
char histCount = 0;			// Dealing with history feature
char history[HIST_SIZE][MAX_LINE];

int DEBUG = FALSE; // Debugging flag

int main(void){

	int running = TRUE; // Shell terminator
	char userInput[MAX_LINE];
	int isBackground; // Background flag
	char* args[MAX_LINE/2 + 1]; // Command line arguments

	while(running){
		isBackground = FALSE;
		memset(userInput, '\0', sizeof(userInput)); // Reset/rewipe userInput

		printf("%d \e[0;32mosh>\033[0m ", getpid()); // Display prompt
		fflush(stdout);
		read(STDIN_FILENO, userInput, MAX_LINE); // Read user input to buffer

		if(strcmp(userInput, "exit\n") == 0) { exit(0);} // Successful exit (Exit command)
		if(strcmp(userInput, "history\n") == 0)		// History command
		{
			printHistory(); // Print history
			fflush(stdout);
			continue;  	// Continue to next command
		}
		if(strcmp(userInput,"\n") == 0){printf("ERROR: No argument was issued. \n"); continue;} // Test if enter hit without command
		int error = translate(userInput, args, &isBackground,0); // attempt to translate user command
		if(error != 0 ){continue;}

		pid_t pid = fork(); // Create child process
		int status = 0;
		if(pid < 0){printf("ERROR: Did not fork! \n"); exit(-1);} // Catch forking error
		else if(pid == 0){ // Child process
			if(DEBUG == TRUE){printf("%d is running \n", getpid());}
			fflush(stdout);
			status = execvp(*args, args); // Replace child memory space with new program
			if(status == -1){ printf("ERROR: Invalid command! \n"); exit(1);} // Check for error in exec
			exit(0);
		}
		else{ // Parent pid
			if(isBackground == 0){ // Not a background process
				if(DEBUG == TRUE){printf("Waiting for %d \n", pid);}
				fflush(stdout);
				wait(NULL); // Wait on child
			}
			else{ // background process
				if(DEBUG == TRUE){printf("Waiting for background process %d \n", pid);}
				fflush(stdout);
				do{
					status = waitpid(pid,&status,WNOHANG);
				  } while(status == 0);	// While child is still executing
			}
		}
	}

	return 0;
}

// Prints the existing history entries
void printHistory(void){
	if(histCount == 0){printf("History is empty! \n");return;} // Check for no history
	for (int i = 0; i < HIST_SIZE; i++){ // For each existing entry (5)
		int hisNum = histCount - i;
		if (hisNum <= 0){
			break;
		}
		printf("%d ", hisNum);
		int j = 0;
		while(history[i][j] != '\n' && history[i][j] != '\0'){
			printf("%c", history[i][j]);
			j++;
		}
		printf("\n");
	}
	printf("\n");
}
// Translate the command the user entered in the prompt
int translate(char userInput[], char* args[], int* isBackground, int isHistoryCall){

	if(isHistoryCall == FALSE){ // Access whether this is a history call
		if(strcmp(userInput,"!!\n") == 0){ // Execute most recent command
			if(histCount == FALSE){ // No recent commands in history
				printf("ERROR: No commands in history. \n");
				return -1; // Return error
			}
			strcpy(userInput,history[0]); //
			translate(userInput, args, isBackground, TRUE); // Recurse with history flag on
			return 0; // Success
		}
		else if((strncmp(userInput, "! ", 2) == 0) && (isdigit(userInput[2]))){ // Nth Command in History

			// Scan for following num
			char* p = userInput;
			int num;
			while(*p){ // while there are more characters to process
				if(isdigit(*p)){ // If the current pointed to char is a digit
					num = strtol(p,&p,10);
				}
				else{
					p++;
				}
			}
			// num holds the hist num
			int isFound = FALSE; // Have we found the index in the History?
			for(int id = 0; id < HIST_SIZE; id++){ // Search history buffer
				int hisNum = histCount - id;
				if((hisNum == num) && num != 0){ // Found number in history
					isFound == TRUE;
					strcpy(userInput, history[id]); //
					translate(userInput,args,isBackground,TRUE); // Recurse with specified command

					// Place the command in the history buffer as the next command
					char temp[sizeof(history[id])];
					strcpy(temp, history[id-1]);
					for(int i = (HIST_SIZE - 1); i > 0; i--){
						strcpy(history[i], history[i-1]); // Shift the indexes down
					}
					strcpy(history[0], temp);
					histCount++;
					return 0;
				}
			}
			if (isFound == FALSE){
				printf("ERROR: No command found in history. \n");
				return -1;
			}
		}
		else{ // Normal Command Case (Not !! or ! [num])
			for(int i = (HIST_SIZE -1); i > 0; i--){
				strcpy(history[i], history[i-1]);
			}
			// Append current command to the most left
			strcpy(history[0], userInput);
			histCount++; // Increment history
		}
	}
	int inputLen = strlen(userInput); // 

	// Loop through the userInput and put args into args[]
	int uPointer = -1; // Index of the start of the next command in userInput (-1 is currently unknown)
	int aPointer = 0; // Index where the next command should be put in args[]

	for (int i = 0; i < inputLen; i++){
		switch(userInput[i]){

			case '\t': // Separator cases
			case ' ':
				if(uPointer != -1){
					args[aPointer] = &userInput[uPointer];
					aPointer++;
				}
				userInput[i] = '\0';
				uPointer = -1;
				break;
					// Background case
			case '&':
				*isBackground = TRUE; // Flag on 
				userInput[i] = '\0'; // Null terminate string
				break;
					// Termination case
			case '\n':
				if(uPointer != -1){
					args[aPointer] = &userInput[uPointer];
					aPointer++;
				}
				userInput[i] = '\0';
				args[aPointer] = NULL;
				break;
			default:
				if(uPointer == -1){
					uPointer = i;
				}
				break;
		}
		args[aPointer] = NULL;
	}
	// Debugging to see if the command AND each of the flags are caught if they exist ex. ls -l -a
	if(DEBUG){
		printf("----------Arguments-----------\n");
		for(int i = 0; i < sizeof(*args); i++){
			printf("%s", args[i]);
			printf("\n");
		}
		printf("------------------------------\n");
	}
	return 0; // Successful return
}
