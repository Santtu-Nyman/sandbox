#include <stddef.h>
#include <stdint.h>

void quick_sort(size_t length, int* data)
{
	size_t j = (length - 1) / 2;
	if (data[length - 1] < data[0])
	{
		int t = data[length - 1];
		data[length - 1] = data[0];
		data[0] = t;
	}
	if (data[j] < data[0])
	{
		int t = data[j];
		data[j] = data[0];
		data[0] = t;
	}
	if (data[length - 1] < data[j])
	{
		int t = data[length - 1];
		data[length - 1] = data[j];
		data[j] = t;
	}
	int x = data[j];

	j = length;
	for (size_t i = (size_t)~0;;)
	{
		while (data[--j] > x)
			continue;
		while (data[++i] < x)
			continue;
		if (i < j)
		{
			int t = data[i];
			data[i] = data[j];
			data[j] = t;
		}
		else
			break;
	}
	++j;

	if (j > 1)
		quick_sort(j, data);
	if (length - j > 1)
		quick_sort(length - j, data + j);
}
