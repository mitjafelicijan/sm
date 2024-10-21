#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdarg.h>
#include <getopt.h>
#include <libtar.h>
#include <fcntl.h>

#define BUFFER_SIZE 256*4

const char *ignored_directories[] = {
	".git",
	".venv",
	".zig-cache",
	"node_modules",
};

int num_ignored_directories = sizeof(ignored_directories) / sizeof(ignored_directories[0]);

typedef struct {
	TAR *tar;
	int verbose;
	const char *project_name;
	const char *tag;
	const char *directory_path;
	char snapshot_file[BUFFER_SIZE];
} Snapshot;

const char *get_cwd_basename(const char *arg0) {
	char buffer[BUFFER_SIZE];

	char *cwd = getcwd(buffer, BUFFER_SIZE);
	if (cwd == NULL) {
		fprintf(stderr, "Error: getting current directory %s.\n", arg0);
		exit(1);
	}

	char *base = basename(buffer);
	char *b = malloc(strlen(base) + 1);
	if (b == NULL) {
		fprintf(stderr, "Error: memory allocation failed.\n");
		exit(1);
	}
	strncpy(b, base, strlen(base) + 1);

	return b;
}

void add_to_snapshot(TAR *tar, const char *directory_path, int verbose) {
	struct dirent *entry;
	DIR *dir = opendir(directory_path);
	if (dir == NULL) {
		fprintf(stderr, "Error: could not open dir %s.\n", directory_path);
		exit(1);
	}

	struct stat s;
	char file_path[BUFFER_SIZE];

	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		snprintf(file_path, sizeof(file_path), "%s/%s", directory_path, entry->d_name);
		stat(file_path, &s);

		if (S_ISDIR(s.st_mode)) {
			int skip = 0;
			for (int i = 0; i < num_ignored_directories; i++) {
				if (strcmp(basename(file_path), ignored_directories[i]) == 0) {
					skip = 1;
				}
			}

			if (skip == 0) {
				add_to_snapshot(tar, file_path, verbose);
			} else {
				if (verbose > 0) {
					fprintf(stdout, "- %s\n", file_path); 
				}
			}
		} else {
			if (verbose > 0) {
				fprintf(stdout, "+ %s\n", file_path);
			}

			if (tar_append_file(tar, file_path, file_path) == -1) {
				fprintf(stderr, "Error: could not append file %s.\n", file_path);
				exit(1);
			}
		}
	}

	closedir(dir);
}

void help(const char *argv0) {
	printf("Usage: %s [options]\n"
			"\nAvailable options:\n"
			"  -t,--tag=1.4         set tag of the snapshot\n"
			"  -h,--help            this help\n"
			"  -v,--verbose         show detailed log\n",
			argv0);
}

int main(int argc, char **argv) {
	Snapshot sn = {0};
	sn.tag = NULL;

	const char short_options[] = "t:vh";
	const struct option long_options[] = {
		{ "tag", 1, NULL, 't' },
		{ "help", 0, NULL, 'h' },
		{ "verbose", 0, NULL, 'v' },
		{ 0 },
	};

	int opt;
	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch (opt) {
			case 't': {
				sn.tag = optarg;
			} break;
			case 'v': {
				sn.verbose = 1;
			} break;
			case 'h': {
				help(argv[0]);
				return 0;
			} break;
			default: {
				fprintf(stdout, "Missing options. Check help.\n");
				return 1;
			} break;
		}
	}

	if (sn.tag == NULL) {
		/* free((void*)sn.project_name); */
		help(argv[0]);
		return 0;
	}

	sn.directory_path = ".";
	sn.project_name = get_cwd_basename(argv[0]);
	snprintf(sn.snapshot_file, sizeof(sn.snapshot_file), "../%s-v%s.tar", sn.project_name, sn.tag);

	fprintf(stdout, "tag:      %s\n", sn.tag);
	fprintf(stdout, "snapshot: %s\n", sn.snapshot_file);

	if (remove(sn.snapshot_file) == 0) {
		if (sn.verbose > 0) {
			fprintf(stdout, "! snapshot file %s deleted\n", sn.snapshot_file);
		}
	}

	if (tar_open(&sn.tar, sn.snapshot_file, NULL, O_WRONLY | O_CREAT, 0644, TAR_GNU) == -1) {
		fprintf(stderr, "Error: could not create tar file %s.\n", sn.snapshot_file);
		return 1;
	}

	add_to_snapshot(sn.tar, sn.directory_path, sn.verbose);

	if (tar_append_eof(sn.tar) == -1) {
		fprintf(stderr, "Error: could not append EOF to tar.\n");
		return 1;
	}

	if (tar_close(sn.tar) == -1) {
		fprintf(stderr, "Error: could not close tar file.\n");
		return 1;
	}

	/* free((void*)sn.project_name); */
	return 0;
}

