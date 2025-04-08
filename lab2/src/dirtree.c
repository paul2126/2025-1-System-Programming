//--------------------------------------------------------------------------------------------------
// System Programming                         I/O Lab Spring 2025
//
/// @file
/// @brief resursively traverse directory tree and list all entries
/// @author <yourname>
/// @studid <studentid>
//--------------------------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_DIR 64 ///< maximum number of supported directories

/// @brief output control flags
#define F_TREE 0x1    ///< enable tree view
#define F_SUMMARY 0x2 ///< enable summary
#define F_VERBOSE 0x4 ///< turn on verbose mode

/// @brief struct holding the summary
struct summary {
  unsigned int dirs;  ///< number of directories encountered
  unsigned int files; ///< number of files
  unsigned int links; ///< number of links
  unsigned int fifos; ///< number of pipes
  unsigned int socks; ///< number of sockets

  unsigned long long size; ///< total size (in bytes)
  unsigned long long
      blocks; ///< total number of blocks (512 byte blocks)
};

/// @brief File type
typedef enum FileType {
  FT_REGULAR = 0, ///< regular file
  FT_DIRECTORY,   ///< directory
  FT_LINK,        ///< symbolic link
  FT_CHARACTER,   ///< character device
  FT_BLOCK,       ///< block device
  FT_FIFO,        ///< pipe
  FT_SOCKET,      ///< socket
} FileType;

/// @brief Struct holding the file info
typedef struct FileInfo {
  char name[256];             ///< file name
  char *path;                 ///< absolute path
  FileType type;              ///< file type
  struct FileInfo **children; ///< pointer to subdirectories
  int nchildren;              ///< number of subdirectories
} FileInfo;

/// @brief abort the program with EXIT_FAILURE and an optional error
/// message
///
/// @param msg optional error message or NULL
void panic(const char *msg) {
  if (msg)
    fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

/// @brief read next directory entry from open directory 'dir'. Ignores
/// '.' and '..' entries
///
/// @param dir open DIR* stream
/// @retval entry on success
/// @retval NULL on error or if there are no more entries
struct dirent *getNext(DIR *dir) {
  struct dirent *next;
  int ignore;

  do {
    errno = 0;
    next = readdir(dir);
    if (errno != 0)
      perror(NULL);
    ignore = next && ((strcmp(next->d_name, ".") == 0) ||
                      (strcmp(next->d_name, "..") == 0));
  } while (next && ignore);

  return next;
}

void replace_char(const char **pstr, char target, char replacement) {
  char *copy = malloc(strlen(*pstr) + 1);
  strcpy(copy, *pstr);

  for (char *p = copy; *p != '\0'; p++) {
    if (*p == target) {
      *p = replacement;
    }
  }

  *pstr = copy;
  // printf("Modified: %s\n", copy);s
}

/// @brief Calculate the prefix for the tree view
/// @param pstr Pointer to the string to be modified
/// @param flags Output control flags (F_*). If F_TREE is set, the
/// prefix is modified to be suitable for tree view
/// @param isLastEntry 1 if the entry is the last entry in the directory
void calculatePrefix(const char **pstr, unsigned int flags,
                     int isLastEntry) {
  char *prefix = malloc(strlen(*pstr) + 1);
  replace_char(pstr, '-', ' ');
  replace_char(pstr, '`', ' ');
  // printf("Prefix: %s\n", *pstr);
  strcpy(prefix, *pstr);

  if ((flags & F_TREE) == F_TREE) {
    prefix[strlen(*pstr) - 1] = '-'; // "-" for tree view
    if (strlen(*pstr) > 2) {
      // "|" only when it is not base directory
      prefix[strlen(*pstr) - 2] = '|';
    }
    if (isLastEntry) {                 // last entry
      prefix[strlen(*pstr) - 2] = '`'; // "`" for tree view
    }
  }

  // Free old pstr memory
  free((char *)*pstr);
  *pstr = prefix;
}

/// @brief Calculate the suffix for the file name
/// @param pstr Pointer to the string to be modified
/// @param pfileName Pointer to the file name to be modified
void calculateSuffix(const char **pstr, char **pfileName) {
  int len = strlen(*pstr) + strlen(*pfileName);
  if (len > 54) {
    // Calculate the available length of the file name
    int remainingLen = 54 - strlen(*pstr);

    // Create a new string with the remaining length
    char *copy = malloc(remainingLen);
    strncpy(copy, *pfileName, remainingLen);

    // Add suffic
    copy[remainingLen - 1] = '.';
    copy[remainingLen - 2] = '.';
    copy[remainingLen - 3] = '.';

    // Free old pfileName memory
    free(*pfileName);

    *pfileName = copy;
  }
}

/// @brief qsort comparator to sort directory entries. Sorted by name,
/// directories first.
///
/// @param a pointer to first entry
/// @param b pointer to second entry
/// @retval -1 if a<b
/// @retval 0  if a==b
/// @retval 1  if a>b
static int dirent_compare(const struct dirent **a,
                          const struct dirent **b) {
  // struct dirent *e1 = (struct dirent *)a;
  // struct dirent *e2 = (struct dirent *)b;
  const struct dirent *e1 = *a;
  const struct dirent *e2 = *b;

  // if one of the entries is a directory, it comes first
  if (e1->d_type != e2->d_type) {
    if (e1->d_type == DT_DIR)
      return -1;
    if (e2->d_type == DT_DIR)
      return 1;
  }

  // otherwise sorty by name
  return strcmp(e1->d_name, e2->d_name);
}

/// @brief recursively process directory @a dn and print its tree
///
/// @param dn absolute or relative path string
/// @param pstr prefix string printed in front of each entry
/// @param stats pointer to statistics
/// @param flags output control flags (F_*)
void processDir(const char *dn, const char *pstr, struct summary *stats,
                unsigned int flags) {
  // Open directory
  DIR *curDir = opendir(dn);
  if (curDir == NULL) { // open directory failed
    calculatePrefix(&pstr, flags, 1);
    printf("%sError: Permission denied\n", pstr);
    return;
  }
  // Entries to save the file in the directory
  struct dirent **entries;
  // Read and get number of entries
  int dircnt = scandir(dn, &entries, NULL, dirent_compare);
  if (dircnt < 0) { // read directory failed
    calculatePrefix(&pstr, flags, 0);
    printf("%sError: Permission denied\n", pstr);
    closedir(curDir);
    return;
  }

  // Print default state
  // Get the directory name
  // const char *last_slash = strrchr(dn, '/');
  if (strlen(pstr) <= 2) { // Only the base directory
    // printf("%s\n", last_slash + 1);
    printf("%s\n", dn);
  }

  for (int i = 0; i < dircnt; i++) {
    struct dirent *entry = entries[i];
    free(entries[i]); // Free the entry
    if (strcmp(entry->d_name, ".") == 0 ||
        strcmp(entry->d_name, "..") == 0) { // Ignore "./.."
      continue;
    } else {
      // Copy the entry name
      char *entryName = malloc(strlen(entry->d_name) + 1);
      strcpy(entryName, entry->d_name);

      calculatePrefix(&pstr, flags, i == dircnt - 1);
      calculateSuffix(&pstr, &entryName);
      // Print the entry name
      printf("%s%s\n", pstr, entryName);
      // Free the entry name
      free(entryName);

      if (entry->d_type == DT_DIR) { // If entry is a directory
        // Update subdirectory path
        char *subDirPath =
            malloc(strlen(dn) + strlen(entry->d_name) + 2);
        snprintf(subDirPath, strlen(dn) + strlen(entry->d_name) + 2,
                 "%s/%s", dn, entry->d_name);

        // Increase prefix length
        char *subDirPrefix = malloc(strlen(pstr) + 3);
        snprintf(subDirPrefix, strlen(pstr) + 3, "%s  ", pstr);

        processDir(subDirPath, subDirPrefix, stats, flags);

        free(subDirPath);
        free(subDirPrefix);
      }
    }
  }
  free(entries); // Free the entries
  closedir(curDir);
}

/// @brief print program syntax and an optional error message. Aborts
/// the program with EXIT_FAILURE
///
/// @param argv0 command line argument 0 (executable)
/// @param error optional error (format) string (printf format) or
/// NULL
/// @param ... parameter to the error format string
void syntax(const char *argv0, const char *error, ...) {
  if (error) {
    va_list ap;

    va_start(ap, error);
    vfprintf(stderr, error, ap);
    va_end(ap);

    printf("\n\n");
  }

  assert(argv0 != NULL);

  fprintf(stderr,
          "Usage %s [-t] [-s] [-v] [-h] [path...]\n"
          "Gather information about directory trees. If no path is "
          "given, the current directory\n"
          "is analyzed.\n"
          "\n"
          "Options:\n"
          " -t        print the directory tree (default if no other "
          "option specified)\n"
          " -s        print summary of directories (total number of "
          "files, total file size, etc)\n"
          " -v        print detailed information for each file. Turns "
          "on tree view.\n"
          " -h        print this help\n"
          " path...   list of space-separated paths (max %d). Default "
          "is the current directory.\n",
          basename(argv0), MAX_DIR);

  exit(EXIT_FAILURE);
}

/// @brief program entry point
int main(int argc, char *argv[]) {
  // default directory is the current directory (".")
  const char CURDIR[] = ".";
  const char *directories[MAX_DIR];
  int ndir = 0;

  struct summary tstat;
  unsigned int flags = 0;

  // parse arguments
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      // format: "-<flag>"
      if (!strcmp(argv[i], "-t"))
        flags |= F_TREE;
      else if (!strcmp(argv[i], "-s"))
        flags |= F_SUMMARY;
      else if (!strcmp(argv[i], "-v"))
        flags |= F_VERBOSE;
      else if (!strcmp(argv[i], "-h"))
        syntax(argv[0], NULL);
      else
        syntax(argv[0], "Unrecognized option '%s'.", argv[i]);
    } else {
      // anything else is recognized as a directory
      if (ndir < MAX_DIR) {
        directories[ndir++] = argv[i];
      } else {
        printf("Warning: maximum number of directories exceeded, "
               "ignoring '%s'.\n",
               argv[i]);
      }
    }
  }

  // if no directory was specified, use the current directory
  if (ndir == 0)
    directories[ndir++] = CURDIR;

  // process each directory
  //
  // TODO
  //
  // Pseudo-code
  // - reset statistics (tstat)
  // - loop over all entries in 'directories' (number of entires
  // stored in 'ndir')
  //   - reset statistics (dstat)
  //   - if F_SUMMARY flag set: print header
  //   - print directory name
  //   - call processDir() for the directory
  //   - if F_SUMMARY flag set: print summary & update statistics
  memset(&tstat, 0, sizeof(tstat));

  for (int i = 0; i < ndir; i++) {
    if (flags == 0) { // no flags
      processDir(directories[i], "  ", &tstat, flags);
    } else if (flags & F_TREE) { // -t
      processDir(directories[i], "| ", &tstat, flags);
    } else if (flags & F_VERBOSE) { // -v
      printf("Analyzing directory '%s':\n", directories[i]);
    } else if (flags & F_SUMMARY) { // -s
      printf("Analyzing directory '%s':\n", directories[i]);
    } else if ((flags & (F_TREE | F_SUMMARY)) ==
               (F_TREE | F_SUMMARY)) { // -t -s
      printf("Analyzing directory '%s':\n", directories[i]);
    } else if ((flags & (F_VERBOSE | F_TREE)) ==
               (F_VERBOSE | F_TREE)) { // -v -t
      printf("Analyzing directory '%s':\n", directories[i]);
    } else if ((flags & (F_VERBOSE | F_SUMMARY)) ==
               (F_VERBOSE | F_SUMMARY)) { // -v -s
      printf("Analyzing directory '%s':\n", directories[i]);
    } else if ((flags & (F_VERBOSE | F_TREE | F_SUMMARY)) ==
               (F_VERBOSE | F_TREE | F_SUMMARY)) { // -v -t -s
      printf("Analyzing directory '%s':\n", directories[i]);
    }
  }

  // print grand total
  if ((flags & F_SUMMARY) && (ndir > 1)) {
    printf("Analyzed %d directories:\n"
           "  total # of files:        %16d\n"
           "  total # of directories:  %16d\n"
           "  total # of links:        %16d\n"
           "  total # of pipes:        %16d\n"
           "  total # of sockets:      %16d\n",
           ndir, tstat.files, tstat.dirs, tstat.links, tstat.fifos,
           tstat.socks);

    if (flags & F_VERBOSE) {
      printf("  total file size:         %16llu\n"
             "  total # of blocks:       %16llu\n",
             tstat.size, tstat.blocks);
    }
  }
  return EXIT_SUCCESS;
}
