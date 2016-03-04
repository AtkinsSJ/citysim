// localisation

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
		
		default: return "This is my favourite owl.";
	}
}