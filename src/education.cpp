
void initEducationLayer(EducationLayer *layer, City * /*city*/, MemoryArena * /*gameArena*/)
{
	*layer = {};
}

void saveEducationLayer(EducationLayer * /*layer*/, struct BinaryFileWriter *writer)
{
	writer->startSection<SAVSection_Education>(SAV_EDUCATION_ID, SAV_EDUCATION_VERSION);
	SAVSection_Education educationSection = {};

	writer->endSection<SAVSection_Education>(&educationSection);
}

bool loadEducationLayer(EducationLayer * /*layer*/, City * /*city*/, struct BinaryFileReader *reader)
{
	bool succeeded = false;
	while (reader->startSection(SAV_EDUCATION_ID, SAV_EDUCATION_VERSION))
	{
		SAVSection_Education *section = reader->readStruct<SAVSection_Education>(0);
		if (!section) break;

		succeeded = true;
		break;
	}

	return succeeded;
}
