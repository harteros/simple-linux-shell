//Copyright 2018, Lefteris Harteros, All rights reserved.

#ifndef MYSH_H_
#define MYSH_H_

char *read_input(void);
char ** tokenize(char* input);
int execute(char** commands);
int shell();

int execute_command(char** commands);
int execute_pipe(char** commands,int coms);

void exec(char** commands);
void dup_n_close(int count,int coms,int fd[],int fd2[],int process);

void copy(char** commands,int pos,int write);
int count_commands(char** commands);

#endif
