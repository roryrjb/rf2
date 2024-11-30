#include <ctype.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "dirent/include/dirent.h"
#include "getopt/getopt.h"
#include <direct.h>
#include <shlwapi.h>
#include <shlobj.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "shell32")
#define RFIGNORE_PATTERN "%s\\%s"

#include "ignore.h"

#define VERSION "2.0.0-alpha"
#define RFIGNORE ".rfignore"

struct ignores *global_ignores;
struct ignores *local_ignores;

struct switches {
	int count;
	int invert;
	int limit;
	int substring;
	int wholename;
};

static void usage(char *error) {
	if (error != NULL) {
		fprintf(stderr, "Error: %s\n\n", error);
	}

	fprintf(stderr, "rf version %s\n\n", VERSION);
	fprintf(stderr, "usage: rf [-d directory] [-lsvw] pattern ...\n");
}

static int is_child(char *dirname) {
	if (strcmp("..", dirname) == 0 || strcmp(".", dirname) == 0) {
		return 0;
	}

	return 1;
}

static bool excluded(const char *name) {
	if (global_ignores != NULL) {
		for (int i = 0; i < global_ignores->size; i++) {
			if (PathMatchSpecA(name, global_ignores->list[i])) {
				return true;
			}
		}
	}

	if (local_ignores != NULL) {
		for (int i = 0; i < local_ignores->size; i++) {
			if (PathMatchSpecA(name, local_ignores->list[i])) {
				return true;
			}
		}
	}

	return false;
}

/* return 1 if breaking early (e.g. reaching limit) otherwise return 0 */
static int recurse_find(char **patterns, int *pattern_count, const char *dirname, struct switches *switches) {
	DIR *dir;

	char path[MAX_PATH] = {'\0'};
	strcat_s(path, MAX_PATH, dirname);
	dir = opendir(path);

	if (dir != NULL && !excluded(dirname)) {
		struct dirent *entry;

		while ((entry = readdir(dir)) != NULL) {
			int matched = 0;
			int p = 0;

			char full_path[MAX_PATH] = {'\0'};
			strcat_s(full_path, MAX_PATH, path);

			if (full_path[strlen(full_path) - 1] != '\\') {
				strcat_s(full_path, MAX_PATH, "\\\\");
			}

			strcat_s(full_path, MAX_PATH, entry->d_name);

			struct stat entry_stat;

			if (stat(full_path, &entry_stat)) {
				fprintf(stderr, "%s: ", full_path);
				perror("stat");
				continue;
			}

			if (entry_stat.st_mode & S_IFDIR) {
				if (is_child(entry->d_name) && !excluded(entry->d_name)) {
					if (recurse_find(patterns, pattern_count, full_path, switches)) {
						closedir(dir);
						return 1;
					};
				}
			} else if (entry_stat.st_mode & S_IFREG) {
				if (excluded(entry->d_name)) {
					continue;
				}

				for (; p < *pattern_count; p++) {
					char *pattern = patterns[p];

					if (switches->substring) {
						if (strstr(switches->wholename ? full_path : entry->d_name, pattern) != NULL) {
							matched = 1;
						}
					} else {
						if (PathMatchSpecA(switches->wholename ? full_path : entry->d_name, pattern)) {
							matched = 1;
						}
					}
				}

				if (switches->invert) {
					if (matched) {
						matched = 0;
					} else {
						matched = 1;
					}
				}
			}

			if (matched) {
				if (is_child(entry->d_name) != 0) {
					printf("%s\n", full_path);
				}

				if (switches->limit > 0 && ++switches->count == switches->limit) {
					closedir(dir);
					return 1;
				}
			}
		} /* while */

		closedir(dir);
	} /* if */

	return 0;
}

int main(int argc, char **argv) {
	struct switches switches = {
		.count = 0,
		.invert = 0,
		.limit = 0,
		.substring = 0,
		.wholename = 0,
	};

	int exit_code = EXIT_SUCCESS;
	int ch;
	char *remainder;
	const char *root = ".";
	char cwd[MAX_PATH];
	int unmatched_error = 0;

	if (_getcwd(cwd, MAX_PATH) == NULL) {
		perror("getcwd");
		exit(EXIT_FAILURE);
	}

	TCHAR home[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, home);

	while ((ch = getopt(argc, argv, "d:l:svw")) > -1) {
		switch (ch) {
		case 'd':
			root = optarg;
			break;

		case 'h':
			usage(NULL);
			exit(EXIT_SUCCESS);

		case 's':
			switches.substring = 1;
			break;

		case 'v':
			switches.invert = 1;
			break;

		case 'w':
			switches.wholename = 1;
			break;

		case 'l':
			do {
				int limit = strtol(optarg, &remainder, 10);

				if (limit < 0) {
					usage("Invalid limit.");
					exit(EXIT_FAILURE);
				}

				switches.limit = limit;
			} while (0);

			break;
		}
	}

	char global_ignore_path[MAX_PATH];
	char local_ignore_path[MAX_PATH];

	snprintf(global_ignore_path, MAX_PATH, RFIGNORE_PATTERN, home, RFIGNORE);
	snprintf(local_ignore_path, MAX_PATH, RFIGNORE_PATTERN, cwd, RFIGNORE);

	global_ignores = init_ignores(global_ignore_path);
	local_ignores = init_ignores(local_ignore_path);

	if (optind < argc) {
		int i = 0;
		int pattern_count = argc - optind;
		char **patterns = (char **)calloc(sizeof(char *), pattern_count);

		while (optind < argc) {
			patterns[i++] = argv[optind++];
		}

		recurse_find(patterns, &pattern_count, root, &switches);
		free(patterns);

		if (unmatched_error && switches.count == 0) {
			exit_code = EXIT_FAILURE;
			goto bail;
		}
	} else {
		usage(NULL);
	}

bail:
	free_ignores(global_ignores);
	free_ignores(local_ignores);

	exit(exit_code);
}
