#include <config.h>
#include "parser.h"

#include <assert.h>
#include <ctype.h>

#define SPACE_REPLACEMENT '~'
#define ESCAPE_CHAR       '\\'
#define MAXTOKENSIZE      8192

typedef struct parser_state {
  const unsigned char *current_token;
  char * current_cmd;
  struct parser_state * next;
} parser_state;

static parser_state * state;

void
init_tokens_str(const char * initstr, char * cmd)
{
  if (state==NULL) {
    state = malloc(sizeof(parser_state));
  }
  if (state->current_cmd) free(state->current_cmd);
  state->current_cmd = cmd;
  state->current_token = (const unsigned char *)initstr;
}

void
parser_pushstate(void)
{
  parser_state * new_state = malloc(sizeof(parser_state));
  new_state->current_cmd = NULL;
  new_state->current_token = NULL;
  new_state->next = state;
  state = new_state;
}

void
parser_popstate(void)
{
  parser_state * new_state = state->next;
  if (state->current_cmd!=NULL) free(state->current_cmd);
  free(state);
  state = new_state;
}

boolean
parser_end(void)
{
  while (isspace(*state->current_token)) ++state->current_token;
  return *state->current_token == 0;
}

void
skip_token(void)
{
  char quotechar = 0;

  while (isspace(*state->current_token)) ++state->current_token;
  while (*state->current_token) {
    if (isspace(*state->current_token) && quotechar==0) {
      return;
    } else {
      switch(*state->current_token) {
        case '"':
        case '\'':
          if (*state->current_token==quotechar) return;
          quotechar = *state->current_token;
          break;
        case ESCAPE_CHAR:
          ++state->current_token;
          break;
      }
    }
    ++state->current_token;
  }
}

const char *
parse_token(const char ** str)
{
  static char lbuf[MAXTOKENSIZE];
  char * cursor = lbuf;
  char quotechar = 0;
  boolean escape = false;
  const unsigned char * ctoken = (const unsigned char*)*str;

  assert(ctoken);

  while (isspace(*ctoken)) ++ctoken;
  while (*ctoken && cursor-lbuf < MAXTOKENSIZE-1) {
    if (escape) {
      *cursor++ = *ctoken++;
    } else if (isspace(*ctoken)) {
      if (quotechar==0) break;
      *cursor++ = *ctoken++;
    } else if (*ctoken=='"' || *ctoken=='\'') {
      if (*ctoken==quotechar) {
        ++ctoken;
        break;
      } else if (quotechar==0) {
        quotechar = *ctoken;
        ++ctoken;
      } else {
        *cursor++ = *ctoken++;
      }
    } else if (*ctoken==SPACE_REPLACEMENT) {
      *cursor++ = ' ';
      ++ctoken;
    } else if (*ctoken==ESCAPE_CHAR) {
      escape = true;
      ++ctoken;
    } else {
      *cursor++ = *ctoken++;
    }
  }

  *cursor = '\0';
  *str = (const char *)ctoken;
  return lbuf;
}

const char *
getstrtoken(void)
{
  return parse_token((const char**)&state->current_token);
}
