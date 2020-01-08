#pragma once

inline DateTime getLocalTimeFromTimestamp(u64 unixTimestamp)
{
	return platform_getLocalTimeFromTimestamp(unixTimestamp);
}

String formatDateTime(DateTime dateTime, DateTimeFormat format)
{
	// TODO: Handle the various formats!

	return myprintf("{0}/{1}/{2} {3}:{4}:{5}"_s, {
		formatInt(dateTime.year),
		formatInt(dateTime.month),
		formatInt(dateTime.dayOfMonth),
		formatInt(dateTime.hour),
		formatInt(dateTime.minute),
		formatInt(dateTime.second)
	});
}
