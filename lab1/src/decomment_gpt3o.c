

#include <stdio.h>
#include <stdlib.h>

/* Define the various states for our DFA */
typedef enum {
  STATE_CODE,             /* Normal code */
  STATE_POSSIBLE_COMMENT, /* After seeing '/' in code */
  STATE_COMMENT_SINGLE,   /* Inside a // comment */
  STATE_COMMENT_MULTI,
  STATE_COMMENT_MULTI_STAR, /* Inside a multi-line comment after reading
                               '*' */
  STATE_STRING,             /* Inside a double-quoted string */
  STATE_STRING_ESCAPE,      /* After reading '\' in a string */
  STATE_CHAR,       /* Inside a single-quoted character constant */
  STATE_CHAR_ESCAPE /* After reading '\' in a character constant */
} State;

int main(void) {
  int ch;       /* current character from input */
  int line = 1; /* current line number */
  int comment_start_line =
      0; /* line number where a multi-line comment began */
  State state = STATE_CODE; /* start in normal code state */

  while ((ch = getchar()) != EOF) {
    switch (state) {
    case STATE_CODE:
      if (ch == '/') {
        /* A '/' might be the start of a comment. */
        state = STATE_POSSIBLE_COMMENT;
      } else if (ch == '"') {
        /* Enter string literal state. */
        putchar(ch);
        state = STATE_STRING;
      } else if (ch == '\'') {
        /* Enter character constant state. */
        putchar(ch);
        state = STATE_CHAR;
      } else {
        putchar(ch);
        if (ch == '\n')
          line++;
      }
      break;

    case STATE_POSSIBLE_COMMENT:
      if (ch == '*') {
        /* Begin a multi-line comment. */
        comment_start_line = line;
        /* Replace entire comment with a space. */
        putchar(' ');
        state = STATE_COMMENT_MULTI;
      } else if (ch == '/') {
        /* Begin a single-line comment. */
        putchar(' ');
        state = STATE_COMMENT_SINGLE;
      } else {
        /* It was not a comment start; output the '/' we saw earlier,
           then process the current character normally. */
        putchar('/');
        if (ch == '"') {
          putchar(ch);
          state = STATE_STRING;
        } else if (ch == '\'') {
          putchar(ch);
          state = STATE_CHAR;
        } else {
          putchar(ch);
          if (ch == '\n')
            line++;
          state = STATE_CODE;
        }
      }
      break;

    case STATE_COMMENT_SINGLE:
      /* Skip all characters until the end of the line */
      if (ch == '\n') {
        putchar('\n');
        line++;
        state = STATE_CODE;
      }
      /* Otherwise, ignore characters in the single-line comment */
      break;

    case STATE_COMMENT_MULTI:
      /* Inside a multi-line comment */
      if (ch == '*') {
        state = STATE_COMMENT_MULTI_STAR;
      } else if (ch == '\n') {
        putchar('\n');
        line++;
      }
      /* All other characters are skipped */
      break;

    case STATE_COMMENT_MULTI_STAR:
      if (ch == '/') {
        /* End of multi-line comment */
        state = STATE_CODE;
      } else if (ch == '*') {
        /* Stay in the '*' state if more '*' are seen. */
        /* Remain in STATE_COMMENT_MULTI_STAR. */
      } else {
        if (ch == '\n') {
          putchar('\n');
          line++;
        }
        /* Any other character returns us to the comment state. */
        state = STATE_COMMENT_MULTI;
      }
      break;

    case STATE_STRING:
      /* Inside a string literal: output characters verbatim */
      putchar(ch);
      if (ch == '\\') {
        state = STATE_STRING_ESCAPE;
      } else if (ch == '"') {
        state = STATE_CODE;
      } else if (ch == '\n') {
        line++;
      }
      break;

    case STATE_STRING_ESCAPE:
      /* Output the character following the backslash and return to
       * string */
      putchar(ch);
      state = STATE_STRING;
      if (ch == '\n')
        line++;
      break;

    case STATE_CHAR:
      /* Inside a character constant: output characters verbatim */
      putchar(ch);
      if (ch == '\\') {
        state = STATE_CHAR_ESCAPE;
      } else if (ch == '\'') {
        state = STATE_CODE;
      } else if (ch == '\n') {
        line++;
      }
      break;

    case STATE_CHAR_ESCAPE:
      /* Output the character following the backslash and return to char
       * constant */
      putchar(ch);
      state = STATE_CHAR;
      if (ch == '\n')
        line++;
      break;
    }
  }

  /* If we ended after reading a lone '/' then output it */
  if (state == STATE_POSSIBLE_COMMENT) {
    putchar('/');
    state = STATE_CODE;
  }

  /* If we ended while inside a multi-line comment, print an error */
  if (state == STATE_COMMENT_MULTI ||
      state == STATE_COMMENT_MULTI_STAR) {
    fprintf(stderr, "Error: line %d: unterminated comment\n",
            comment_start_line);
    return EXIT_FAILURE;
  }

  /* For unterminated strings or character constants, the assignment
     spec requires no error message. */
  return EXIT_SUCCESS;
}
