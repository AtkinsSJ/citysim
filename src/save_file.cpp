#pragma once

bool writeSaveFile(City *city, FileHandle *file)
{
	bool succeeded = file->isOpen;

	if (succeeded)
	{
		SAVFileHeader fileHeader = SAVFileHeader();
		succeeded = writeToFile<SAVFileHeader>(file, &fileHeader);
	}

	return succeeded;
}
