#ifndef _CONFDB_EXAMPLE_UTILS_H_
#define _CONFDB_EXAMPLE_UTILS_H_

int str_split(const char *string, const char *delimiter, char ***result,
	      int *argc);
void str_free_split(char **split, int num);

#endif /* _CONFDB_EXAMPLE_UTILS_H_ */
