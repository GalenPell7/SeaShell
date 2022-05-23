#include <stdio.h>

#include <unistd.h>

#include <string.h>

#include <stdlib.h>

#include <sys/wait.h>

#include <fcntl.h>

#include <time.h>


/**
 * @author Galen Q. Pellitteri
 * 
 * This program is intended to act as a simple shell to execute both built in commands as well as simple
 * UNIX commands. The built in functions implemented are cd and exit and the simple UNIX commands are executed
 * using execvp. This shell can also account for the & tag in order for the process to not wait. Shell does not handle piping.
 *
 */

// prototypes
void welcome_user();
void run_shell();
int chg_dir(char * );
int execute_command(char ** , int, int, int, char * , char * );

/**
 * Main function
 *
 */
int main(int argc, char * argv[]) {
  welcome_user();
  run_shell();
  return 0;
}

/**
 * General purpose function that is responsible for majority of the shells implementation.
 * Function will create variables that will be used by the execute_command function.
 * The command supplied from the user will be tokenized into inividual tokens and 
 * if the token supplied is not necassary for execvp, then it will not be included in the tokens being sent.
 */
void run_shell() {
  int amp = 0;
  int in_redirect = 0;
  char * in_file;
  char * out_file;
  int out_redirect = 0;
  size_t LINE_SIZE = 64;
  char * line = NULL;
  char ** tokens = malloc(sizeof(char * ) * LINE_SIZE);
  char * token = NULL;
  int position = 0;
  int status = 1;
  size_t characters = 0;
  while (status) { // Begin
    printf("-> ");
    characters = getline( & line, & LINE_SIZE, stdin); // read line from input.
    token = strtok(line, " \n");
    do {
      if (strchr(token, '&')) { // account for &
        amp = 1;
        token = strtok(NULL, " \n"); // DO NOT INCLUDE INTO TOKENS
      } else if (strchr(token, '<')) { // account for <
        in_redirect = 1;
        token = strtok(NULL, " \n");
        in_file = token;
        token = strtok(NULL, " \n"); // DO NOT INCLUDE INTO TOKENS
      } else if (strchr(token, '>')) { // account for >
        out_redirect = 1;
        token = strtok(NULL, " \n");
        out_file = token;
        token = strtok(NULL, " \n"); // DO NOT INCLUDE INTO TOKENS
      } else if (strstr(token, ">>")) { // account for >>
        out_redirect = 2;
        token = strtok(NULL, " \n");
        out_file = token;
        token = strtok(NULL, " \n"); // DO NOT INCLUDE INTO TOKENS
      } else {
        tokens[position] = token; // INCLUDE INTO TOKENS
        token = strtok(NULL, " \n");
        position++;
      }
    } while (token != NULL);
    if (strstr(tokens[0], "cd"))
      status = chg_dir(tokens[1]); // for cd command 
    else if (strstr(tokens[0], "exit"))
      exit(0); // for exit command
    else execute_command(tokens, amp, in_redirect, out_redirect, in_file, out_file); // for simple UNIX commands

    // reset values for the next supplied commands
    position = 0;
    amp = 0;
    in_redirect = 0;
    out_redirect = 0;
  }
}

/**
 * Welcome the user to the "Sea" Shell. ;) <-- winky face with smile
 * Contains initial prompt, some of my favorite quotes, and weather conditions.
 */
void welcome_user() {
  time_t t;
  srand((unsigned) time( & t));
  char quotes[6][200] = {
    "It is not death that a man should fear, but he should fear never beginning to live. ~ Marcus Aurelius",
    "To the living we owe respect, but to the dead we owe only the truth. ~ Voltaire",
    "Before you marry a person, you should first make them use a computer with slow Internet service to see who they really are. ~ Will Ferrell",
    "Do what is good for the bee, at which is also good for the hive. ~ Marcus Aurelius",
    "When you arise in the morning, think of what a precious privilege it is to be alive - to breathe, to think, to enjoy, to love. ~ Marcus Aurelius",
    "A fit body, a calm mind, a house full of love. These things cannot be bought — they must be earned. ~ Naval Ravikant"
  };
  printf("<===============================================>\n");
  printf("A Simple \"Sea\" Shell, Written By Galen Q. Pellitteri.\n");
  printf("-------------------------------------------------\n");
  printf("%s\n", quotes[rand() % 6]);
  printf("-------------------------------------------------\n");
  if (system("curl wttr.in") == -1) {
    perror("Error recieving weather conitions");
  }
  printf("<===============================================>\n");
}

/**
 * Function to change directory for the user.
 * If ~ is supplied, next_level gets the enviornment variable HOME.
 */
int chg_dir(char * token) {
  char * next_level = NULL;
  if (token == NULL) { //next level not supplied
    printf("Must supply directory.\n");
  } else {
    if (strstr(token, "~")) {
      next_level = getenv("HOME"); // Get the enviornment variable HOME to access home directory.
    } else next_level = token; // Just get the token.
    if (chdir(next_level) != 0)
      perror(token); // Something went wrong, most likely file or directory does not exist.
  }
  return 1;
}

/**
 * Function to execute the tokens supplie from run_shell().
 * Function will accept the tokens, the variables explaining whether or not we are uing I/O redirection
 * and names of the files supplied.
 */
int execute_command(char ** tokens, int amp, int in , int out, char * in_filename, char * out_filename) {
  pid_t pid;
  int status;
  pid = fork(); // begin fork, return 0 for the child.
  if (pid == 0) {
    if ( in == 1) {
      int file_descriptor = open(in_filename, O_RDONLY); // read file
      dup2(file_descriptor, 0);
      close(file_descriptor);
    }
    if (out == 1) {
      int file_descriptor = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666); // write to file, override file, create if it doesnt exist.
      dup2(file_descriptor, 1);
      close(file_descriptor);
    } else if (out == 2) {
      int file_descriptor = open(out_filename,O_CREAT | O_APPEND,0666); // append to file, create if it doesnt exist.
      dup2(file_descriptor, 1);
      close(file_descriptor);
    }
    if (execvp(tokens[0], tokens) == -1) { // Enter Child Process
      perror(tokens[0]); // did not execute correctly
    } else if (pid < 0) { // error forking child
      perror("Error forking child");
    }
  } else { // Parent Process
    if (amp == 0) {
      waitpid(pid, & status, 0);
    }
  }
  return 1;
}