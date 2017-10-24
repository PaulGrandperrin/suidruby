#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

int main(int argc, char** argv, char** envp)
{
	struct stat sb;

	if(argc < 2) {
		fprintf(stderr, "You must pass a ruby script path as an argument\n");
		exit(EXIT_FAILURE);
	}

	if(getuid() == 0 && getgid() == 0)
		goto exec;

	if(geteuid() != 0 || getegid() != 0) {
		fprintf(stderr, "The SUID bit is not correctly set on the wrapper's executable: %s\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(stat(argv[1], &sb) == -1) {
		fprintf(stderr, "Failed to open %s:%s\n", argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(sb.st_uid != geteuid() || sb.st_gid != getegid() ||  (S_IWOTH | S_IWGRP) & sb.st_mode || !(sb.st_mode & S_ISUID)) {
		fprintf(stderr, "The script must be owned by root as a user and as a group, not writable by group and others and have the suid bit\n");
		exit(EXIT_FAILURE);
	}
	
	// Get euid rights
	if(setuid(geteuid())) {
		perror("setuid");
		exit(EXIT_FAILURE);
	}

	// Get egid rights
	if(setgid(getegid())) {
		perror("setgid");
		exit(EXIT_FAILURE);
	}

	exec:

	// envp = NULL; // Totaly unsafe to pass the environment as is but really hard to avoid too...

	execvpe("ruby", argv, envp); // Launch the ruby interpretor

	perror("execvpe");
	return errno;
}

