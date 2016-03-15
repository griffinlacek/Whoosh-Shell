//Griffin Lacek CS537 2016

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>

void check_redirect(char **args, int size, int *r_count, int *last_r) {
	int i;
	
	for(i=1; i<size; i++) {
		if(strcmp(args[i], ">") == 0) {
			*r_count = *r_count + 1;
			*last_r = i;
		}
	}
}

int get_redirect(char *line, int size, char **command, int *fd) {
	char *token;
	token = strtok(line, " ");
	
	while(token != NULL) {
		if(strcmp(token, ">") == 0) {			
			token = strtok(NULL, " ");
			*fd = open(token, O_WRONLY | O_CREAT | O_TRUNC | S_IWUSR);
			break;
		}
		
		command[size] = token;
		command[size + 1] = NULL;
		size++;
		
		token = strtok(NULL, " ");
	}
	return *fd;
}

void error_msg() {
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}

int path(char **command, int num_args, char **path_list, int *path_num) {
	if(strcmp(command[0], "path") == 0) {		
		if(num_args > 1) {
			int i;
			for(i=1; i<num_args; i++) {
				if(command[i][0] == '/') {
					path_list[i - 1] = strdup(command[i]);
					*path_num = *path_num + 1;
				}
				else {
					error_msg();
				}
			}
		}
		else {
			//Remove the current path
			path_list[0] = NULL;
			path_num = 0;
		}
		return 0;
	}
	return 1;
}

int pwd(char **command) {
	
	if(strcmp(command[0], "pwd") == 0) {
		
		char *cwd;
		char buff[PATH_MAX + 1];
	
		cwd = getcwd(buff, PATH_MAX + 1);
		if(cwd != NULL) {
			printf("%s\n", cwd);
		}
		else {
			error_msg();
		}
		return 0;
	}
	
	return 1;
}

int cd(char **command, int num_args) {
	
	if(strcmp(command[0], "cd") == 0) {
		
		char *dir;
		
		if(num_args == 1) {
			dir = getenv("HOME");
			
			if(chdir(dir) != 0) {
				error_msg();
			}			
		}
		else {
			dir = command[1];
			
			if(chdir(dir) != 0) {
				error_msg();
			}
		}
		return 0;
	}
	
	return 1;
}

void check_exit(char **command) {
	if(strcmp(command[0], "exit") == 0) {
		exit(0);
	}
}

int check_built_in(char **command, int num_args, char **path_list, int *path_num) {
	
	if(cd(command, num_args) == 0) {
		return 0;
	}
	else if(pwd(command) == 0) {
		return 0;
	}
	else if(path(command, num_args, path_list, path_num) == 0) {
		return 0;
	}
	else {
		return 1;
	}
}

int count_args(char *command) {
	int num_args = 0;
	int length = strlen(command);
	char *str = command;
	int state = 0;
	int i;
	
	for(i=0; i<length; i++) {
		if (str[i] == ' ' || str[i] == '\n' || str[i] == '\t') {
			state = 0;
		}
		else if (state == 0) {
			state = 1;
			num_args++;
		}
	}
	
	return num_args;
}

void get_line(char *input) {
	input = strtok(input, "\n");	
}

void get_args(char *line, char **args, int size) {
	char *token;
	int i = 0;
		
	token = strtok(line, " ");

	while (token != NULL) {
		
		if (i < size) {
			if((i + 1) == size) {
				args[i] = token;
			}
			else {
				args[i] = strdup(token);
			}
			i++;
		}
		
		token = strtok(NULL, " ");	
		
	}
}

void exec_cmd(char **command, char *r_output, char *r_path, int r_count) {
	// process creation
	int rc = fork();

	//child process
	if (rc == 0) {	
		//Execute process
		if(r_count == 1) {
			if(r_path != NULL) {
				chdir(r_path);
			}
			char *r_error = strdup(r_output);
			strcat(r_output, ".out");
			strcat(r_error, ".err");		
			
			close(STDOUT_FILENO);
			open(r_output, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
			close(STDERR_FILENO);
			open(r_error, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
		}
	
		if (execv(command[0], command) == -1) {
			error_msg();
			_exit(1);
		}
	}
	//parent process
	else if (rc > 0) {
		int cpid = (int) wait(NULL);
		if (cpid == -1) {
			error_msg();
		}
	}
	else {
		error_msg();
	}
}

int main(int argc, char *argv[]) {
	
	//Checks that only ./whoosh is entered to invoke shell
	if(argc != 1) {
		error_msg();
		exit(1);
	}
	
	int CHAR_LIMIT = 129;
	int BUFFER_LIMIT = 1024;
	char buffer[BUFFER_LIMIT];
	
	int path_num = 1;
	char* s_path[BUFFER_LIMIT];
	s_path[0] = "/bin";
	
	while(1) {
		printf("whoosh> ");
		fflush(stdout);		
		fgets(buffer, sizeof(buffer), stdin);
		
		get_line(buffer);
		
		int line_size = strlen(buffer);
		
		if(line_size > CHAR_LIMIT) {
			error_msg();
			continue;
		}
		
		//Get num of args in command 
		int num_args = count_args(buffer);
		
		if(num_args != 0) {
			
			char* myargv[num_args + 1];
			int r_count = 0;
			int last_r = 0;
			char* r_path = NULL;
			int i;
			char buf[PATH_MAX + 1];
			char *cwd;
			char *r_output;
			
			//Put each argument in myargv array
			get_args(buffer, myargv, num_args);
			
			//Check for redirect
			check_redirect(myargv, num_args, &r_count, &last_r);
		
			//Redirect error handling
			if(r_count > 1) {
				error_msg();
				continue;
			}
			else if (r_count == 1) {
				if(last_r != num_args - 2) {
					error_msg();
					continue;
				}
				if(myargv[num_args - 1][0] == '/') {
					if(chdir(myargv[num_args - 1]) == 0) {
						r_path = strdup(myargv[num_args - 1]);
					}
					else {
						error_msg();
						continue;
					}
				}
				
				r_output = malloc(strlen(myargv[num_args - 1]) + strlen(".out"));
				sprintf(r_output, "%s", myargv[num_args - 1]);	
				//Take off > and path from arguments
				num_args = num_args - 2;
			}
			
			//Set last element in myargv array to NULL
			myargv[num_args] = NULL;
	
			//Check if exit was entered
			check_exit(myargv);
			
			//Check if any built-in commands were called
			if(check_built_in(myargv, num_args, s_path, &path_num) == 0) {
				continue;
			}
			else{
				int file_exist = 0;
				
				cwd = getcwd(buf, PATH_MAX + 1);
				for(i=0; i<path_num; i++) {
					chdir(s_path[i]);
					struct stat path_buff;
					if(stat(myargv[0], &path_buff) == 0) {
						file_exist = 1;
						char *exec_path = malloc(strlen(s_path[i]) + strlen("/") + strlen(myargv[0]) + 1);
						sprintf(exec_path, "%s/%s", s_path[i], myargv[i]);
						myargv[0] = exec_path;
						chdir(cwd);
						break;
					}
				}	
				if(file_exist == 0) {
					error_msg();
					continue;
				}
				else {
					exec_cmd(myargv, r_output, r_path, r_count);
				}
			}
		}	
	}
}