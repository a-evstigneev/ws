#ifndef KEYVAL_H
#define KEYVAL_H

struct keyval {
	char *key;
	char *value;
};

struct set_keyval {
	struct keyval *element;
	int ind_cur;
	int ind_max;
};

int add_record(struct set_keyval *ptr, const char *key, const char *val);

char *get_value(struct set_keyval name, const char *key);

#endif
