
void initBudgetLayer(BudgetLayer *layer, City * /*city*/, MemoryArena * /*gameArena*/)
{
	*layer = {};
}

void saveBudgetLayer(BudgetLayer * /*layer*/, struct BinaryFileWriter *writer)
{
	writer->startSection<SAVSection_Budget>(SAV_BUDGET_ID, SAV_BUDGET_VERSION);
	SAVSection_Budget budgetSection = {};

	writer->endSection<SAVSection_Budget>(&budgetSection);
}

bool loadBudgetLayer(BudgetLayer * /*layer*/, City * /*city*/, struct BinaryFileReader *reader)
{
	bool succeeded = false;
	while (reader->startSection(SAV_BUDGET_ID, SAV_BUDGET_VERSION))
	{
		SAVSection_Budget *section = reader->readStruct<SAVSection_Budget>(0);
		if (!section) break;

		// TODO: Implement Budget!

		succeeded = true;
		break;
	}

	return succeeded;
}
