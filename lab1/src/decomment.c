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

void single_line_comment_loop();
void multi_line_comment_loop();
void string_loop();
void char_const_loop();
void print_cur_char(char ch);
void check_new_line();
void print_error(int line_no);

int main(void) {
  /*
  Read characters from standard input one by one and remove comments
  from the input. return EXIT_SUCCESS if the program runs successfully
  and exit with EXIT_FAILURE if the program encounters an error.
  */
  // ich: int type variable to store character input from getchar()
  // (abbreviation of int character)
  int ich;

  // ch: character that comes from casting (char) on ich (abbreviation
  // of character)
  char ch;

  // is_slash: flag to check if the prev character is a slash
  int is_slash = 0;

  // This while loop reads all characters from standard input one by one
  while (1) {

    ich = getchar();
    if (ich == EOF)
      break;

    ch = (char)ich;
    // TODO: Implement the decommenting logic
    switch (ich) {
    case '/':                           // POSSIBLE_SINGLE_LINE_COMMENT
      if (ch == '/' && is_slash == 1) { // SINGLE_LINE_COMMENT
        single_line_comment_loop();
        is_slash = 0;
      } else { // POSSIBLE_COMMENT need to check next character
        is_slash = 1;
      }
      break;

    case '*':              // POSSIBLE_MULTI_LINE_COMMENT
      if (is_slash == 1) { // MULTI_LINE_COMMENT
        multi_line_comment_loop();
      } else { // GET_CHAR or slash
        print_cur_char(ch);
      }
      is_slash = 0;
      break;
    case '\"':             // STRING
      if (is_slash == 1) { // Makeup for slash that was used to evaluate
                           // is_slash. not a special char
        print_cur_char('/');
      }
      string_loop(); // STRING_LOOP
      is_slash = 0;
      break;               // END_STRING
    case '\'':             // CHAR_CONST
      if (is_slash == 1) { // Makeup for slash that was used to evaluate
                           // is_slash. not a special char
        print_cur_char('/');
      }
      char_const_loop();
      is_slash = 0;
      break;
    case '\n':             // NEW_LINE
      if (is_slash == 1) { // Makeup for slash that was used to evaluate
                           // is_slash. not a special char
        print_cur_char('/');
      }
      check_new_line();
      print_cur_char(ch);
      is_slash = 0;
      break;
    case EOF:
      break;
    default:
      if (is_slash == 1) { // Makeup for slash that was used to evaluate
                           // is_slash. not a special char
        print_cur_char('/');
      }
      print_cur_char(ch);
      is_slash = 0;
      break;
    }
  }
  return (EXIT_SUCCESS);
}

void check_new_line() {
  /* Increament global variable line_cur by 1 */
  line_cur++;
}

void print_cur_char(char ch) {
  /* Print character to standard output */
  fprintf(stdout, "%c", ch);
}

void print_error(int line_no) {
  /* Print error message to stderr */
  fprintf(stderr, "Error: line %d: unterminated comment\n", line_no);
}

void single_line_comment_loop() {
  /* Reads characters until the end of line and print a version without
    single line comment */
  char ch;
  int ich;

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
    } else { // SINGLE_LINE_COMMENT
    }
  } while (1); // EOF is checked in the while loop

  print_cur_char('\n'); // Print new line (end of line)
}

void multi_line_comment_loop() {
  /* Reads characters until the end of comment or EOF.
  If it ends safely it prints a version without  multi line comment.
  If it ends with EOF then it prints a version without  multi line
  comment and error indicating error line in sterror. */
  char ch;
  int ich;
  int is_star = 0;
  int com_line = line_cur;

  print_cur_char(' '); // Print space for multi line comment
  do {
    ich = getchar();
    ch = (char)ich;
    if (ch == '\n') { // MULTI_LINE_COMMENT
      check_new_line();
      print_cur_char(ch);
      is_star = 0;
    } else if (ich == EOF) { // END_ERROR
      print_error(com_line);
      exit(EXIT_FAILURE);
      is_star = 0;
      break;
    } else if (ch == '*') {
      is_star = 1;
    } else if (is_star == 1 && ch == '/') { // END_MULTI_LINE_COMMENT
      break;
    } else { // MULTI_LINE_COMMENT
      is_star = 0;
    }
  } while (1); // EOF is checked in the while loop
}

void string_loop() {
  /* Reads the input and print it until the string ends
  comments are treated as string
  */
  char ch;
  int ich;
  // variable to check if the prev character is a backslash
  int is_backslash = 0;

  print_cur_char('\"'); // Print starting double quote
  do {
    ich = getchar();
    ch = (char)ich;

    if (ch == '\"' && is_backslash != 1) { // END_STRING
      print_cur_char(ch);
      break;
    } else if (ch == '\n') { // END_STRING
      print_cur_char(ch);
      is_backslash = 0;
      check_new_line();
    } else if (ich == EOF) { // END_STRING
      break;
    } else if (ch == '\\' && is_backslash != 1) {
      // Check backslash and also two backslashes
      // if two backslashes then ignore since it is a char and not
      // special char
      print_cur_char(ch);
      is_backslash = 1;
    } else { // STRING
      print_cur_char(ch);
      is_backslash = 0;
    }
  } while (1);
}

void char_const_loop() {
  /* Reads the input and print it until the string ends
  comments are treated as string
  */
  char ch;
  int ich;
  // variable to check if the prev character is a backslash
  int is_backslash = 0;

  print_cur_char('\''); // Print starting single quote
  do {
    ich = getchar();
    ch = (char)ich;

    if (ch == '\'' && is_backslash != 1) { // END_CHAR_CONST
      print_cur_char(ch);
      break;
    } else if (ch == '\n') { // END_CHAR_CONST
      print_cur_char(ch);
      is_backslash = 0;
      check_new_line();
    } else if (ich == EOF) { // END_CHAR_CONST
      break;
    } else if (ch == '\\' && is_backslash != 1) {
      // Check backslash and also two backslashes
      // if two backslashes then ignore since it is a char and not
      // special char
      print_cur_char(ch);
      is_backslash = 1;
    } else { // CHAR_CONST
      print_cur_char(ch);
      is_backslash = 0;
    }
  } while (1);
}