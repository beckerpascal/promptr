/* praat_script.h
 *
 * Copyright (C) 1992-2007 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * pb 2007/06/09
 */

#ifndef _Interpreter_h_
	#include "Interpreter.h"
#endif

int praat_executeCommand (Interpreter me, const wchar_t *command);
int praat_executeCommandFromStandardInput (const char *programName);
int praat_executeScriptFromFile (MelderFile file, const wchar_t *arguments);
int praat_executeScriptFromFileNameWithArguments (const wchar_t *nameAndArguments);
int praat_executeScriptFromText (wchar_t *text);
int praat_executeScriptFromDialog (Any dia);
int DO_praat_runScript (Any sender, void *dummy);
int DO_RunTheScriptFromAnyAddedMenuCommand (Any sender, void *dummy);
int DO_RunTheScriptFromAnyAddedEditorCommand (Any editor, const wchar_t *script);

/* End of file praat_script.h */
