#pragma once

struct BudgetLayer {
};

void initBudgetLayer(BudgetLayer* layer, City* city, MemoryArena* gameArena);
void saveBudgetLayer(BudgetLayer* layer, struct BinaryFileWriter* writer);
bool loadBudgetLayer(BudgetLayer* layer, City* city, struct BinaryFileReader* reader);
