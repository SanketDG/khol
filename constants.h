/*
 * Some helpful constants for the shell
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

#define MAX_BUFSIZE 1024 /* Maximum size for the command buffer */
#define PROMPT_MAXSIZE 1024 /* Maximum size of the prompt */
#define TOKEN_BUFSIZE 64 /* Maximum size of each of the tokens in the command */
#define TOKEN_DELIMS " \t\r\n\a" /* token delimeters */

#define HISTFILE ".khol_history"

/* list of builtins in the shell */
char *builtins[] = {
    "cd",
    "help",
    "history",
    "exit"
};

/* terminal styles */
#define RESET "\e[0m"
#define BOLD "\e[1m"

/* terminal colors */
#define YELLOW "\e[33m"
#define RED "\e[31m"
