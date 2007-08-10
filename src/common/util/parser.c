#include <config.h>
#include "parser.h"
#include "unicode.h"
#include "log.h"

#include <assert.h>
#include <wctype.h>
#include <memory.h>

#define SPACE_REPLACEMENT '~'
#define ESCAPE_CHAR       '\\'
#define MAXTOKENSIZE      8192

typedef struct parser_state {
  const char *current_token;
  char * current_cmd;
  struct parser_state * next;
} parser_state;

static parser_state * state;

static int
eatwhitespace_c(const char ** str)
{
  int ret;
  wint_t ucs;
  size_t len;

  /* skip over potential whitespace */
  for (;;) {
    unsigned char utf8_character = (*(unsigned char**)str)[0];
    if (~utf8_character & 0x80) {
      if (!iswspace(utf8_character)) break;
      ++*str;
    } else {
      ret = unicode_utf8_to_ucs4(&ucs, *str, &len);
      if (ret!=0) {
        log_warning(("illegal character sequence in UTF8 string: %s\n", *str));
        return ret;
      }
      if (!iswspace(ucs)) break;
      *str+=len;
    }
  }
  return 0;
}

void
init_tokens_str(const char * initstr, char * cmd)
{
  if (state==NULL) {
    state = malloc(sizeof(parser_state));
  }
  else if (state->current_cmd) free(state->current_cmd);
  state->current_cmd = cmd;
  state->current_token = initstr;
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
  eatwhitespace_c(&state->current_token);
  return *state->current_token == 0;
}

void
skip_token(void)
{
  char quotechar = 0;
  eatwhitespace_c(&state->current_token);

  while (*state->current_token) {
    wint_t ucs;
    size_t len;

    unsigned char utf8_character = (unsigned char)state->current_token[0];
    if (~utf8_character & 0x80) {
      ucs = utf8_character;
      ++state->current_token;
    } else {
      int ret = unicode_utf8_to_ucs4(&ucs, state->current_token, &len);
      if (ret==0) {
        state->current_token+=len;
      } else {
        log_warning(("illegal character sequence in UTF8 string: %s\n", state->current_token));
      }
    }
    if (iswspace(ucs) && quotechar==0) {
      return;
    } else {
      switch(utf8_character) {
        case '"':
        case '\'':
          if (utf8_character==quotechar) return;
          quotechar = utf8_character;
          break;
        case ESCAPE_CHAR:
          ++state->current_token;
          break;
      }
    }
  }
}

const char *
parse_token(const char ** str)
{
  static char lbuf[MAXTOKENSIZE];
  char * cursor = lbuf;
  char quotechar = 0;
  boolean escape = false;
  const char * ctoken = *str;

  assert(ctoken);

  eatwhitespace_c(&ctoken);
  while (*ctoken && cursor-lbuf < MAXTOKENSIZE-1) {
    wint_t ucs;
    size_t len;
    boolean copy = false;

    unsigned char utf8_character = *(unsigned char *)ctoken;
    if (~utf8_character & 0x80) {
      ucs = utf8_character;
      len = 1;
    } else {
      int ret = unicode_utf8_to_ucs4(&ucs, ctoken, &len);
      if (ret!=0) {
        log_warning(("illegal character sequence in UTF8 string: %s\n", ctoken));
        break;
      }
    }
    if (escape) {
      copy = true;
    } else if (iswspace(ucs)) {
      if (quotechar==0) break;
      copy = true;
    } else if (utf8_character=='"' || utf8_character=='\'') {
      if (utf8_character==quotechar) {
        ++ctoken;
        break;
      } else if (quotechar==0) {
        quotechar = utf8_character;
        ++ctoken;
      } else {
        *cursor++ = *ctoken++;
      }
    } else if (utf8_character==SPACE_REPLACEMENT) {
      *cursor++ = ' ';
      ++ctoken;
    } else if (utf8_character==ESCAPE_CHAR) {
      escape = true;
      ++ctoken;
    } else {
      copy = true;
    }
    if (copy) {
      memcpy(cursor, ctoken, len);
      cursor+=len;
      ctoken+=len;
    }
  }

  *cursor = '\0';
  *str = ctoken;
  return lbuf;
}

const char *
getstrtoken(void)
{
  return parse_token((const char **)&state->current_token);
}
