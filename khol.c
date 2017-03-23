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

int (*builtin_func[]) (char**) = {
    &khol_cd,
    &khol_help,
    &khol_history,
    &khol_exit
};

int num_builtins() {
    return sizeof(builtins) / sizeof(char *);
}

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


int launch(char **args, int fd, int options) {

    int khol_bg = 1 ? options & KHOL_BG : 0;
    int khol_stdout = 1 ? options & KHOL_STDOUT : 0;
    int khol_stderr = 1 ? options & KHOL_STDERR : 0;
    int khol_stdin = 1 ? options & KHOL_STDIN : 0;

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

int khol_history(char **args) {
    char *history_args[4] = {"cat", "-n", history_path, NULL};
    return launch(history_args, STDOUT_FILENO, KHOL_FG);
}

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
        j++;
    }

    return launch(args, STDOUT_FILENO, KHOL_FG);
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

    size_t prompt_len = sizeof(char) * PROMPT_MAXSIZE;

    prompt = malloc(prompt_len);

    if( getcwd( tempbuf, sizeof(tempbuf) ) != NULL ) {

        snprintf(prompt, prompt_len, "%s > ", tempbuf);
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

    size_t history_path_len = sizeof(char) * PATH_MAX;
    history_path = malloc(history_path_len);
    snprintf(history_path, history_path_len, "%s/%s", homedir, HISTFILE);

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

            if(line[0] == '!' && !line[1]) {
                int index;
                sscanf(line, "!%d", &index);
                args = split_line(history_get(index)->line);
            }
            else if(line[0] == '!' && line[1] == '-') {
                int index;
                sscanf(line, "!-%d", &index);

                args = split_line(history_get(history_length - index + 1)->line);
            }
            else {
                args = split_line(line);
            }

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
