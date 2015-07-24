/*

	OK, so this is pretty dumb, I know.
	The problem is, when I build, the VS compiler returns all errors in its initial file
	as relative paths - "..\src\main.cpp", which Sublime doesn't understand and can't jump
	to. AND SO, here's a dumb file that includes main.cpp so that I can jump to erros with F4.
*/

#include "main.cpp"