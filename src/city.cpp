
inline City createCity(uint16 width, uint16 height) {
	City city;
	city.width = width;
	city.height = height;
	city.terrain = (Terrain*) calloc(width*height, sizeof(Terrain));

	return city;
}

inline void freeCity(City* city) {
	free(city->terrain);
	(*city) = {};
}

inline uint32 tileIndex(City *city, uint16 x, uint16 y) {
	return (y * city->width) + x;
}

void generateTerrain(City* city) {
	for (int y = 0; y < city->height; y++)
	{
		for (int x = 0; x < city->width; x++)
		{
			city->terrain[tileIndex(city,x,y)] = Terrain_Water;
		}
	}
}