#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "constants.h"

char *history_path = NULL;

int cd(char **args);
int help(char **args);
int history(char **args);
int khol_exit(char **args);

int (*builtin_func[]) (char**) = {
    &cd,
    &help,
    &history,
    &khol_exit
};

int num_builtins() {
    return sizeof(builtins) / sizeof(char *);
}

int cd(char **args) {
    if(args[1] == NULL) {
        fprintf(stderr, YELLOW "khol: expected argument to `cd`\n" RESET);
    } else {
        if(chdir(args[1]) != 0) {
            perror(RED "khol: " RESET);
        }
    }

    return 1;
}

int help(char **args) {
    int i;
    printf(BOLD"khol: A minimalistic shell written in C."
               "\nCopyright (C) 2017 Sanket Dasgupta\n"
           RESET);

    return 1;
}

int khol_exit(char **args)
{
    return 0;
}


int launch(char **args, int fd) {

    pid_t pid, wpid;

    int status;

    if( (pid = fork()) == 0 ) {
        // child process

        if(fd > -1) {

            if(dup2(fd, 1) == -1 ) {
                perror(RED "Error duplicating stream: " RESET);
                return 1;
            }

            close(fd);
        }


        if( execvp(args[0], args) == -1 ) {
            perror(RED "khol: " RESET);
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

int history(char **args) {
    char *history_args[4] = {"cat", "-n", history_path, NULL};
    return launch(history_args, -1);
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

    int j = 0;

    while(args[j] != NULL) {
        // for `>` operator for redirection
        if(!( strcmp(">", args[j]) )) {
            int fd = fileno(fopen(args[j+1], "w+"));
            args[j] = NULL;
            return launch(args, fd);
        }
        // for `>>` operator for redirection
        else if(!( strcmp(">>", args[j]) )) {
            int fd = fileno(fopen(args[j+1], "a+"));
            args[j] = NULL;
            return launch(args, fd);
        }
        j++;
    }

    return launch(args, -1);
}

char **split_line(char *line) {
    char **tokens = malloc(sizeof(char*) * TOKEN_BUFSIZE);
    char *token;

    int pos = 0;

    if(!tokens) {
        fprintf(stderr, RED "khol: Memory allocation failed." RESET);
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOKEN_DELIMS);
    while(token != NULL) {
        tokens[pos] = token;
        pos++;

        if(pos >= TOKEN_BUFSIZE) {
            tokens = realloc(tokens, sizeof(char*) * TOKEN_BUFSIZE * 2);

            if(!token) {
                fprintf(stderr, RED "khol: Memory allocation failed." RESET);
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOKEN_DELIMS);
    }

    tokens[pos] = NULL;
    return tokens;
}

char *get_prompt(void) {
    char *prompt, tempbuf[PATH_MAX];

    prompt = malloc(sizeof(char) * PROMPT_MAXSIZE);

    if( getcwd( tempbuf, sizeof(tempbuf) ) != NULL ) {

        sprintf(prompt, "%s > ", tempbuf);
        return prompt;
    }
}

void main_loop(void) {
    char *line;
    char **args;

    int status;

    // get prompt
    char *prompt;

    char *homedir = getenv("HOME");

    history_path = malloc(sizeof(char) * HISTFILE_SIZE);

    sprintf(history_path, "%s/%s", homedir, ".khol_history");

    // read from history file
    read_history(history_path);

    do {
        // use GNU's readline() function
        prompt = get_prompt();
        line = readline(prompt);


        // add to history for future use
        add_history(line);

        // EOF
        if(!line) {
            status = 0;
        }
        else {
            // write to history file
            write_history(history_path);

            args = split_line(line);
            status = execute(args);

            free(args);
        }

        free(line);
    } while ( status );

    free(history_path);
}

int main(int argc, char* argv[]) {
    main_loop();

    return EXIT_SUCCESS;
}
