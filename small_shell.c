#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>


#define INPUT_LENGTH 2048
#define MAX_ARGS	 512

// These variables are used for status management
int exit_status;
int terminated_signal;
bool background_status = false;

struct command_line
{
	char *argv[MAX_ARGS + 1];
	int argc;
	char *input_file;
	char *output_file;
	bool is_bg;
};

/*
    Function: sigchld_handler
    This function shows messages when child process is terminated normally or by signal
*/
void sigchld_handler(int signo) {
    int status;
    pid_t pid;
    
    while ((pid = wait(&status)) > 0) {
		if (WIFEXITED(status)) {
			// child prcess is terminated normally
			write(STDOUT_FILENO, "background pid ", 15);
			printf("%d", pid);
			fflush(stdout);
			write(STDOUT_FILENO, " is done: exit value ", 21);
			printf(" %d\n", WEXITSTATUS(status));
			fflush(stdout);
		} else if (WIFSIGNALED(status)) {
			// child process is terminated by signal
			write(STDOUT_FILENO, "background pid ", 15);
			printf("%d", pid);
			fflush(stdout);
			write(STDOUT_FILENO, " is done: terminated by signal ", 30);
			printf(" %d\n", WTERMSIG(status));
			fflush(stdout);
		}
    }
}

/*
	Function: signal_handler
	This function shows message when inputting SIGINT or SIGTSTP
*/
void signal_handler(int sig) {
	// Store the terminated signal
	terminated_signal = sig;

	if (sig == SIGINT) {
		write(STDOUT_FILENO, "terminated by signal ", 21);
		printf("%d\n", sig);
		fflush(stdout);
	}
	
	if (sig == SIGTSTP) {
		if (!background_status) {
			// First time
			write(STDOUT_FILENO, "Entering foreground-only mode (& is now ignored)", 48);
		} else {
			// Not first time
			write(STDOUT_FILENO, "Entering foreground-only mode", 29);
		}
		printf("\n");
		fflush(stdout);

		// Update the variable to true for not to run process in the background
		background_status = true;
	}
}

/*
	Function: status_command
	This function works as status command
*/
void status_command(struct command_line *command) {
	if (terminated_signal > 0) {
		// Show message with signal
		write(STDOUT_FILENO, "terminated by signal ", 21);
		printf("%d\n", terminated_signal);
		fflush(stdout);
		terminated_signal = 0;
	} else {
		// Show message with exit value
		write(STDOUT_FILENO, "exit value ", 11);
		printf("%d", exit_status);
		fflush(stdout);
		write(STDOUT_FILENO, "\n", 2);
	}
}

/*
	Function: cd_command
	This function works as cd command
*/
void cd_command(struct command_line *command) {
	if (command->argv[1] == NULL) {
		// Change to the home directory
		chdir(getenv("HOME"));
	} else {
		// Get current path
		char *current_directory;
		char buffer[INPUT_LENGTH];		
		current_directory = getcwd(buffer, sizeof(buffer));

		// Make change directory path
		char *change_directory;
		change_directory = strcat(current_directory, "/");
		change_directory = strcat(change_directory, command->argv[1]);

		// Change directory
		chdir(change_directory);
	}
}

/*
	Function: redirect_stdout
	This function works as stdout redirection command
*/
void redirect_stdout(struct command_line *command) {
	// Open output file
	int output_fd;
	if (command->is_bg) {
		output_fd = open("dev/null", O_WRONLY);
	} else {
		output_fd = open(command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	}
	
	// Open the output file for writing
	if (output_fd == -1) {
		perror("output_fd");
		exit(EXIT_FAILURE);
	}

	// Redirect stdout to the output file
	if (dup2(output_fd, STDOUT_FILENO) == -1) {
		perror("dup2 output_fd");
		close(output_fd);
		exit(EXIT_FAILURE);
	}
	close(output_fd);
}

/*
	Function: redirect_stdin
	This function works as stdin redirection command
*/
void redirect_stdin(struct command_line *command) {
	// Open input file
	int input_fd;
	if (command->is_bg) {
		input_fd = open("dev/null", O_RDONLY);
	} else {
		input_fd = open(command->input_file, O_RDONLY);
	}

	// Open the input file for reading
	if (input_fd == -1) {
		write(STDOUT_FILENO, "cannot open ", 12);
		write(STDOUT_FILENO, command->input_file, strlen(command->input_file));
		write(STDOUT_FILENO, " for input", 10);
		printf("\n");
		fflush(stdout);
		// perror("input_fd");
		exit(EXIT_FAILURE);
	}

	// Redirect stdin to the input file
	if (dup2(input_fd, STDIN_FILENO) == -1) {
		perror("dup2 input_fd");
		close(input_fd);
		exit(EXIT_FAILURE);
	}
	close(input_fd);
}

/*
	Function: non_builtin_command
	This function works as linux commands except for cd, status, and exit commands
*/
void non_builtin_command(struct command_line *command) {	
	// Make child process
	pid_t pid = fork();

	if (pid == -1) {
		perror("fork");
		exit(1);
	} else if (pid == 0) {
		// Child process

		// Redirect stdin and stdout
		if ((command->input_file != NULL) && (command->output_file != NULL)) {
			redirect_stdin(command);
			redirect_stdout(command);
	
			// Execute the command
			execlp(command->argv[0], command->argv[0], NULL);
			perror(command->argv[0]);
			exit(EXIT_FAILURE);
		}
		
		// Redirect stdin
		if (command->input_file != NULL) {
			redirect_stdin(command);
	
			// Execute the command
			execlp(command->argv[0], command->argv[0], NULL);
			perror(command->argv[0]);
			exit(EXIT_FAILURE);
		}

		// Redirect stdout
		if (command->output_file != NULL) {
			redirect_stdout(command);
			
			// Execute command
			execlp(command->argv[0], command->argv[0], NULL);
			perror(command->argv[0]);
			exit(EXIT_FAILURE);
		}

		// test command
		if (!strcmp(command->argv[0], "test")) {
			execlp(command->argv[0], command->argv[0], command->argv[1], command->argv[2], NULL);
			perror(command->argv[0]);
			exit(EXIT_FAILURE);
		}

		// kill command
		if (!strcmp(command->argv[0], "kill")) {
			execlp(command->argv[0], command->argv[0], command->argv[1], command->argv[2], NULL);
			perror(command->argv[0]);
			exit(EXIT_FAILURE);
		}

		// Other non builtin commands
		if (command->argv[1] == NULL) {
			// No argument
			execlp(command->argv[0], command->argv[0], NULL);
			perror(command->argv[0]);
			exit(EXIT_FAILURE);
			// One argument
		} else if (command->argv[1] != NULL) {
			execlp(command->argv[0], command->argv[0], command->argv[1], NULL);
			perror(command->argv[0]);
			exit(EXIT_FAILURE);
		}
	} else {
		// Parent process

		// Set sigchild handler to manage child processes
		struct sigaction sa;
		sa.sa_handler = sigchld_handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

		int status;
		// Check SIGTSTP status
		if (!background_status) {
			// Not executed SIGTSTP command
			if (command->is_bg) {
				// Background process

				// Show background pid
				write(STDOUT_FILENO, "background pid is ", 18);
				printf("%d\n", pid);
				fflush(stdout);
			} else {
				// Foreground process
			
				// Signal handler
				if (signal(SIGINT, signal_handler) == SIG_ERR) {
					perror("SIGINT");
					exit(EXIT_FAILURE);
				} else if (signal(SIGTSTP, signal_handler) == SIG_ERR) {
					perror("SIGTSTP");
					exit(EXIT_FAILURE);
				}

				// Background child processes are teminated
				if (sigaction(SIGCHLD, &sa, NULL) == -1) {
					perror("sigchild");
					exit(EXIT_FAILURE);
				}

				// Wait child process
				waitpid(pid, &status, 0);
			}
		} else {
			// Executed SIGTSTP command (& is ignored)
			
			// Signal handler
			if (signal(SIGINT, signal_handler) == SIG_ERR) {
				perror("SIGINT");
				exit(EXIT_FAILURE);
			} else if (signal(SIGTSTP, signal_handler) == SIG_ERR) {
				perror("SIGTSTP");
				exit(EXIT_FAILURE);
			}

			// Background child processes are teminated
			if (sigaction(SIGCHLD, &sa, NULL) == -1) {
				perror("sigchild");
				exit(EXIT_FAILURE);
			}

			// Wait child process
			waitpid(pid, &status, 0);
		}

		// Store the terminated signal
		if (WIFSIGNALED(status)) {
			terminated_signal = WTERMSIG(status);
		}

		if (WIFEXITED(status)) {
			// Successfully finished child process
			exit_status = WEXITSTATUS(status);

		} else {
			// Failed child process
			exit_status = 1;
		}
	}
}

struct command_line *parse_input()
{	
	char input[INPUT_LENGTH];
	struct command_line *curr_command = (struct command_line *) calloc(1,sizeof(struct command_line));

	// Get input
	printf(": ");
	fflush(stdout);
	fgets(input, INPUT_LENGTH, stdin);

	// Tokenize the input
	char *token = strtok(input, " \n");
	while(token){
		if (!strcmp(token,"<")){
			curr_command->input_file = strdup(strtok(NULL," \n"));
		} else if(!strcmp(token,">")){
			curr_command->output_file = strdup(strtok(NULL," \n"));
		} else if(!strcmp(token,"&")){
			curr_command->is_bg = true;
		} else{
			curr_command->argv[curr_command->argc++] = strdup(token);
		}
		token=strtok(NULL," \n");
	}
	return curr_command;
}

int main() {
	struct command_line *curr_command;
	while(true)
	{
		curr_command = parse_input();
		if (curr_command->argv[0] != NULL) {
			if (!strncmp(curr_command->argv[0], "#", 1)) {
				// Skip for comment input
				continue;
			} else if (!strcmp(curr_command->argv[0], "cd")) {
				// cd command
				cd_command(curr_command);
			} else if (!strcmp(curr_command->argv[0], "exit")) {
				// exit command
				return EXIT_SUCCESS;
			} else if (!strcmp(curr_command->argv[0], "status")) {
				// status command
				status_command(curr_command);
			} else {
				// non-builtin commands
				non_builtin_command(curr_command);
			} 
		} else if (curr_command->argv[0] == NULL) {
			// Skip for line feed input
			continue;
		}
	}
	return EXIT_SUCCESS;
}