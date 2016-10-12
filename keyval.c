#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "servfunc.h"
#include "keyval.h"

enum {
	INITMP = 1,
	GROWMP = 2
};

// add_record() - функция для добавления строки ключ-значение в динамический массив   
int 
add_record(struct set_keyval *ptr, const char *key, const char *val)
{
	struct keyval *temp;
	int state = 0;

	if (ptr->element == NULL) {
		temp = realloc(NULL, INITMP * sizeof(struct keyval));
		if (temp == NULL)
			return -1;
		ptr->ind_max = INITMP;
		ptr->element = temp;
	} 
	else if (ptr->ind_cur >= ptr->ind_max) {
		temp = realloc(ptr->element, GROWMP * (ptr->ind_max) * sizeof(struct keyval));
		if (temp == NULL)  
			return -1;
		ptr->ind_max *= GROWMP;
		ptr->element = temp;
	}
	
	if (key == NULL)
		return ptr->ind_cur;
	else if (val != NULL) {
		if ( (ptr->element[ptr->ind_cur].key = strdup(key)) == NULL)
			return -1; 
		if ( (ptr->element[ptr->ind_cur].value = strdup(val)) == NULL)
			return - 1;
	}
	else
		ptr->element[ptr->ind_cur].value = NULL;
		
	return ptr->ind_cur++;
}

// get_value() - функция, которая возвращает значение по ключу из динамического массива 
char *
get_value(struct set_keyval name, const char *key)
{
	int i;
	
	if (key != NULL) 
		for (i = 0; i < name.ind_cur; ++i) 
			if (strcmp(name.element[i].key, key) == 0)
				return name.element[i].value;
	return NULL;
}
