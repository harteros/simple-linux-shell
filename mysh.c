//Copyright 2018, Lefteris Harteros, All rights reserved.

#include <stdio.h>
#include "mysh.h"


int main(int argv,char ** argc){

	while(1){ //while true

	 	printf("mysh>");

		int out=shell(); //calls method shell which will return 0 if everything went well esle -1

		if(out==-1) break; //if -1 is returned from shell which means there is an error we end the program
	}
	return 0;
}


