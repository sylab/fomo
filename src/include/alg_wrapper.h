#ifndef INCLUDE_ALG_WRAPPER_H
#define INCLUDE_ALG_WRAPPER_H

#include "cache_nucleus_struct.h"
#include "common.h"
#include <string.h>

struct alg_field {
  char *id;
  bool *field;
};

struct alg_wrapper;

typedef int (*alg_wrapper_init_f)(struct alg_wrapper *alg_w, char *fields_str);
typedef void (*alg_wrapper_print_f)(struct alg_wrapper *alg_w);

struct alg_wrapper {
  struct cache_nucleus *nucleus;

  alg_wrapper_init_f init;
  alg_wrapper_print_f print;
};

static int alg_wrapper_init(struct alg_wrapper *alg_w, char *fields_str) {
  return alg_w->init(alg_w, fields_str);
}

static void alg_wrapper_print(struct alg_wrapper *alg_w) {
  alg_w->print(alg_w);
}

static int alg_field_set(char *id, struct alg_field *alg_fields) {
  while (alg_fields->id != NULL) {
    if (strcmp(id, alg_fields->id) == 0) {
      *alg_fields->field = true;
      return 0;
    }
    alg_fields++;
  }

  return -1;
}

static int alg_wrapper_read(char *fields_str, struct alg_field *alg_fields) {
  int r = 0;

  int len = strlen(fields_str);
  char *tmp = mem_alloc((len + 1) * sizeof(*tmp));
  char *p;

  LOG_ASSERT(tmp != NULL);
  strcpy(tmp, fields_str);

  for (p = tmp; *p != '\0'; p++) {
    if (*p == ',') {
      *p = '\0';
      r = alg_field_set(tmp, alg_fields);
      if (r) {
        goto read_out;
      }
      tmp = p + 1;
    }
  }
  r = alg_field_set(tmp, alg_fields);

read_out:
  return r;
}

static void alg_wrapper_init_functions(struct alg_wrapper *alg_w,
                                       alg_wrapper_init_f init,
                                       alg_wrapper_print_f print) {
  alg_w->init = init;
  alg_w->print = print;
}

#endif /* INCLUDE_ALG_WRAPPER_H */
