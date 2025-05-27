#pragma once

struct EducationLayer {
};

void initEducationLayer(EducationLayer* layer, City* city, MemoryArena* gameArena);
void saveEducationLayer(EducationLayer* layer, struct BinaryFileWriter* writer);
bool loadEducationLayer(EducationLayer* layer, City* city, struct BinaryFileReader* reader);
