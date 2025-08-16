#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/filesystem.h"
#include "../lib/linenoise.h"
#include "../lib/operations.h"
#include "../lib/utils.h"

int
main(int argc, const char *argv[])
{
	file_system *fs = NULL;
	if (argc < 2) {
		fprintf(stderr,
		        "No arguments given. You must either load a filesystem or create a new one.\n\n");
		printhelp();
		exit(1);
	} else if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--create") == 0) {
		if (argc < 4) {
			fprintf(stderr, "Not enough arguments given\n");
			printhelp();
			exit(1);
		} else {
			fs = fs_create(argv[2], (uint32_t)atol(argv[3]));
		}
	} else if (strcmp(argv[1], "-l") == 0 || strcmp(argv[1], "--load") == 0) {
		fs = fs_load(argv[2]);
	} else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		printhelp();
	}
	else{
		fprintf(stderr, "Unknown argument.\n");
		printhelp();
		exit(1);
	}

	linenoiseHistorySetMaxLen(20);

	while (1) {
		char *input_buf = linenoise("user@SPR: ");
		if (input_buf != NULL) {
			linenoiseHistoryAdd(input_buf);
		} else {
			continue;
		}
		char *command = strtok(input_buf, " \n");
		
		if(command == NULL){
			LOG("Unknown command\nValid commands:\nlist\nmkfile\nmakedir\ncp\nrm\nexport\nimport\nwritef\nreadf\ndump\n");
			free(input_buf);
			continue;
		}

		//determine which command to execute (only our build in commands are possible)
		int res = 0;
		if (!strcmp(command, "mkdir")) {
			res = fs_mkdir(fs, strtok(NULL, " \n"));
		} else if (!strcmp(command, "mkfile")) {
			res = fs_mkfile(fs, strtok(NULL, " \n"));
		} else if (strcmp(command, "cp") == 0) {
        	res = fs_cp(fs, strtok(NULL, " \n"),  strtok(NULL, " \n"));
    	} else if (!strcmp(command, "list")) {
			char *output = (char *)fs_list(fs, strtok(NULL, " \n"));
			if(output){
				fwrite(output, strlen(output), 1, stdout);
				fflush(stdout);
				free(output);
			}
			else{
				res = -2;
			}
		} else if (!strcmp(command, "writef")) {
			char *path = strtok(NULL, " \n");
			char *text = strtok(NULL, "\0");
			res = fs_writef(fs, path, text);
		} else if (!strcmp(command, "readf")) {
			int file_size = 0;
			char *output  = (char *)fs_readf(fs, strtok(NULL, " \n"), &file_size);
			if(output){
				fwrite(output, file_size, 1, stdout);
				fflush(stdout);
				free(output);
				LOG("\n")
			}
			else{
				res = -2;
			}
		} else if (!strcmp(command, "rm")) {
			res = fs_rm(fs, strtok(NULL, " \n"));
		} else if (!strcmp(command, "export")) {
			char *int_path = strtok(NULL, " \n");
			char *ext_path = strtok(NULL, "\0");
			res = fs_export(fs, int_path, ext_path);
		} else if (!strcmp(command, "import")) {
			char *int_path = strtok(NULL, " \n");
			char *ext_path = strtok(NULL, "\0");
			res = fs_import(fs, int_path, ext_path);
		} else if (!strcmp(command, "dump")) {
			res = fs_dump(fs, argv[2]);
		} else if (!strcmp(command, "exit") || !strcmp(command, "quit")) {
			cleanup(fs);
			free(input_buf);
			exit(0);
		} else {
			LOG("Unknown command\nValid commands:\nlist\nmkfile\nmakedir\ncp\nrm\nexport\nimport\nwritef\nreadf\ndump\n");
		}

		if(res < 0){
			char output[20];
			switch(res){
				case -1:	sprintf(output, "not found!"); break;
				case -2:	sprintf(output, "failed"); break; 
			}
			fwrite(output, strlen(output), 1, stdout);
			fflush(stdout);
			LOG("\n");
		}
		free(input_buf);
	}
}
