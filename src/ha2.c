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
		if (!strcmp(command, "mkdir")) {
			LOG("Chosen mkdir\n");
			fs_mkdir(fs, strtok(NULL, " \n"));
		} else if (!strcmp(command, "mkfile")) {
			fs_mkfile(fs, strtok(NULL, " \n"));
			LOG("Chosen mkfile\n");
		} else if (strcmp(command, "cp") == 0) {
        fs_cp(fs, strtok(NULL, " \n"),  strtok(NULL, " \n"));
				LOG ("Chosen Copyfile\n");
    } else if (!strcmp(command, "list")) {
			LOG("Chosen list\n");
			char *output = fs_list(fs, strtok(NULL, " \n"));
			printf("%s", output);
			free(output);
		} else if (!strcmp(command, "writef")) {
			char *path = strtok(NULL, " \n");
			char *text = strtok(NULL, "\0");
			fs_writef(fs, path, text);
			LOG("Chosen writef\n");
		} else if (!strcmp(command, "readf")) {
			LOG("Chosen readf\n");
			int file_size = 0;
			char *output  = (char *)fs_readf(fs, strtok(NULL, " \n"), &file_size);
			fwrite(output, file_size, 1, stdout);
			fflush(stdout);
			free(output);
		} else if (!strcmp(command, "rm")) {
			LOG("Chosen rm\n");
			fs_rm(fs, strtok(NULL, " \n"));
		} else if (!strcmp(command, "export")) {
			char *int_path = strtok(NULL, " \n");
			char *ext_path = strtok(NULL, "\0");
			fs_export(fs, int_path, ext_path);
			LOG("Chosen export\n");
		} else if (!strcmp(command, "import")) {
			char *int_path = strtok(NULL, " \n");
			char *ext_path = strtok(NULL, "\0");
			fs_import(fs, int_path, ext_path);
		} else if (!strcmp(command, "dump")) {
			LOG("Saving filesystem to disk\n");
			fs_dump(fs, argv[2]);
		} else if (!strcmp(command, "exit") || !strcmp(command, "quit")) {
			cleanup(fs);
			free(input_buf);
			exit(0);
		} else {
			LOG("Unknown command\nValid commands:\nlist\nmkfile\nmakedir\ncp\nrm\nexport\nimport\nwritef\nreadf\ndump\n");
		}
		free(input_buf);
	}
}
