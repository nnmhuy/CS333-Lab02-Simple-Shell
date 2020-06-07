/**********************************************************************************************************************************************************
 * Simple UNIX Shell
 * @author: nnmhuy, thduc, tnhien
 * 
 **/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LENGTH 80 // The maximum length of the commands
#define MAX_ARGUMENTS 40
char lastCommand[MAX_LENGTH];
char tmp[MAX_LENGTH];

char** getArgs(char* command) {
  strcpy(tmp, command);
	// remove trailling endline '\n' in command
	char *p = strchr(tmp, '\n');
	if (p)  *p = 0;

	char **args = (char**)malloc(MAX_ARGUMENTS * sizeof(char *));
	
	if (args == NULL) { // catch system out of memory
	  perror("malloc failed");
    exit(1);
  }

  char *separator = " ";
  char *parsed;
  int index = 0;

  parsed = strtok(tmp, separator); // split args by space
  while (parsed != NULL) {
    args[index] = parsed;
    index++;
    parsed = strtok(NULL, separator);
  }
  args[index] = NULL; // insert NULL at the end of args list
  return args;
}

bool extractRunInBackground(char** args) { // check if last args is '&' and remove it
	int index = 0;
	while (args[index + 1] != NULL) {
		++index;
	}
	if (index > 0 && strcmp(args[index], "&") == 0) {
		args[index] = NULL; // remove "&" at the end of args list
		return true; // let run in background
	}

	return false;
}

void executeNormalCommand(char* command) {
	if (strcmp(command, "") == 0) { // check for empty command
		return;
	}

	// store command to history
	strcpy(lastCommand, command);

	pid_t child_pid;
	int stat_loc;

	// extract arguments from command
	char** args = getArgs(command);

	bool runInBackground = extractRunInBackground(args);

	// create a child process and store its id in child_pid
	child_pid = fork();

	if (child_pid < 0) {
		perror("Fork failed");
		free(args); // free up memory for arguments
		exit(1);
	}

	if (child_pid == 0) { 
		// Executing inside child process
		if (execvp(args[0], args) < 0) { // catch error when executing command fail
			perror(args[0]);
			free(args); // free up memory for arguments
			exit(1);
		}
		// if ace success -> it never return and not run any code more
	} else { 
		// Executing inside parent process 
		if (!runInBackground) {	// wait for child process stop
			waitpid(child_pid, &stat_loc, WUNTRACED);
		}
	}

	free(args); // free up memory for arguments
}

void executeRedirectCommand(char* command) {
	if (strcmp(command, "") == 0) { // check for empty command
		return;
	}
	// store command to history
	strcpy(lastCommand, command);

	pid_t child_pid;
	int stat_loc;
	int newfd, ret;
	bool isOut = true;
	// extract arguments from command
	char** args = getArgs(command);
	
	char** a1 = (char**)malloc(MAX_ARGUMENTS * sizeof(char *));
	int index = 0;
	while (strcmp(args[index],">") != 0 && strcmp(args[index],"<") != 0) {
		a1[index] = args[index];
		index++;
	}
	if (strcmp(args[index],"<") == 0) isOut = false;
	a1[index] = NULL;
	
	bool runInBackground = extractRunInBackground(args);
	
	// create a child process and store its id in child_pid
	child_pid = fork();
	
	if (child_pid < 0) {
		perror("Fork failed");
		free(args); // free up memory for arguments
		free(a1);
		exit(1);
	}
	
	
	if (child_pid == 0) { 
		// Executing inside child process
		if (isOut) {
			newfd = open(args[index+1],  O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (newfd<0) {
				perror("open file output failed");	/* open failed */
				free(args); // free up memory for arguments
				free(a1);
				exit(1);
			}
			ret = dup2(newfd, STDOUT_FILENO);
		}
		else {
			newfd = open(args[index+1],  O_RDONLY, 0);
			if (newfd<0) {
				perror("open file input failed");	/* open failed */
				free(args); // free up memory for arguments
				free(a1);
				exit(1);
			}
			ret = dup2(newfd, STDIN_FILENO);
		}
		if (ret < 0) {
			perror("dup2 failed");
			free(args); // free up memory for arguments
			free(a1);
			exit(1);
		}
		close(newfd);
		if (execvp(a1[0], a1) < 0) { // catch error when executing command fail
			perror(a1[0]);
			free(args); // free up memory for arguments
			free(a1);
			exit(1);
		}
		// if execvp success -> it never return and not run any code more
	} else { 
		// Executing inside parent process 
		if (!runInBackground) {	// wait for child process stop
			waitpid(child_pid, &stat_loc, WUNTRACED);
		}
	}
	free(args); // free up memory for arguments
	free(a1);
}

void executePipeCommand(char* command) {
	if (strcmp(command, "") == 0) { // check for empty command
		return;
	}
	// store command to history
	strcpy(lastCommand, command);

	pid_t child_pid;
	int stat_loc;
	int fd[2];
	int WRITE_END = 1;
	int READ_END = 0;
	// extract arguments from command
	char** args = getArgs(command);
	
	// extract the first UNIX command before the pipe symbol (|)
	char** a1 = (char**)malloc(MAX_ARGUMENTS * sizeof(char *));
	int index = 0;
	while (strcmp(args[index],"|") != 0) {
		a1[index] = args[index];
		index++;
	}
	a1[index] = NULL;
	index++;
	
	// extract the second UNIX command after the pipe symbol (|)
	char** a2 = (char**)malloc(MAX_ARGUMENTS * sizeof(char *));
	int index2 = 0;
	while (args[index] != NULL) {
		a2[index2] = args[index];
		index2++;
		index++;
	}
	a2[index] = NULL;
	
	bool runInBackground = extractRunInBackground(args);
	
	// create a child process and store its id in child_pid
	child_pid = fork();
	
	if (child_pid < 0) {
		perror("Fork 1 failed");
		free(args); // free up memory for arguments
		free(a1);
		free(a2);
		exit(1);
	}
	if (child_pid == 0) { 
		
		pipe(fd);
		//fork another child process invoking fork() system call and perform the followings in this child process:
		child_pid = fork();
		if (child_pid < 0) {
			perror("Fork failed");
			free(args); // free up memory for arguments
			free(a1);
			free(a2);
			exit(1);
		}
		if (child_pid == 0) {
			//close the write end descriptor of the pipe invoking close() system call
			close(fd[WRITE_END]);
			//copy the read end  descriptor of the pipe to standard input file descriptor (STDIN_FILENO) invoking dup2() system call
			dup2(fd[READ_END], STDIN_FILENO);
			close(fd[READ_END]);
			//change the process image of the this child with the new image according to the second UNIX command after the pipe symbol (|) using execvp() system call
			if (execvp(a2[0], a2) < 0) { // catch error when executing command fail
				perror(a2[0]);
				free(args); // free up memory for arguments
				free(a1);
				free(a2);
				exit(1);
			}
		}
		else {
			//close the read end descriptor of the pipe invoking close() system call
			close(fd[READ_END]);
			//copy the write end descriptor of the pipe to standard output file descriptor (STDOUT_FILENO) invoking dup2() system call
			dup2(fd[WRITE_END], STDOUT_FILENO);
			close(fd[WRITE_END]);
			fflush(stdout);
			//change the process image with the new process image according to the first UNIX command before the pipe symbol (|) using execvp() system call
			if (execvp(a1[0], a1) < 0) { // catch error when executing command fail
				perror(a1[0]);
				free(args); // free up memory for arguments
				free(a1);
				free(a2);
				exit(1);
			}
		}
	}
	else { 
		// Executing inside parent process 
		if (!runInBackground) {	// wait for child process stop
			waitpid(child_pid, &stat_loc, WUNTRACED);
		}
	}
	free(args); // free up memory for arguments
	free(a1);
	free(a2);
}

int getCommandType(char * command) {
	char** args = getArgs(command);
	int type = 0, // default normal command
		index = 0;
	while (args[index] != NULL) {
		if (strcmp(args[index], "!!") == 0) {
			type = 1; break;
		}
		if (strcmp(args[index], ">") == 0 || strcmp(args[index], "<") == 0) {
			type = 2; break;
		}
		if (strcmp(args[index], "|") == 0) {
			type = 3; break;
		}
		index ++;
	}
	free(args);
	printf("type:%d\n", type);
	return type; 
}

void executeLastCommand() {
	if (strcmp(lastCommand, "") == 0) { // catch empty history
		printf("No commands in history.\n");
		return;
	}
	int type = getCommandType(lastCommand);
	if (type == 2) {
		executeRedirectCommand(lastCommand);	
		return;
	}
	if (type == 3) {
		executePipeCommand(lastCommand);
		return;
	}
	executeNormalCommand(lastCommand);
}

void getFirstWord(char* str, char* firstWord)
{
  int index = 0, i;
  while(str[index] == ' ' || str[index] == '\t' || str[index] == '\n')
    index++;
	
  i = 0;
  while(str[i + index] != '\0' && str[i + index] != ' ' && str[index] != '\t') {
    firstWord[i] = str[i + index];
    i++;
  }
	firstWord[i] = '\0';
}

int main(void) {
	char command[MAX_LENGTH], firstWord[MAX_LENGTH];
	int should_run = 1;

	while (should_run) {
		printf("ssh>>");
		fflush(stdout);
		fgets(command, MAX_LENGTH, stdin); // input command with MAX_LENGTH=80

		// remove ending '\n' at the end of command
		char *pos;
		if ((pos=strchr(command, '\n')) != NULL) {
    		*pos = '\0';
		}
		
		// check exit
		getFirstWord(command, firstWord);
		if (strcmp(firstWord, "exit") == 0) {
			should_run = 0;
			break;
		}

		//Parse command and arguments.
		int commandType = getCommandType(command);

		switch (commandType)
		{
		case 0:
			executeNormalCommand(command);
			break;
		case 1:
			executeLastCommand();
			break;
		case 2:
			executeRedirectCommand(command);
			break;
		case 3:
			executePipeCommand(command);
			break;
		default:
			break;
		}
		
		//If command contains output redirection argument
		//	fork a child process invoking fork() system call and perform the followings in the child process:
		//		open the redirected file in write only mode invoking open() system call
		//		copy the opened file descriptor to standard output file descriptor (STDOUT_FILENO) invoking dup2() system call
		//		close the opened file descriptor invoking close() system call
		//		change the process image with the new process image according to the UNIX command using execvp() system call
		//	If command does not conatain & (ampersand) at the end
		//		invoke wait() system call in parent process.
		//
		//		
		//If command contains input redirection argument
		//	fork a child process invoking fork() system call and perform the followings in the child process:
		//		open the redirected file in read  only mode invoking open() system call
		//		copy the opened file descriptor to standard input file descriptor (STDIN_FILENO) invoking dup2() system call
		//		close the opened file descriptor invoking close() system call
		//		change the process image with the new process image according to the UNIX command using execvp() system call
		//	If command does not conatain & (ampersand) at the end
		//		invoke wait() system call in parent process.
		//
		//	
		
		//If command contains pipe argument
		//	fork a child process invoking fork() system call and perform the followings in the child process:
		//		create a pipe invoking pipe() system call
		//		fork another child process invoking fork() system call and perform the followings in this child process:
		//			close the write end descriptor of the pipe invoking close() system call
		//			copy the read end  descriptor of the pipe to standard input file descriptor (STDIN_FILENO) invoking dup2() system call
		//			change the process image of the this child with the new image according to the second UNIX command after the pipe symbol (|) using execvp() system call
		//		close the read end descriptor of the pipe invoking close() system call
		//		copy the write end descriptor of the pipe to standard output file descriptor (STDOUT_FILENO) invoking dup2() system call
		//		change the process image with the new process image according to the first UNIX command before the pipe symbol (|) using execvp() system call
		//	If command does not conatain & (ampersand) at the end
		//		invoke wait() system call in parent process.
		//
		//
		//If command does not contain any of the above
		//	fork a child process using fork() system call and perform the followings in the child process.
		//		change the process image with the new process image according to the UNIX command using execvp() system call
		//	If command does not conatain & (ampersand) at the end
		//		invoke wait() system call in parent process.
	}

	return 0;
}
