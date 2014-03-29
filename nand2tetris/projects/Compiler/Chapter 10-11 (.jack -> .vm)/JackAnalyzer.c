#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "JackAnalyzer.h"

#include "JackTokenizer.h"
#include "CompilationEngine.h"
#include "SymbolTable.h"

/* Get the VM output file name */
void getVMOutputFileName(char *string)
{
	int length;

	length = strlen(string);
	string[length - 4] = 'v';
	string[length - 3] = 'm';
	string[length - 2] = '\0';
}

/* Translate the jack file currentFileName */
void translateJackFile(void)
{
	jackTokenizer();
	compilationEngine();
}

void checkDeclaration(const char *line)
{
	/* Checks if in line there is a declaraction like a class declaraction, a class variable declaration, or a subroutine declaration */

	char *result;

	result = strstr(line, "class");
	if(result != NULL)
	{
		DefineClassDec(result);
		return ;
	}

	result = strstr(line, "static");
	if(result != NULL)
	{
		DefineClassVarDec(result, STATIC_KIND);
		return ;
	}

	result = strstr(line, "field");
	if(result != NULL)
	{
		DefineClassVarDec(result, FIELD_KIND);
		return ;
	}

	result = strstr(line, "function");
	if(result != NULL)
	{
		DefineSubroutineDec(result, FUNCTION_SUB);
		return ;
	}

	result = strstr(line, "constructor");
	if(result != NULL)
	{
		DefineSubroutineDec(result, CONSTRUCTOR_SUB);
		return ;
	}

	result = strstr(line, "method");
	if(result != NULL)
		DefineSubroutineDec(result, METHOD_SUB);
}

/* Clean the file fileName, by removing comments, useless space character, and empty lines */
void cleanJackFile(const char *fileName)
{
	FILE *tokenizerInput;
	int isInAComment;
	int lineNumber;

	isInAComment = 0;
	lineNumber = 1;

	/* Open the input  of the tokenizer (which is the output of this function) */
	tokenizerInput = fopen(fileName, "w+");

	while(!feof(currentJackFile))
	{
		char buffer[LENGTH_MAX];
		char line[LENGTH_MAX];
		int indexBuffer, indexLine;
		int nbSpace;

		nbSpace = 0;

		if(fgets(buffer, LENGTH_MAX, currentJackFile) != NULL)
			buffer[strlen(buffer)] = '\0';
		else
		{
			line[0] = '\0';
			break;
		}

		for(indexBuffer = 0, indexLine = 0; buffer[indexBuffer] != '\0'; ++indexBuffer)
		{	
			/* If it is a comment */
			if(buffer[indexBuffer] == '/' && buffer[indexBuffer + 1] == '*')
			{
				/* Remember that we are in a possible multi lines comment */
				isInAComment = 1;

				/* If on the same line there is the end of the comment */
				if(strstr(buffer, "*/") != NULL)
					/* Remember that we are in a possible multi lines comment */
					isInAComment = 0;

				/* Comments are ignored so we stop here */
				break;
			}
			/* If it is a simple comment line : // */
			if(buffer[indexBuffer] == '/' && buffer[indexBuffer + 1] == '/')
				/* Stop here */
				break;
			/* If it is the end of a multi lines comment */
			else if(buffer[indexBuffer] == '*' && buffer[indexBuffer + 1] == '/')
			{
				/* Remember that we are in a possible multi lines comment */
				isInAComment = 0;

				/* Continue to clean the rest of the line */
				++indexBuffer;
				continue ;
			}

			/* If we are not in a comment */
			if(!isInAComment)
			{
				/* If it is a space character */
				if(buffer[indexBuffer] == ' ')
				{
					/* Increment the counter of space character*/
					++nbSpace;

					/* If it is at least 1, we can copy it (more than 1 is useless, we copy only 1) */
					if(nbSpace <= 1)
					{
						line[indexLine] = buffer[indexBuffer];
						++indexLine;
					}
				}
				/* Else if it is not a control character (tabulation, new line etc.) */
				else if(!iscntrl(buffer[indexBuffer]))
				{
					/* Copy the character in line */
					line[indexLine] = buffer[indexBuffer];
					++indexLine;

					/* Reset the space character counter to 0 */
					nbSpace = 0;
				}
			}
		}

		line[indexLine] = '\0';

		/* Check if there is any declaration in the current line like a class declaration, a subroutine etc.*/
		checkDeclaration(line);

		/* Copy all the character of the clean line in the output (the tokenizer input) */
		for(indexLine = 0; line[indexLine] != '\0'; ++indexLine)
		{
			/* If the line begin with a space we don't copy it and we print the line number */
			if(indexLine == 0 && line[indexLine] == ' ')
			{
				fprintf(tokenizerInput, "%d ", lineNumber);
				continue ;
			}
			/* If the line don't begin with a space we juste print the line number */
			else if(indexLine == 0)
			{
				fprintf(tokenizerInput, "%d ", lineNumber);
				fputc(line[indexLine], tokenizerInput);
			}
			else
				fputc(line[indexLine], tokenizerInput);
		}

		/* Puts only new line if the clean line is note empty */
		if(line[0] != '\0')
			fputc('\n', tokenizerInput);

		++lineNumber;
	}

	fclose(tokenizerInput);
}

void prepareJackCompiler(void)
{
	/* Create the symbol table (class, subroutine) and clean all the files */
	char path[LENGTH_MAX];

	strcpy(path, currentFileName);
	createTokenizerOutputFileName(path);

	strcat(path, "_clean.xml");
	cleanJackFile(path);
}

void goThroughtDirectory(const char *directoryName, void (*pointer)(void))
{
	DIR *directory;
	struct dirent *file;

	/* Go throught the directory */
	directory = opendir(directoryName);

	while((file = readdir(directory)) != NULL)
	{
		/* If the file name is a .jack */
		if(strstr(file->d_name, ".jack") != NULL)
		{
			char fileName[LENGTH_MAX];

			/* Get the name of the .jack file */
			sprintf(fileName, "%s/%s", directoryName, file->d_name);
			strcpy(currentFileName, fileName);
			currentJackFile = fopen(fileName, "r");

			getVMOutputFileName(fileName);
			vmFile = fopen(fileName, "w+");

			/* Call the function in parameter */
			(*pointer)();

			fclose(currentJackFile);
			fclose(vmFile);
		}
	}

	closedir(directory);
}

int main(int argc, char *argv[])
{
	char vmFilePath[LENGTH_MAX];

	ConstructorSymbolTable();
	symbolTable.currentScope = CLASS_SCOPE;
	DefineJackStandardLibrarySubroutine();

	/* If it is a directory */
	if(strchr(argv[1], '.') == NULL)
	{
		/* Create the symbol table (class, subroutine) and clean all the files */
		goThroughtDirectory(argv[1], &prepareJackCompiler);
		/* Translate the .jack file */
		goThroughtDirectory(argv[1], &translateJackFile);
	}
	/* Else it is a simple .jack file */
	else
	{
		strcpy(currentFileName, argv[1]);
		currentJackFile = fopen(currentFileName, "r");

		strcpy(vmFilePath, argv[1]);
		getVMOutputFileName(vmFilePath);
		vmFile = fopen(vmFilePath, "w+");

		/* Create the symbol table (class, subroutine) and clean all the files */
		prepareJackCompiler();
		/* Translate the .jack file */
		translateJackFile();

		fclose(currentJackFile);
		fclose(vmFile);
	}
	
	return 0;
}
