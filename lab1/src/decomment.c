// RyuMyungHyun
// assignmnet1
// decomment.c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/* This is skeleton code for reading characters from
standard input (e.g., a file or console input) one by one until
the end of the file (EOF) is reached. It keeps track of the current
line number and is designed to be extended with additional
functionality, such as processing or transforming the input data.
In this specific task, the goal is to implement logic that removes
C-style comments from the input. */

// global variable for current line number
int line_cur = 1;
const int got_eof = 0;

// array to store the characters of the current line
#define INITIAL_ARRAY_SIZE 1024

void single_line_comment_loop();
void multi_line_comment_loop();
void string_loop();
void char_const_loop();
void print_cur_char(char ch);
void check_new_line();
void print_error(int line_no);

int main(void) {
  // ich: int type variable to store character input from getchar()
  // (abbreviation of int character)
  int ich;
  // line_com: current comment line number
  // (abbreviation of current line and comment line)
  // int line_com;
  // ch: character that comes from casting (char) on ich (abbreviation
  // of character)
  char ch;

  // line_com = -1;

  // This while loop reads all characters from standard input one by one
  while (1) {

    ich = getchar();
    if (ich == EOF)
      break;

    ch = (char)ich;
    // TODO: Implement the decommenting logic
    switch (ich) {
    case '/':                              // POSSIBLE_COMMENT
      if ((ch = (char)getchar()) == '/') { // SINGLE_LINE_COMMENT
        single_line_comment_loop();
      } else if (ch == '*') { // MULTI_LINE_COMMENT
        multi_line_comment_loop();
      } else {
        print_cur_char('/');
        print_cur_char(ch);
      }
      break;
    case '\"':       // STRING
      string_loop(); // STRING_LOOP
      break;         // END_STRING
    case '\'':       // CHAR_CONST
      char_const_loop();
      break;
    case '\n': // NEW_LINE
      check_new_line();
      print_cur_char(ch);
      break;
    case EOF:
      break;
    default:
      // fprintf(stdout, "%c", ch);
      print_cur_char(ch);
      break;
    }
  }

  return (EXIT_SUCCESS);
}

void check_new_line() { line_cur++; }

void print_cur_char(char ch) {
  // Print character to standard output
  fprintf(stdout, "%c", ch);
}

void print_error(int line_no) {
  // Print error message
  fprintf(stderr, "Error: line %d: unterminated comment", line_no);
}

void single_line_comment_loop() {
  char ch;
  int ich;
  int cnt = 0;

  // Allocate array
  int size = INITIAL_ARRAY_SIZE;
  char *char_array = malloc(size);
  if (!char_array) {
    perror("Memory allocation failed");
    free(char_array);
  }

  print_cur_char(' '); // Print space for single line comment
  do {
    ich = getchar();
    ch = (char)ich;
    if (ch == '\n') { // END_SINGLE_LINE_COMMENT
      check_new_line();
      break;
    } else if (ich == EOF) { // END_ERROR(probably not required in spec)
      print_error(line_cur);
      exit(EXIT_FAILURE);
      break;
    } else {             // SINGLE_LINE_COMMENT
      if (cnt == size) { // Increase array size
        size *= 2;
        char_array = realloc(char_array, size);
        if (!char_array) {
          perror("Memory allocation failed");
          free(char_array);
        }
      }
      char_array[cnt++] = ch;
    }
  } while (1); // EOF is checked in the while loop

  print_cur_char('\n'); // Print new line (end of line)
}

void multi_line_comment_loop() {
  char ch;
  int ich;
  int cnt = 0;
  int is_star = 0;

  // Allocate array
  int size = INITIAL_ARRAY_SIZE;
  char *char_array = malloc(size);
  if (!char_array) {
    perror("Memory allocation failed");
    free(char_array);
  }

  print_cur_char(' '); // Print space for multi line comment
  do {
    ich = getchar();
    ch = (char)ich;
    if (ch == '\n') { // MULTI_LINE_COMMENT
      check_new_line();
      print_cur_char(ch);
      is_star = 0;
    } else if (ich == EOF) { // END_ERROR
      print_error(line_cur);
      print_cur_char('\n'); // Print EOF
      exit(EXIT_FAILURE);
      is_star = 0;
      break;
    } else if (ch == '*') {
      is_star = 1;
    } else if (is_star == 1 && ch == '/') { // END_MULTI_LINE_COMMENT
      break;
    } else {             // MULTI_LINE_COMMENT
      if (cnt == size) { // Increase array size
        size *= 2;
        char_array = realloc(char_array, size);
        if (!char_array) {
          perror("Memory allocation failed");
          free(char_array);
        }
      }
      char_array[cnt++] = ch;
      is_star = 0;
    }
  } while (1); // EOF is checked in the while loop

  // print_cur_char('\n'); // Print new line (end of line)
}

void string_loop() {
  char ch;
  int ich;
  print_cur_char('\"'); // Print starting double quote
  do {
    ich = getchar();
    ch = (char)ich;

    if (ch == '\"') { // END_STRING
      print_cur_char(ch);
      break;
    } else if (ch == '\n') { // END_STRING
      print_cur_char(ch);
      line_cur++;
    } else if (ich == EOF) { // END_STRING
      // print_cur_char('\n');  // Print EOF
      break;
    } else { // STRING
      print_cur_char(ch);
    }
  } while (1);
}

void char_const_loop() {
  char ch;
  int ich;

  print_cur_char('\''); // Print starting single quote
  do {
    ich = getchar();
    ch = (char)ich;

    if (ch == '\'') { // END_CHAR_CONST
      print_cur_char(ch);
      break;
    } else if (ch == '\n') { // END_CHAR_CONST
      print_cur_char(ch);
      line_cur++;
    } else if (ich == EOF) { // END_CHAR_CONST
      // print_cur_char('\n');
      break;
    } else { // CHAR_CONST
      print_cur_char(ch);
    }
  } while (ich != EOF);
}