// localisation
#pragma once

// This is a placeholder so I can find translatable strings. It will probably get renamed and replaced!
#define LocalString(str) stringFromChars(str)
// #define LocalString(str) (str)

enum TestLanguage
{
	LangEnglish,

	LangAlbanian,
	LangGreek,
	LangJapanese,

	Lang_Count
};

char *languageNames[] = {
	"English",

	"Albanian",
	"Greek",
	"Japanese",
};

char *getHelloOwlUTF8(TestLanguage language)
{
	switch (language)
	{
		case LangAlbanian: return "Kjo është owl im i preferuar.";
		case LangGreek: return "Αυτό είναι το αγαπημένο μου κουκουβάγια.";
		case LangJapanese: return "これは私の好きなフクロウです。";
		
		case LangEnglish:
		default: return "This is my favourite owl.";
	}
}

