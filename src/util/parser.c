#include <platform.h>
#include "parser.h"
#include "unicode.h"
#include "base36.h"
#include "log.h"

#include <assert.h>
#include <stdlib.h>
#include <wctype.h>
#include <memory.h>

#define SPACE_REPLACEMENT '~'
#define ESCAPE_CHAR       '\\'
#define MAXTOKENSIZE      8192

typedef struct parser_state {
  const char *current_token;
  char *current_cmd;
  struct parser_state *next;
} parser_state;

static parser_state *states;

static int eatwhitespace_c(const char **str_p)
{
  int ret = 0;
  ucs4_t ucs;
  size_t len;
  const char *str = *str_p;

  /* skip over potential whitespace */
  for (;;) {
    unsigned char utf8_character = (unsigned char)*str;
    if (~utf8_character & 0x80) {
      if (!iswxspace(utf8_character))
        break;
      ++str;
    } else {
      ret = unicode_utf8_to_ucs4(&ucs, str, &len);
      if (ret != 0) {
        log_warning("illegal character sequence in UTF8 string: %s\n", str);
        break;
      }
      if (!iswxspace((wint_t) ucs))
        break;
      str += len;
    }
  }
  *str_p = str;
  return ret;
}

void init_tokens_str(const char *initstr, char *cmd)
{
  if (states == NULL) {
    states = malloc(sizeof(parser_state));
  }
  else if (states->current_cmd && states->current_cmd!=cmd) {
    free(states->current_cmd);
  }
  states->current_cmd = cmd;
  states->current_token = initstr;
}

void parser_pushstate(void)
{
  parser_state *new_state = malloc(sizeof(parser_state));
  new_state->current_cmd = NULL;
  new_state->current_token = NULL;
  new_state->next = states;
  states = new_state;
}

void parser_popstate(void)
{
  parser_state *new_state = states->next;
  if (states->current_cmd != NULL)
    free(states->current_cmd);
  free(states);
  states = new_state;
}

bool parser_end(void)
{
    if (states->current_token) {
        eatwhitespace_c(&states->current_token);
        return *states->current_token == 0;
    }
    return true;
}

void skip_token(void)
{
  char quotechar = 0;
  eatwhitespace_c(&states->current_token);

  while (*states->current_token) {
    ucs4_t ucs;
    size_t len;

    unsigned char utf8_character = (unsigned char)states->current_token[0];
    if (~utf8_character & 0x80) {
      ucs = utf8_character;
      ++states->current_token;
    } else {
      int ret = unicode_utf8_to_ucs4(&ucs, states->current_token, &len);
      if (ret == 0) {
        states->current_token += len;
      } else {
        log_warning("illegal character sequence in UTF8 string: %s\n", states->current_token);
      }
    }
    if (iswxspace((wint_t) ucs) && quotechar == 0) {
      return;
    } else {
      switch (utf8_character) {
        case '"':
        case '\'':
          if (utf8_character == quotechar)
            return;
          quotechar = utf8_character;
          break;
        case ESCAPE_CHAR:
          ++states->current_token;
          break;
      }
    }
  }
}

const char *parse_token(const char **str)
{
  static char lbuf[MAXTOKENSIZE];       /* STATIC_RESULT: used for return, not across calls */
  char *cursor = lbuf;
  char quotechar = 0;
  bool escape = false;
  const char *ctoken = *str;

  if (!ctoken) {
      return 0;
  }
  eatwhitespace_c(&ctoken);
  if (!*ctoken) {
      return 0;
  }
  while (*ctoken && cursor - lbuf < MAXTOKENSIZE - 1) {
    ucs4_t ucs;
    size_t len;
    bool copy = false;

    unsigned char utf8_character = *(unsigned char *)ctoken;
    if (~utf8_character & 0x80) {
      ucs = utf8_character;
      len = 1;
    } else {
      int ret = unicode_utf8_to_ucs4(&ucs, ctoken, &len);
      if (ret != 0) {
        log_warning("illegal character sequence in UTF8 string: %s\n", ctoken);
        break;
      }
    }
    if (escape) {
      copy = true;
      escape = false;
    } else if (iswxspace((wint_t) ucs)) {
      if (quotechar == 0)
        break;
      copy = true;
    } else if (utf8_character == '"' || utf8_character == '\'') {
      if (utf8_character == quotechar) {
        ++ctoken;
        break;
      } else if (quotechar == 0) {
        quotechar = utf8_character;
        ++ctoken;
      } else {
        *cursor++ = *ctoken++;
      }
    } else if (utf8_character == SPACE_REPLACEMENT) {
      *cursor++ = ' ';
      ++ctoken;
    } else if (utf8_character == ESCAPE_CHAR) {
      escape = true;
      ++ctoken;
    } else {
      copy = true;
    }
    if (copy) {
      memcpy(cursor, ctoken, len);
      cursor += len;
      ctoken += len;
    }
  }

  *cursor = '\0';
  *str = ctoken;
  return lbuf;
}

const char *getstrtoken(void)
{
  return parse_token((const char **)&states->current_token);
}

int getid(void)
{
    const char *str = getstrtoken();
    int i = str ? atoi36(str) : 0;
    if (i < 0) {
        return -1;
    }
    return i;
}
