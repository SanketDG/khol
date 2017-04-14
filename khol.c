/*
 * khol (খোল) - A minimalistic shell written in C
 *
 * Copyright (C) 2017 Sanket Dasgupta
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <errno.h>
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

int khol_cd(char **args);
int khol_help(char **args);
int khol_history(char **args);
int khol_exit(char **args);


/* Function pointers that correspond to the buitins */
int (*builtin_func[]) (char**) = {
    &khol_cd,
    &khol_help,
    &khol_history,
    &khol_exit
};


/* Returns the number of builtin commands available for khol */
int num_builtins() {
    return sizeof(builtins) / sizeof(char *);
}

/*
 * Function that implements the khol builtin "cd", to change directories
 *
 * The command "cd" needs an argument to change the directory.
 *
 * If a argument is not provided, khol raises an warning.
 *
 */
int khol_cd(char **args) {
    if(args[1] == NULL) {
        fprintf(stderr, YELLOW "khol: expected argument to `cd`\n" RESET);
    } else {
        if(chdir(args[1]) != 0) {
            fprintf(stderr, RED "khol: %s\n" RESET, strerror(errno));
        }
    }

    return 1;
}

/* Function that shows the help message */
int khol_help(char **args) {
    int i;
    printf(BOLD"\nkhol: A minimalistic shell written in C."
               "\nCopyright (C) 2017 Sanket Dasgupta\n\n"RESET
               "Builtin commands:\n"
               "cd - Change dicrectory\n"
               "history - Show history from ~/.khol_history\n"
               "exit - Exit this shell"
               "help - Show this help\n\n"
           );

    return 1;
}

int khol_exit(char **args)
{
    return 0;
}

/*
 * Function that is responsible for launching and handling processes.
 *
 * @args:    The argument list provided to the shell. This argument list will
 *           be used to launch and execute the child process.
 * @fd:      The file descriptor to be passed, in case of a redirection operator
 *           being passed to the shell.
 * @options: A list of options being implemented to handle background processing
 *           and redirection. The options are defined in constants.h
 */
int launch(char **args, int fd, int options) {

    int khol_bg = (options & KHOL_BG) ? 1 : 0;
    int khol_stdout = (options & KHOL_STDOUT) ? 1 : 0;
    int khol_stderr = (options & KHOL_STDERR) ? 1 : 0;
    int khol_stdin = (options & KHOL_STDIN) ? 1 : 0;

    pid_t pid, wpid;

    int status;

    if( (pid = fork()) == 0 ) {
        // child process

        if(fd > 2) {

            if(khol_stdout && dup2(fd, STDOUT_FILENO) == -1 ) {
                fprintf(stderr, RED "khol: Error duplicating stream: %s\n" RESET, strerror(errno));
                return 1;
            }

            if(khol_stderr && dup2(fd, STDERR_FILENO) == -1 ) {
                fprintf(stderr, RED "khol: Error duplicating stream: %s\n" RESET, strerror(errno));
                return 1;
            }

            if(khol_stdin && dup2(fd, STDIN_FILENO) == -1 ) {
                fprintf(stderr, RED "khol: Error duplicating stream: %s\n" RESET, strerror(errno));
                return 1;
            }

            close(fd);
        }


        if( execvp(args[0], args) == -1 ) {
            fprintf(stderr, RED "khol: %s\n" RESET, strerror(errno));
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        fprintf(stderr, RED "khol: %s\n" RESET, strerror(errno));
    } else {
        do {
            if( !khol_bg ) {
                wpid = waitpid(pid, &status, WUNTRACED);
            }
            else {
                printf(YELLOW "[bg][%d] - %s\n" RESET, pid, args[0]);
            }
        } while ( !WIFEXITED(status) && !WIFSIGNALED(status) );
    }

    return 1;
}

/* Function that shows the khol history file along with line numbers
 *
 * Uses the 'cat' tool with '-n' argument to display line numbers
 */
int khol_history(char **args) {
    char *history_args[4] = {"cat", "-n", history_path, NULL};
    return launch(history_args, STDOUT_FILENO, KHOL_FG);
}

int pipe_launch(char** arg1, char** arg2) {
    int fd[2], pid;

    pipe(fd);

    int stdin_copy = dup(STDIN_FILENO);

    if( (pid = fork()) == 0 ) {
        close(STDOUT_FILENO);
        dup(fd[1]);
        close(fd[0]);
        launch(arg1, STDOUT_FILENO, KHOL_FG);
        exit(EXIT_FAILURE);
    }
    else if (pid > 0){
        close(STDIN_FILENO);
        dup(fd[0]);
        close(fd[1]);
        launch(arg2, STDOUT_FILENO, KHOL_FG);
        dup2(stdin_copy, STDIN_FILENO);
        return 1;
    }
}

/* Function responsible for parsing the arguments */
int execute(char **args) {
    int i;

    if(args[0] == NULL) {
        return 1;
    }

    i = 0;
    while(args[i]) {
        i++;
    }

    /* launch process in background */
    if( !strcmp("&", args[--i]) ) {
        args[i] = NULL;
        return launch(args, STDOUT_FILENO, KHOL_BG);
    }

    for(i = 0; i < num_builtins(); i++) {
        if ( !strcmp(args[0], builtins[i]) ) {
            return (*builtin_func[i])(args);
        }
    }

    int j = 0;

    while(args[j] != NULL) {
        // for `>` operator for redirection (stdout)
        if( !strcmp(">", args[j]) ) {
            int fd = fileno(fopen(args[j+1], "w+"));
            args[j] = NULL;
            return launch(args, fd, KHOL_FG);
        }
        // for `>>` operator for redirection (stdout with append)
        else if( !strcmp(">>", args[j]) ) {
            int fd = fileno(fopen(args[j+1], "a+"));
            args[j] = NULL;
            return launch(args, fd, KHOL_FG | KHOL_STDOUT);
        }
        // for `2>` operator for redirection (stderr)
        else if( !strcmp("2>", args[j]) ) {
            int fd = fileno(fopen(args[j+1], "w+"));
            args[j] = NULL;
            return launch(args, fd, KHOL_FG | KHOL_STDERR);
        }
        // for `>&` operator for redirection (stdout and stderr)
        else if( !strcmp(">&", args[j]) ) {
            int fd = fileno(fopen(args[j+1], "w+"));
            args[j] = NULL;
            return launch(args, fd, KHOL_FG | KHOL_STDERR | KHOL_STDOUT);
        }
        // for `<` operator for redirection (stdin)
        else if( !strcmp("<", args[j]) ) {
            int fd = fileno(fopen(args[j+1], "r"));
            args[j] = NULL;
            return launch(args, fd, KHOL_FG | KHOL_STDIN);
        }
        // for piping
        else if( !strcmp("|", args[j]) ) {
            char** arg2;
            int i = 0;
            args[j] = NULL;
            arg2 = &args[j+1];

            return pipe_launch(args, arg2);
        }
        j++;
    }

    return launch(args, STDOUT_FILENO, KHOL_FG);
}

/* Function that splits the line based on delimeters defined in constants.h */
char **split_line(char *line) {
    char **tokens = malloc(sizeof(char*) * TOKEN_BUFSIZE);
    char *token;

    int bufsize_copy = TOKEN_BUFSIZE
    int pos = 0;

    if(!tokens) {
        fprintf(stderr, RED "khol: Memory allocation failed." RESET);
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOKEN_DELIMS);
    while(token != NULL) {
        tokens[pos] = token;
        pos++;


        if(pos >= bufsize_copy) {
            bufsize_copy = bufsize_copy * 2
            tokens = realloc(tokens, sizeof(char*) * bufsize_copy);

            if(!tokens) {
                fprintf(stderr, RED "khol: Memory allocation failed." RESET);
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOKEN_DELIMS);
    }

    tokens[pos] = NULL;
    return tokens;
}

/* Function that returns the prompt */
char *get_prompt(void) {
    char *prompt, tempbuf[PATH_MAX];

    size_t prompt_len = sizeof(char) * PROMPT_MAXSIZE;

    prompt = malloc(prompt_len);

    if( getcwd( tempbuf, sizeof(tempbuf) ) != NULL ) {

        snprintf(prompt, prompt_len, "%s > ", tempbuf);
        return prompt;
    }
    else {
        return NULL;
    }
}

/* The main loop of the shell */
void main_loop(void) {
    char *line;
    char **args;

    int status, index;

    // get prompt
    char *prompt;

    char *homedir = getenv("HOME");

    size_t history_path_len = sizeof(char) * PATH_MAX;
    history_path = malloc(history_path_len);
    snprintf(history_path, history_path_len, "%s/%s", homedir, HISTFILE);

    // read from history file
    read_history(history_path);

    do {
        // use GNU's readline() function
        if( (prompt = get_prompt()) == NULL) {
            status = 0;
            fprintf(stderr, RED "khol: Failed to get prompt.\n" RESET);
        }
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

            char* history_copy = (char*) malloc(MAX_BUFSIZE * sizeof(char*));

            if(line[0] == '!' && line[1] == '-') {
                if(sscanf(line, "!-%d", &index) != EOF) {
                    strcpy(history_copy, history_get(history_length - index)->line);
                    args = split_line(history_copy);
                }
                else
                    fprintf(stderr, RED "khol: Expected digit after '!-' for history recollection.\n." RESET);

            }
            else if(line[0] == '!' ) {
                if(sscanf(line, "!-%d", &index) != EOF) {
                    strcpy(history_copy, history_get(index)->line);
                    args = split_line(history_copy);
                }
                else
                    fprintf(stderr, RED "khol: Expected digit after '!' for history recollection.\n" RESET);
            }
            else {
                args = split_line(line);
            }

            if(args) {
                status = execute(args);
                free(args);
            }
            else {
                status = 1;
            }
        }

        free(prompt);
        free(line);

    } while ( status );

    free(history_path);
}

int main(int argc, char* argv[]) {
    main_loop();

    return EXIT_SUCCESS;
}
