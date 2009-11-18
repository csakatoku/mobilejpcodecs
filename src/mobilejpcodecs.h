#ifndef _MOBILEJPCODECS_H_
#define _MOBILEJPCODECS_H_

struct emoji_t {
  const char *data;
  size_t length;
};

#define emoji_string(str) { str, sizeof(str) - 1 }

#endif
