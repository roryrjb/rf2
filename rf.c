#include <stdbool.h>
#include <stdio.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi")

static void print_dir(const char *base_path, int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '!') {
      if (strstr(base_path, argv[i] + 1) != NULL) {
        return;
      }
    }
  }

  char search_path[MAX_PATH] = {'\0'};
  strcat_s(search_path, MAX_PATH, base_path);
  strcat_s(search_path, MAX_PATH, "\\*");

  WIN32_FIND_DATAA find_data;
  HANDLE handle = FindFirstFileA(search_path, &find_data);

  bool found = false;

  while (found = FindNextFileA(handle, &find_data)) {
    if (!strcmp(find_data.cFileName, "..") || !strcmp(find_data.cFileName, ".") || found == false) {
      continue;
    }

    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      char dir_path[MAX_PATH] = {'\0'};
      strcat_s(dir_path, MAX_PATH, base_path);
      strcat_s(dir_path, MAX_PATH, "\\");
      strcat_s(dir_path, MAX_PATH, find_data.cFileName);
      print_dir(dir_path, argc, argv);
    } else {
      bool matched = false;

      for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '!') {
          if (PathMatchSpecA(find_data.cFileName, argv[i] + 1) == true) {
            matched = false;
          }
        }

        if (PathMatchSpecA(find_data.cFileName, argv[i]) == true) {
          matched = true;
        }
      }

      if (matched) {
        printf("%s\\%s\n", base_path, find_data.cFileName);
      }
    }
  }

  FindClose(handle);
}

void usage() {
  printf("Recursive Finder\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage();
    return 1;
  }

  const char *root = ".";
  print_dir(root, argc, argv);
}
