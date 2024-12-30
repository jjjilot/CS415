#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>


void lfcat()
{

/* High level functionality you need to implement: */

	/* Get the current directory with getcwd() */

	char cwd[1024];
	getcwd(cwd, sizeof(cwd));

	/* Open the dir using opendir() */

	DIR *dir;
	dir = opendir(cwd);
	
	/* use a while loop to read the dir with readdir()*/
	struct dirent *read_file;

	while ((read_file = readdir(dir)) != NULL ) {
		/* You can debug by printing out the filenames here */
        
		// printf("FILENAME: %s\n", read_file->d_name);

		/* Option: use an if statement to skip any names that are not readable files (e.g. ".", "..", 
        "main.c", "lab2.exe", "output.txt" */

		if (strcmp(read_file->d_name, ".") == 0 || strcmp(read_file->d_name, "..") == 0 
                    || strcmp(read_file->d_name, "lab2") == 0)
			continue;
			
			/* Open the file */

			int fd;
			fd = open(read_file->d_name, O_RDONLY);
	
			/* Read in each line using read() */

			char buffer[1024];
			size_t bytes;

			while((bytes = read(fd, buffer, sizeof(buffer))) != 0) {
				write(STDOUT_FILENO, buffer, bytes);
			}

			close(fd);

				/* Write the line to stdout */
			
			/* write 80 "-" characters to stdout */

			for (int i = 0; i < 80; i++) {
				write(STDOUT_FILENO, "-", strlen("-"));
			}
			
			/* close the read file and free/null assign your line buffer */
	}

	closedir(dir);
	/*close the directory you were reading from using closedir() */
}
