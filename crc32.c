uint32_t simple_crc32(size_t data_size, const void* data)
{
	uint32_t hash = 0xFFFFFFFF;
	for (const uint8_t* read = (const uint8_t*)data, * read_end = read + data_size; read != read_end; ++read)
	{
		hash ^= (uint32_t)*read;
		for (int c = 8; c--;)
			hash = (hash >> 1) ^ (0xEDB88320 & (0 - (hash & 1)));
	}
	return ~hash;
}