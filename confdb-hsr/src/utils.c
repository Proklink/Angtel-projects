#include <stdlib.h>
#include <string.h>

int str_split(const char *string, const char *delimiter, char ***result,
	      int *argc)
{
	unsigned int sz = 4, idx = 0;
	char *copy;
	char *copystart = NULL;
	char **lines;
	int num = 0;
	const char *tok = NULL;

	*result = NULL;
	*argc = 0;

	if (!string)
		return 0;

	lines = calloc(sizeof(char *) * sz, 1);
	if (!lines)
		goto on_error;

	copystart = strdup(string);
	copy = strdup(string);
	if (!copy)
		goto on_error;

	while (copy) {
		tok = strsep(&copy, delimiter);
		lines[idx] = strdup(tok);
		if (!lines[idx])
			goto on_error;

		if (++idx == sz) {
			lines = realloc(lines, (sz *= 2) * sizeof(char *));
			if (!lines)
				goto on_error;
		}
		num++;
	}

	free(copystart);

	*result = lines;
	*argc = num;
	return 0;
on_error:
	if (lines) {
		for (int i = 0; i < idx; i++)
			free(lines[i]);

		free(lines);
	}
	free(copystart);
	return -1;
}

void str_free_split(char **split, int num)
{
	for (int i = 0; i < num; i++)
		free(split[i]);

	free(split);
}
