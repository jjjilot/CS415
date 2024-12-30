#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include "command.h"
#include "string_parser.h"
#include <errno.h>
#include <libgen.h>

void displayFile(char* filename) {   

    int f_outd = fileno(f_out);
    int f_ind;
    f_ind = open(filename, O_RDONLY);
    if (f_ind == -1) { error = 1; return; }
			
    if (f_out == stdout) {
    f_outd = STDOUT_FILENO;
    }

    /* Read in each line using read() */
    char buffer[1024];
    size_t bytes;

    while((bytes = read(f_ind, buffer, sizeof(buffer))) != 0) { write(f_outd, buffer, bytes); }

    const char newline = '\n';
    ssize_t bytes_written = write(f_outd, &newline, 1);
    
    close(f_ind);
}

void listDir() {

    int f_outd = fileno(f_out);
	char cwd[1024];
    getcwd(cwd, sizeof(cwd));

	DIR *dir;
	dir = opendir(cwd);
	struct dirent *read_file;

    char buffer[1024];

    if (f_out == stdout) {
    f_outd = STDOUT_FILENO;
    }

	while ((read_file = readdir(dir)) != NULL) {
        if (strcmp(read_file->d_name, "tmp") == 0) {
            continue; }

        buffer[0] = '\0';
        strcpy(buffer, read_file->d_name);
        strcat(buffer, " ");

        write(f_outd, buffer, strlen(buffer));
    }
    const char newline = '\n';
    ssize_t bytes_written = write(f_outd, &newline, 1);
    closedir(dir);
}

void copyFile(char *sourcePath, char *destinationPath) {	 

    struct stat statbuf;

    // Check if the destination is a directory
    if (stat(destinationPath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
        // Append the source file name to the destination directory path
        char *srcFileName = basename(sourcePath);
        char newDestPath[1024]; 
        snprintf(newDestPath, sizeof(newDestPath), "%s/%s", destinationPath, srcFileName);
        destinationPath = newDestPath; 
    }

    int src = open(sourcePath, O_RDONLY);
    // error checking
    if (src == -1) {
        error = 1;
        return; 
    }

    int dst = open(destinationPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    char buffer[1024];
    ssize_t b_read, b_write;

    while ((b_read = read(src, buffer, sizeof(buffer))) > 0) {
        b_write = write(dst, buffer, b_read);
    }

    // Close both files
    close(src);
    close(dst);
}

void moveFile(char *sourcePath, char *destinationPath) {

    // Error checking pt 1
    if (access(sourcePath, F_OK) == -1 || access(sourcePath, R_OK) == -1) {
        error = 1;
        return;
    }

    struct stat srcStat, destStat;
    char *filename = basename(sourcePath);

    // Check if destinationPath exists
    if (stat(destinationPath, &destStat) == 0) {
        if (S_ISDIR(destStat.st_mode)) {
            int newPathLength = strlen(destinationPath) + strlen(filename) + 2;
            char newPath[newPathLength];

            strcpy(newPath, destinationPath);
            strcat(newPath, "/");
            strcat(newPath, filename);

            // Move the file to the new path
            if (rename(sourcePath, newPath) == -1) {
                error = 1;
                return;
            }
        }
        // If destination is a file, move and overwrite the destination file
        else if (S_ISREG(destStat.st_mode)) {
            if (rename(sourcePath, destinationPath) == -1) {
                error = 1;
                return;
            }
        } else {
            error = 1;
            return;
        }
    } 
    // Error checking pt 2
    else if (errno == ENOENT) {
        if (rename(sourcePath, destinationPath) == -1) {
            error = 1;
            return;
        }
    } else {
        error = 1;
        return; 
    }
}

void showCurrentDir() {
    // write to file
    int f_outd = fileno(f_out);

    if (f_out == stdout) {
    f_outd = STDOUT_FILENO;
    }

	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	write(f_outd, cwd, strlen(cwd));
    // add newline
    const char newline = '\n';
    ssize_t bytes_written = write(f_outd, &newline, 1);
}

void deleteFile(char* filename) {
    // check if file is real
    if (access(filename, F_OK) == -1 || access(filename, R_OK) == -1) {
        error = 1;
        return;
    }
    int del = 0;
    del = remove(filename);
}

void makeDir(char* dirName) {
	
    struct stat st;

    // Check if the directory already exists
    if (stat(dirName, &st) == 0 && S_ISDIR(st.st_mode)) {
        error = 1;
        return;
    }
    
    int mkd;
	mkd = mkdir(dirName, 0777);
}

void changeDir(char* dirName) {
    
    int chd;
    chd = chdir((const char*)dirName);
    if (chd == -1) {
        error = 1;
        return;
    }
}