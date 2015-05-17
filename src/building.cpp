
inline Building createBuilding(BuildingArchetype archetype, Coord position) {

	BuildingDefinition *def = buildingDefinitions + archetype;

	Building building = {};
	building.archetype = archetype;
	building.footprint = {position, def->width, def->height};

	return building;
}