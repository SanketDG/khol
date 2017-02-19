#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

#include <readline/readline.h>
#include <readline/history.h>

#define MAX_BUFSIZE 1024
#define TOKEN_BUFSIZE 64
#define TOKEN_DELIMS " \t\r\n\a"

int cd(char **args);
int help(char **args);
int khol_exit(char **args);

char *builtins[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char**) = {
    &cd,
    &help,
    &khol_exit
};

int num_builtins() {
    return sizeof(builtins) / sizeof(char *);
}

int cd(char **args) {
    if(args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to `cd`\n");
    } else {
        if(chdir(args[1]) != 0) {
            perror("khol:");
        }
    }

    return 1;
}

int help(char **args) {
    int i;
    printf("KHOL: A minimalistic shell\n");

    return 1;
}

int khol_exit(char **args)
{
    return 0;
}


int launch(char **args) {

    pid_t pid, wpid;

    int status;

    if( (pid = fork()) == 0 ) {
        // child process

        if( execvp(args[0], args) == -1 ) {
            perror("khol");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking, do something!
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        }   while ( !WIFEXITED(status) && !WIFSIGNALED(status) );
    }

    return 1;
}

int execute(char **args) {
    int i;

    if(args[0] == NULL) {
        return 1;
    }

    for(i = 0; i < num_builtins(); i++) {
        if (!strcmp(args[0], builtins[i])) {
            return (*builtin_func[i])(args);
        }
    }

    return launch(args);
}

char **split_line(char *line) {
    char **tokens = malloc(sizeof(char*) * TOKEN_BUFSIZE);
    char *token;

    int pos = 0;

    if(!tokens) {
        fprintf(stderr, "KHOL: Memory allocation failed.");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOKEN_DELIMS);
    while(token != NULL) {
        tokens[pos] = token;
        pos++;

        if(pos >= TOKEN_BUFSIZE) {
            tokens = realloc(tokens, sizeof(char*) * TOKEN_BUFSIZE * 2);

            if(!token) {
                fprintf(stderr, "KHOL: Memory allocation failed.");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOKEN_DELIMS);
    }

    tokens[pos] = NULL;
    return tokens;
}

void main_loop(void) {
    char *line;
    char **args;

    int status;

    // basic prompt
    char *prompt = "> ";

    do {
        // use GNU's readline() function
        line = readline(prompt);


        // add to history for future use
        add_history(line);

        // EOF
        if(!line) {
            status = 0;
        }
        else {
            args = split_line(line);
            status = execute(args);

            free(args);
        }

        free(line);
    } while ( status );
}

int main(int argc, char* argv[]) {
    main_loop();

    return EXIT_SUCCESS;
}
