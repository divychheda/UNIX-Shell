// BT19CSE022 Divy Chheda

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()
#include <ctype.h>

#define MAX_PATH 256
#define MAX_LIST 100


int separateAnd(char *buf, char **cmds) // divide the line with '&&' into its separate commands
{ 
    int i = 0;
	while( (cmds[i] = strsep(&buf, "&&")) != NULL && i < MAX_LIST) 
	{
        if( *cmds[i] == '\0' ) {
            continue;
        }
		i++;
    } 
  
    if (cmds[1] == NULL) // No '&&' found in line
        return 0; 
    else 
	{ 
        return 1; 
    } 
}

int separateHash(char *buf, char **cmds) // divide the line with '##' into its separate commands
{
	int i = 0;
	while( (cmds[i] = strsep(&buf, "##")) != NULL && i < MAX_LIST) 
	{
        if( *cmds[i] == '\0' ) {
            continue;
        }
		i++;
    } 
    if (cmds[1] == NULL) // No '##' found in line
        return 0; 
    else 
	{ 
        return 1; 
    } 
}

int separateRedirect(char *buf, char **cmds) // divide the line with '>' into its separate commands
{
	int i = 0;
	while( (cmds[i] = strsep(&buf, ">")) != NULL && i < MAX_LIST) 
	{
        if( *cmds[i] == '\0' ) {
            continue;
        }
		i++;
    }
    if (cmds[1] == NULL) // No '>' found in line
        return 0; 
    else 
	{ 
        return 1; 
    } 
}

int parseInput(char *buf, char **cmds) // returns value according to different separate commands
{
	if(separateAnd(buf,cmds))
	{
		return 1;
	}
	else if(separateHash(buf, cmds))
	{
		return 2;
	}
	else if(separateRedirect(buf, cmds))
	{
		return 3;
	}
	else
	{
		return 0;
	}
	
}

char *trimspace(char *str)
{
	while(isspace((unsigned char)*str)) 
	{
		str++;
	}

	if(*str == 0)
	{
		return str;
	}

	char *end = str + strlen(str) - 1;

	while(end > str && isspace((unsigned char)*end)) 
	{
		end--;
	}
	end[1] = '\0';

	return str;
}

void tokenize(char *cmd, char **tokens) // separate one commands into its individual keywords
{
	int i = 0;
	while( (tokens[i] = strsep(&cmd, " ")) != NULL && i < MAX_LIST) 
	{
        if( *tokens[i] == '\0' ) {
            continue;
        }
		i++;
    }
}

void change_dir(char *cmds)
{
	char *token = cmds+3;
	if(token[0] == '\'')
		token = token+1;
	if(token[strlen(token)-1] == '\'')
		token[strlen(token)-1] = '\0';
		
	int ret_val = chdir(token);
	if(ret_val == -1)
		printf("Shell: Incorrect command\n");
}

void executeCommand(char *cmd)
{
	// This function will fork a new process to execute a command
	cmd = trimspace(cmd);
	if(strncmp(cmd, "cd", 2) == 0)
		change_dir(cmd);
	else
	{
		int rc = fork();

		char *tokens[MAX_LIST];
		if(rc < 0) exit(0);
		else if(rc > 0) wait(NULL);
		else 
		{
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);

			tokenize(cmd, tokens);
			int status = execvp(tokens[0], tokens);
			if(status < 0) printf("Shell: Incorrect command\n");
		
			exit(0);
		}
	}
}

void executeParallelCommands(char **cmds)
{
	// This function will run multiple commands in parallel
	int i = 0;

	while(cmds[i] != NULL)
	{
		char *tokens[MAX_LIST];

		int rc=fork();

		if(rc < 0) exit(0);					
		else if (rc == 0)
		{
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);

			tokenize(cmds[i],tokens);
			int status = execvp(tokens[0], tokens);
			if(status < 0) printf("Shell: Incorrect command\n");
			exit(0);
		}
		i++;
	}

	for(int j = 0; j<i; j++)
	{
		wait(NULL);
	}
}

void executeSequentialCommands(char **cmds)
{	
	// This function will run multiple commands in parallel

	int i = 0;
	while(cmds[i] != NULL)
	{
		executeCommand(cmds[i]);
		i++;
	}
}



void executeCommandRedirection(char **cmds)
{
	// This function will run a single command with output redirected to an output file specificed by user
	int len = 0;
	int i = 0;
	while(cmds[i] != NULL)
	{
		i++;
	}
	len = i;

	int rc = fork();

	char *tokens[MAX_LIST];
	if(rc < 0) exit(0);
	else if(rc > 0) wait(NULL);
	else 
	{
		signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		cmds[len-1] = trimspace(cmds[len-1]);
		close(STDOUT_FILENO);
		int fd = open(cmds[len-1], O_RDWR | O_CREAT | O_APPEND, 0666);
		
		tokenize(cmds[0], tokens);
		int status = execvp(tokens[0], tokens);
		if(status < 0) printf("Shell: Incorrect command\n");

		if(close(fd)<0) printf("error");
		exit(0);
	}
}

void red() {
  printf("\033[1;31m");
}

void blue()
{
	printf("\033[0;34m");
}

void reset() {
  printf("\033[0m");
}

int main()
{

	// Initial declarations
	char currentWorkingDirectory[MAX_PATH];
	
	char *buff;
	size_t buff_size = 128;
	int ch;
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	buff = (char *)malloc(buff_size * sizeof(char));
	while(1)	// This loop will keep your shell running until user exits.
	{
		getcwd(currentWorkingDirectory, MAX_PATH);
		red();
		printf("%s",currentWorkingDirectory);
		blue();
		printf("$ ");
		reset();

		ch = getline(&buff, &buff_size, stdin); // accept input with 'getline()'
		if(strlen(buff) == 1)
		{
			continue;
		}	
		buff[strlen(buff)-1] = '\0';
		char *cmds[MAX_LIST];

		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		int retVal = parseInput(buff, cmds); 		
		
		if(retVal == 0 && strcmp(trimspace(cmds[0]),"exit")==0)	// When user uses exit command.
		{
			printf("Exiting shell...\n");
			exit(0);
		}

		if(retVal == 1)
			executeParallelCommands(cmds);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		else if(retVal == 2)
			executeSequentialCommands(cmds);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		else if(retVal == 3)
			executeCommandRedirection(cmds);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		else
			executeCommand(cmds[0]);			// This function is invoked when user wants to run a single commands
				
	}
	free(buff);
	return 0;
}
