
inline City createCity(uint16 width, uint16 height) {
	City city;
	city.width = width;
	city.height = height;
	city.terrain = new Terrain[width*height]();

	return city;
}

inline void freeCity(City* city) {
	delete[] city->terrain;
	(*city) = {};
}

inline uint32 tileIndex(City *city, uint16 x, uint16 y) {
	return (y * city->width) + x;
}

void generateTerrain(City* city) {
	for (uint16 y = 0; y < city->height; y++) {
		for (uint16 x = 0; x < city->width; x++) {
			Terrain t = Terrain_Ground;
			if ((rand() % 100) < 10) {
				t = Terrain_Water;
			}
			city->terrain[tileIndex(city,x,y)] = t;
		}
	}
}