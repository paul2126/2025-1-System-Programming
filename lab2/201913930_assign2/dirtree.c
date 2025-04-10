//--------------------------------------------------------------------------------------------------
// System Programming                         I/O Lab Spring 2025
//
/// @file
/// @brief resursively traverse directory tree and list all entries
/// @author Ryu MyungHyun
/// @studid 2019-13930
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
  unsigned int files; ///< number of files
  unsigned int dirs;  ///< number of directories encountered
  unsigned int links; ///< number of links
  unsigned int fifos; ///< number of pipes
  unsigned int socks; ///< number of sockets

  unsigned long long size; ///< total size (in bytes)
  unsigned long long
      blocks; ///< total number of blocks (512 byte blocks)
};

/// @brief abort the program with EXIT_FAILURE and an optional error
/// message
/// @param msg optional error message or NULL
void panic(const char *msg) {
  if (msg)
    fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

/// @brief read next directory entry from open directory 'dir'. Ignores
/// '.' and '..' entries
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

/// @brief Calculate the prefix for the tree view
/// @param inputStr Pointer to the string being referenced
/// @param flags Get flag and format prefix accordingly
/// @param isLastEntry 1 if the entry is the last entry in the directory
/// @return Pointer to the prefix string.
char *calculatePrefix(const char *inputStr, unsigned int flags,
                      int isLastEntry) {
  // Make a modifiable copy of inputStr
  char *temp = strdup(inputStr);

  // Replace - ` into ' '
  for (char *p = temp; *p != '\0'; p++) {
    if (*p == '-' || *p == '`') {
      *p = ' ';
    }
  }

  // Create prefix based on modified string
  char *prefix = malloc(strlen(temp) + 1);
  strcpy(prefix, temp);
  free(temp);

  if ((flags & F_TREE) == F_TREE) {
    size_t len = strlen(prefix);
    prefix[len - 1] = '-'; // "-" for tree view
    if (len > 2) {
      // "|" only when it is not base directory
      prefix[len - 2] = '|';
    }
    if (isLastEntry) {       // Last entry
      prefix[len - 2] = '`'; // "`" for tree view
    }
  }

  return prefix;
}

/// @brief Calculate the suffix for the file name
/// @param pstr Pointer to the string to be modified
/// @param pfileName Pointer to the file name to be modified
void calculateSuffix(char *pstr, char **pfileName, int maxLen) {
  int len = strlen(pstr) + strlen(*pfileName);
  if (len > maxLen) {
    // Calculate the available length of the file name
    int remainingLen = maxLen - strlen(pstr);

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
/// @brief Align the string to the right with spaces
/// @param src Source string to be aligned
/// @param width Total width of the aligned string
/// @return Pointer to the aligned string
char *alignRight(const char *src, int width) {
  char *buf = malloc(width + 1);

  // Length of the source string
  size_t len = strlen(src);
  int padding = width - len;
  // Do nothing on neg padding
  if (padding < 0)
    padding = 0;

  // Fill with spaces
  memset(buf, ' ', padding);
  // Fill after padding
  strncpy(buf + padding, src, width - padding);
  buf[width] = '\0';
  return buf;
}

/// @brief Align the string to the left with spaces
/// @param src Source string to be aligned
/// @param width Total width of the aligned string
/// @return Pointer to the aligned string
char *alignLeft(const char *src, int width) {
  char *buf = malloc(width + 1);

  size_t len = strlen(src);
  strncpy(buf, src, width);
  // If the string is shorter than the width
  if (len < width) {
    // Fill remainder with spaces
    memset(buf + len, ' ', width - len);
  }
  buf[width] = '\0';
  return buf;
}

/// @brief Get file information for a given path and update summary
/// @param path Absolute or relative path string
/// @param st Pointer to a stat structure to store file information
/// @param counts Pointer to a summary structure to store file
/// information for printing
/// @return Pointer to the stat structure on success, NULL on error
struct stat *calculateFileInfo(const char *path, struct stat *st,
                               struct summary *counts) {
  // Get file information using lstat. dont follow links
  if (lstat(path, st) == -1) {
    fprintf(stderr, "ERROR: %s", strerror(errno));
    return NULL;
  }
  if (S_ISREG(st->st_mode))
    counts->files++;
  else if (S_ISDIR(st->st_mode))
    counts->dirs++;
  else if (S_ISLNK(st->st_mode))
    counts->links++;
  else if (S_ISFIFO(st->st_mode))
    counts->fifos++;
  else if (S_ISSOCK(st->st_mode))
    counts->socks++;
  else if (S_ISBLK(st->st_mode))
    counts->blocks++;

  // Update total size and blocks
  counts->size += st->st_size;
  counts->blocks += st->st_blocks;

  return st;
}
/// @brief Calculate for -v option
/// @param st Stat structure containing file information
/// @return Formatted string (user, group, size, blocks, type)
char *calculateVerbose(struct stat *st) {
  char *verboseResult = malloc(46);
  struct passwd *pw = getpwuid(st->st_uid);
  struct group *gr = getgrgid(st->st_gid);
  char *userName = NULL;
  char *groupName = NULL;
  char *fileSize = NULL;
  char *fileBlocks = NULL;

  // Format strings. Null terminated
  char *fileType = " ";
  if (S_ISREG(st->st_mode)) {
    fileType = " ";
  } else if (S_ISDIR(st->st_mode)) {
    fileType = "d";
  } else if (S_ISLNK(st->st_mode)) {
    fileType = "l";
  } else if (S_ISCHR(st->st_mode)) {
    fileType = "c";
  } else if (S_ISBLK(st->st_mode)) {
    fileType = "b";
  } else if (S_ISFIFO(st->st_mode)) {
    fileType = "f";
  } else if (S_ISSOCK(st->st_mode)) {
    fileType = "s";
  }
  // Convert size and blocks to strings
  char sizeStr[32];
  snprintf(sizeStr, sizeof(sizeStr), "%lld", (long long)st->st_size);

  char blocksStr[32];
  snprintf(blocksStr, sizeof(blocksStr), "%lld",
           (long long)st->st_blocks);

  userName = alignRight(pw->pw_name, 8);
  groupName = alignLeft(gr->gr_name, 8);
  fileSize = alignRight(sizeStr, 10);
  fileBlocks = alignRight(blocksStr, 8);

  snprintf(verboseResult, 46, "  %s:%s  %s  %s  %s", userName,
           groupName, fileSize, fileBlocks, fileType);
  return verboseResult;
}

/// @brief recursively process directory @a dn and print its tree
///
/// @param dn absolute or relative path string
/// @param pstr prefix string printed in front of each entry
/// @param stats pointer to statistics
/// @param flags output control flags (F_*)
void processDir(const char *dn, const char *pstr, struct summary *stats,
                unsigned int flags) {
  char *pstrCopy = NULL;
  // Print default state
  // Get the directory name
  if (strlen(pstr) <= 2) { // Only the base directory
    printf("%s\n", dn);
  }

  // Open directory
  DIR *curDir = opendir(dn);
  if (curDir == NULL) { // Open directory failed
    pstrCopy = calculatePrefix(pstr, flags, 1);

    fprintf(stderr, "%sERROR: %s\n", pstrCopy, strerror(errno));
    free((char *)pstrCopy);
    return;
  }
  // Entries to save the file in the directory
  struct dirent **entries;
  // Read and get number of entries
  int dircnt = scandir(dn, &entries, NULL, dirent_compare);
  if (dircnt < 0) { // Read directory failed
    pstrCopy = calculatePrefix(pstr, flags, 0);

    fprintf(stderr, "%sERROR: %s\n", pstrCopy, strerror(errno));
    free((char *)pstrCopy);
    closedir(curDir);
    return;
  }

  for (int i = 0; i < dircnt; i++) {
    struct dirent *entry = entries[i];
    if (strcmp(entry->d_name, ".") == 0 ||
        strcmp(entry->d_name, "..") == 0) { // Ignore "./.."
      free(entries[i]);
      continue;
    } else {
      // Copy the entry name
      char *entryName = malloc(strlen(entry->d_name) + 1);
      strcpy(entryName, entry->d_name);

      pstrCopy = calculatePrefix(pstr, flags, i == dircnt - 1);
      calculateSuffix(pstrCopy, &entryName, 54);

      // Calculate the file info
      struct stat st;
      // Entry path
      char *entryPath = malloc(strlen(dn) + strlen(entry->d_name) + 2);
      snprintf(entryPath, strlen(dn) + strlen(entry->d_name) + 2,
               "%s/%s", dn, entry->d_name);

      if (calculateFileInfo(entryPath, &st, stats) == NULL) {
        free(entryPath);
        free(entryName);
        fprintf(stderr, "%sERROR: %s\n", pstrCopy, strerror(errno));
        free((char *)pstrCopy);
        continue;
      }
      free(entryPath);

      if ((flags & F_VERBOSE) == F_VERBOSE) { // -v
        // Combine pstr and entryName
        char *pathNameResult =
            malloc(strlen(pstrCopy) + strlen(entryName) + 1);
        snprintf(pathNameResult,
                 strlen(pstrCopy) + strlen(entryName) + 1, "%s%s",
                 pstrCopy, entryName);
        // Create verbose result
        char *verboseResult = calculateVerbose(&st);
        printf("%-54.54s", pathNameResult);
        printf("%s\n", verboseResult);
      } else { // Default
        // Print the entry name
        printf("%s%s\n", pstrCopy, entryName);
      }

      // Free the entry name
      free(entryName);

      if (entry->d_type == DT_DIR) { // If entry is a directory
        // Update subdirectory path
        char *subDirPath =
            malloc(strlen(dn) + strlen(entry->d_name) + 2);
        snprintf(subDirPath, strlen(dn) + strlen(entry->d_name) + 2,
                 "%s/%s", dn, entry->d_name);

        // Increase prefix length
        char *subDirPrefix = malloc(strlen(pstrCopy) + 3);
        snprintf(subDirPrefix, strlen(pstrCopy) + 3, "%s  ", pstrCopy);

        processDir(subDirPath, subDirPrefix, stats, flags);

        free(subDirPath);
        free(subDirPrefix);
      }
      free((char *)pstrCopy);
    }
    // Free the entry
    free(entries[i]);
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

/// @brief Format according to grammar and save to output
/// @param output
/// @param files
/// @param dirs
/// @param links
/// @param pipes
/// @param sockets
/// @param size
void format_counts(char *output, int files, int dirs, int links,
                   int pipes, int sockets, size_t size) {
  snprintf(output, size, "%d %s, %d %s, %d %s, %d %s, and %d %s", files,
           files == 1 ? "file" : "files", dirs,
           dirs == 1 ? "directory" : "directories", links,
           links == 1 ? "link" : "links", pipes,
           pipes == 1 ? "pipe" : "pipes", sockets,
           sockets == 1 ? "socket" : "sockets");
}
/// @brief Calculate -s option
/// @param flags
/// @param tstat
/// @param directories
/// @param i
void calculateSummary(unsigned int flags, struct summary *tstat,
                      const char *directories, int i) {
  // Header
  if ((flags & F_VERBOSE) == F_VERBOSE) { // -v
    printf("Name                                                     "
           "   User:Group           Size    Blocks Type\n");
    printf("---------------------------------------------------------"
           "-------------------------------------------\n");
  } else {
    printf("Name\n");
    printf("---------------------------------------------------------"
           "-------------------------------------------\n");
  }
  // Body
  if ((flags & F_TREE) == F_TREE) { // -t
    processDir(directories, "| ", tstat, flags);
  } else {
    processDir(directories, "  ", tstat, flags);
  }

  // Footer
  printf("---------------------------------------------------------"
         "-------------------------------------------\n");
  // Calculate summary
  char *summaryOutput = malloc(100);
  char *empty = "";
  format_counts(summaryOutput, tstat->files, tstat->dirs, tstat->links,
                tstat->fifos, tstat->socks, 100);
  calculateSuffix(empty, &summaryOutput, 68);
  if ((flags & (F_SUMMARY | F_VERBOSE)) == (F_SUMMARY | F_VERBOSE)) {
    printf("%-68.68s", summaryOutput);
  } else {
    printf("%-68.68s\n\n", summaryOutput);
  }

  if ((flags & F_VERBOSE) == F_VERBOSE) { // -v
    // Convert total size and total blocks to strings
    char totalSizeStr[32];
    snprintf(totalSizeStr, sizeof(totalSizeStr), "%lld",
             (long long)tstat->size);

    char totalBlocksStr[32];
    snprintf(totalBlocksStr, sizeof(totalBlocksStr), "%lld",
             (long long)tstat->blocks);
    char *totalSize = alignRight(totalSizeStr, 14);
    char *totalBlocks = alignRight(totalBlocksStr, 9);
    printf("   %s %s\n\n", totalSize, totalBlocks);
  }
}

/// @brief program entry point
int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0); // Force stdout to be unbuffered
  // default directory is the current directory (".")
  const char CURDIR[] = ".";
  const char *directories[MAX_DIR];
  int ndir = 0;
  unsigned int files = 0, dirs = 0, links = 0, fifos = 0, socks = 0;
  unsigned long long size = 0, blocks = 0;

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

  memset(&tstat, 0, sizeof(tstat));

  for (int i = 0; i < ndir; i++) {
    if (flags == 0) { // no flags
      processDir(directories[i], "  ", &tstat, flags);
    } else if (flags == F_TREE) { // -t
      processDir(directories[i], "| ", &tstat, flags);
    } else if (flags == F_VERBOSE) { // -v
      processDir(directories[i], "  ", &tstat, flags);
    } else if (flags == F_SUMMARY) { // -s
      calculateSummary(flags, &tstat, directories[i], i);
    } else if ((flags & (F_VERBOSE | F_TREE | F_SUMMARY)) ==
               (F_VERBOSE | F_TREE | F_SUMMARY)) { // -v -t -s
      calculateSummary(flags, &tstat, directories[i], i);
    } else if ((flags & (F_TREE | F_SUMMARY)) ==
               (F_TREE | F_SUMMARY)) { // -t -s
      calculateSummary(flags, &tstat, directories[i], i);
    } else if ((flags & (F_VERBOSE | F_TREE)) ==
               (F_VERBOSE | F_TREE)) { // -v -t
      processDir(directories[i], "| ", &tstat, flags);
    } else if ((flags & (F_VERBOSE | F_SUMMARY)) ==
               (F_VERBOSE | F_SUMMARY)) { // -v -s
      calculateSummary(flags, &tstat, directories[i], i);
    }
    // Save to local variable and reset statistics.
    files += tstat.files;
    dirs += tstat.dirs;
    links += tstat.links;
    fifos += tstat.fifos;
    socks += tstat.socks;
    size += tstat.size;
    blocks += tstat.blocks;
    tstat.files = 0;
    tstat.dirs = 0;
    tstat.links = 0;
    tstat.fifos = 0;
    tstat.socks = 0;
    tstat.size = 0;
    tstat.blocks = 0;
  }

  // print grand total
  if ((flags & F_SUMMARY) && (ndir > 1)) {
    printf("Analyzed %d directories:\n"
           "  total # of files:        %16d\n"
           "  total # of directories:  %16d\n"
           "  total # of links:        %16d\n"
           "  total # of pipes:        %16d\n"
           "  total # of sockets:      %16d\n",
           ndir, files, dirs, links, fifos, socks);

    if (flags & F_VERBOSE) {
      printf("  total file size:         %16llu\n"
             "  total # of blocks:       %16llu\n",
             size, blocks);
    }
  }
  return EXIT_SUCCESS;
}
