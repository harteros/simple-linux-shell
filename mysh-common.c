//Copyright 2018, Lefteris Harteros, All rights reserved.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "mysh.h"


//------------------------------------------main functions---------------------------------//


//-------------------------------input read---------------------------------//
char *read_input(void){
	
	int size=255;
	int pos=0;
	char* buffer=malloc(size*sizeof(char)); //allocates memory for the buffer
	if (buffer == NULL) {					//checks if allocation was successfull
		perror("ERROR: Not enough memory.\n");
		return NULL;
	}
	char ch;	

	while(1){

		ch=getchar();						//read the input of the user
		if(pos>=size) {						//check if the size of the input is bigger than the allocated memory
			printf("Input larger than expected. Program will now exit.\n");		
			return NULL;
		}	
		if(ch==EOF || ch=='\n'){			
			buffer[pos]='\0';				//when the program reads the \n put \0 at the end of the buffer to show that there is no more input
			if(ch==EOF){					//if the character read is the EOF end the program
				printf("EOF entered. Program will now exit.\n");
				return NULL;				
			}else return buffer;			//returns a buffer with the input of the user
		}else{
			buffer[pos]=ch;
			pos++;
		}	
	}		
}

//--------------------------------input tokenize----------------------------//

char ** tokenize(char* input){

	int size=64;
	char** commands=malloc(size*sizeof(char*)); //allocates memory for the array with the commands
	if (commands == NULL) {						//checks if allocation was successfull
		perror("ERROR: Not enough memory.\n");
		return NULL;
	}
	int pos=0;
	char* token=strtok(input," ");				//tokenize the user input based on the blank delimeter

	while(token!=NULL){							//while we havent read the whole input continue
		commands[pos]=token;					//and put in each place of the array one command 
		token=strtok(NULL," ");
		pos++;
	}
	commands[pos]=NULL;							//after the last command in the array we put NULL so that  the program knows that the array ends there
	return commands;							//returns an array with the input of the user being put in cells 
}

//----------------------command execution WITHOUT pipe-----------------------//

int execute_command(char** commands){

	pid_t pid, waitPid;
	int  status;
	
	pid = fork();						//forks and creates 2 processes 1 parent and 1 child
	
	
	if (pid < 0) {						//checks if fork was successfull
		perror("ERROR: Fork failed.\n");
		return -1;
	}
	if (pid == 0) {						//if we are at the child process
		

		exec(commands);					//calls exec method to execute the commands

	}else {								//if we are at the parent process

		waitPid = wait(&status);		//waits for the child to finish
		if (waitPid == -1) {			//checks if waiting was successfull
			perror("ERROR: Waitpid failed.\n");
			return -1;
		}
	}


	return 0;							//returns 0 if everything went well else -1
}

//------------------------command execution WITH pipe-----------------------//

int execute_pipe(char** commands , int coms ){

	pid_t pid,waitPid;
	int check_pipe,status,fd1[2],fd2[2]; 
	char **command=malloc(64*sizeof(char*)); //allocates memory for the array which will hold the commands till the character "|""
	if (command == NULL) {					//checks if allocation was successfull
		perror("ERROR: Not enough memory.\n");
		return -1;
	}
	int count=0,pos=0,more=1;
	
	while (commands[pos] != NULL && more == 1){ //while there are more commands in the array and 
												//while we are copying commands from our array to our new array we dont reach the end of the first array
		int i = 0;
		while (strcmp(commands[pos],"|") != 0){ //while we dont find the pipe character
			command[i] = commands[pos];			//copy the contect of our first array to the new array we created
			i++;
			pos++;	
			if (commands[pos] == NULL){			//while you are copying the contect if there are no more commands in the first array
				more = 0;						//change more to 0 to show that there is nothig more to read and stop the loop
				break;
			}
		}

		command[i] = NULL;					//after the last command in the array we put NULL so that  the program knows that the array ends there
		pos++;		
											//we will open pipes each time based on the number of loops we have done 
		if (count % 2 == 0){				//for even number open the first pipe
			check_pipe=pipe(fd1);
		}else{
			check_pipe=pipe(fd2); 			//for odd number open the second pipe
		}
		if(check_pipe==-1){					//checks if pipe creation was successfull
			printf("ERROR: Pipe command failed.\n");
			return -1;
		}

		pid=fork();							//forks and creates 2 processes 1 parent and 1 child
		
		if(pid<0){							//checks if fork was successfull
						
			perror("ERROR: Fork failed.\n");
			return -1;
		}
		if(pid==0){							//if we are at the child process
			
			dup_n_close(count,coms,fd1,fd2,pid);	//calls dup_n_close method to open the pipes needed to make the right redirections
			
			exec(command);					//calls exec method to execute the commands
		}else{								//if we are at the parent process
				
			dup_n_close(count,coms,fd1,fd2,pid);	//calls dup_n_close method to close the pipes that were opened by the child process
				
			waitPid = wait(&status);	//waits for the child to finish
			if (waitPid == -1) {		//checks if waiting was successfull
				perror("ERROR: Waitpid failed.\n");
				return -1;
			}
		}		
		count++;	
	}
	return 0;				//returns 0 if everything went well else -1
}

//------------------------------execution choice------------------------------//

int execute(char** commands){
	
	int out;
	int coms=count_commands(commands); 	//calls count_commands method to check if there is pipe or not (if it returns 1 there is no pipe else there is)
	if(coms==1){						//if there are no pipes calls the method execute_command to execute a simple command
		out=execute_command(commands);
	}else{								//if there are pipes calls the method execute_pipe to execute piped commands
		out=execute_pipe(commands,coms);
	}
	return out;				//returns 0 if the methods executed successfully else -1
}

//------------------------------working shell------------------------------//

int shell(){

	char* input=read_input();		//calls read_input method to read the users input

	if(input==NULL) return -1;		//if user entered EOF or something went wrong during reading stops the shell

	char** commands=tokenize(input);	//calls tokenize method to break down the input of the user into commands
	
	if(commands==NULL) return -1;	//if commands array could not be created stops the shell

	int out=execute(commands);		//calls method execute to execute the commands given by the user
		
	return out;			//returns 0 if the commands executed successfully else -1
}	

//------------------------------------------secondary functions---------------------------------//


//----------------------------I/O redirection function------------------------//

void exec(char** commands){
		
	int infile,outfile,rd=0,wr=0,app=0,pos=0,index_rd=-1,index_wa=-1;	
	char* destination=NULL;

	while(commands[pos]!=NULL){	
	
		if(strcmp(commands[pos],"<")==0){	//if < character is found
			rd=1;							//make rd=1 which shows that we found the reading redirection
			index_rd=pos;
		}
		if(strcmp(commands[pos],">")==0){	//if > character is found
			wr=1;							//make wr=1 which shows that we found writing redirection 
			index_wa=pos;					//and save the name of the file where we will redirect in destination
			destination= commands[pos+1];	
		}
		if(strcmp(commands[pos],">>")==0){	//if >> character is found
			app=1;							//make app=1 which shows that we found append redirection 
			index_wa=pos;					//and save the name of the file where we will redirect in destination
			destination=commands[pos+1];
		}
			pos++;
		}
		
		mode_t mode=S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH; // rwx for everyone


		if(wr){	//if the write redirection operator is found
			copy(commands,index_wa,1);	//call copy method to change the commands array cells shift them 2 places left from the position we found th redirection operator
			outfile=open(destination,O_TRUNC|O_WRONLY|O_CREAT,mode);	//open the file where we will write with the proper methods(O_TRUNC)
			dup2(outfile,1);	// redirect its output to st_out which means everything that was gonna print on the screen will be written to the file
			close(outfile);		//close the file

		}
		if(app){ //if the append redirection operator is found
			copy(commands,index_wa,1);	//call copy method to change the commands array cells shift them 2 places left from the position we found th redirection operator
			outfile=open(destination,O_APPEND|O_WRONLY|O_CREAT,mode);	//open the file where we will append with the with the proper methods(O_APPEND)
			dup2(outfile,1);	// redirect output to st_out which means everything that was gonna print on the screen will be written to the file
			close(outfile);		//close the file
		}
		if(rd){ //if the read redirection operator is found
			copy(commands,index_rd,0);	//call copy method to change the commands array cells shift them 1 place left from the position we found th redirection operator
			infile=open(commands[index_rd],O_RDONLY);	//open the file where we will append with the with the proper methods(O_RDONLY)
			dup2(infile,0);		// redirect input to st_in which means everything that we now read from the file
			close(infile);		//close the file
		}		
		execvp(commands[0],commands);	//executes the command that was given and returns to the parent process
		perror("ERROR: Could not execute command.\n"); //if command was not executed prints this error message
		exit(1);	//and returns to the parent process
}


//---------------------------------pipe function------------------------------//


void dup_n_close(int count,int coms,int fd1[],int fd2[],int process){

	if(process==0){	//if we are at the child process we dup the file descriptors
		if (count != coms-1){ //if we are not at the last command
			if(count==0){	//if we are at the first command
				dup2(fd1[1],1);	//redirect std_out to fd1[1] cause as we said at the execute_pipe method
								//in order to achieve multi piping for even commands we pipe fd1 and for odd we pipe fd2
								//and cause at the first command count=0 which means even number we dup2 the fd1
			}else{	//if we are at a middle command we use to pipes ones for the input and one for the output
					//cause basically what we do is we take the output of the previous command and make it input
					//for the next command and so on till the last command
				if (count % 2 == 0){	//if it is even then we
					dup2(fd2[0],0); 	//redirect input to fd2[0] (read-end)
					dup2(fd1[1],1);		//and output to fd1[1] (write-end)
				}else{ 					//else if its odd we
					dup2(fd1[0],0); 	//redirect input to fd1
					dup2(fd2[1],1);		//and output to fd2			
				} 
			}
		}else{	//if we are at the last command
			if (coms % 2 == 0){ //if the number of the commands is even 
				dup2(fd1[0],0); //redirect input to fd1[0] (read-end)
			}else{ 				//if the number of the commands is even 
				dup2(fd2[0],0);	//redirect input to fd2[0] (read-end)
			}
		}
	}else{	//if we are at the parent process we close the file descriptors
			//with the same if else statements close the file descriptors we opened at the child process
		if (count != coms-1){
			if(count==0){
				close(fd1[1]);
			}else{
				if (count % 2 == 0){					
					close(fd2[0]);
					close(fd1[1]);
				}else{					
					close(fd1[0]);
					close(fd2[1]);
				}
			}
		}
		else{
			if (coms % 2 == 0){					
				close(fd1[0]);
			}else{					
				close(fd2[0]);
			}
		}
	}
}

//-----------------------------count and copy functions--------------------------//

int count_commands(char** commands){
	
	int pos=0,count=1;

	while(commands[pos]!=NULL){		//while there are commands
		if(strcmp(commands[pos],"|")==0){	//if pipe operator is found increase count by 1
			count++;		
		}
		pos++;	//increase position of the array
	}
	return count; // return the number of commands 1 if there are no pipes else the number of commands found (1 pipe=2 commands etc...)

}



void copy(char** commands,int pos,int write){
	
	int i,cpy;
	for(i=0; i<=write; i++){	//executes the for 1 or 2 times which means shift 1 or 2 places left 
								//if read then we need to shift only 1 place left to remove the read operator
								//else we need to shift 2 places to remove write or append operator and the file where we will write
								//as it is already saved
		cpy=pos;				//save at cpy the position from where we will shift
		while(commands[cpy]!=NULL){	//while there are commands
	
				if(commands[cpy+1]!=NULL){	//if at the next position there are commands
					commands[cpy]=commands[cpy+1]; //move them to the previous cell
				}else{
					commands[cpy]=NULL;	//else put NULL into the cell showing that there are no more commands and deleting like this the
										//previous content of the cell
					break;
				}
		
			cpy++;
		}
	}
}




