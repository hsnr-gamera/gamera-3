#include <stdio.h>

void parseArgs(int, char**);
void printUsage();
void doPurePython();
void doMinGW();
void doMSVC();
void doICL();
FILE* templ, *outMM;
int numCompilers=0;
int compilerActive[3] = {0,0,0};

int main(int argc, char** argv)
{
	int i = 0;
	if(argc==1) {printUsage(); return 1;} //have to provide some arguments
	
	
	parseArgs(argc,argv); //figure out what the user wants to do

	doPurePython(); //always rebuild the Pure Python modules
	for(; i<3 && numCompilers>0;i++) //configure the .mm & handle build products for each compiled version
	{
		if(compilerActive[i]) switch(i)
		{
			case 0:
				doMinGW();
				break;
			case 1:
				doMSVC();
				break;
			case 2:
				doICL();
				break;
		}
	}
	system("mm");
	system("copy /Y .\\out\\Gamera2.MM\\Msi\\Gamera2.msi ..\\Gamera2.msi");
	system("rmdir /S /Q out");
	return 0;
}

void doPurePython()
{
	char buff[2048];
	printf("Preparing pure Python modules & scripts.\n");
	
	getcwd(buff,2048);
	chdir("..");
	system("cd..");
	system("rmdir /S /Q build");
	system("python setup.py build_py build_scripts");
	system("python setup.py install --skip-build --prefix .\\dist\\pure");
	system("rmdir /S /Q build");
	chdir(buff);

	system("copy /Y Gamera2.MM.template Gamera2.MM");
}

void doMinGW()
{
	if(compilerActive[0]==1)
	{
		char buff[2048];
		printf("Compiling extensions with MingW32 compiler.\n");

		getcwd(buff,2048);
		chdir("..");
		system("rmdir /S /Q build");
		system("python setup.py build_ext --compiler=mingw32");
		system("mkdir .\\dist\\mingw\\Lib\\site-packages\\gamera\\");
		system("xcopy /Y /E /Q .\\build\\lib.win32-2.3\\gamera .\\dist\\mingw\\Lib\\site-packages\\gamera");
		system("rmdir /S /Q build");
		chdir(buff);
    }
	outMM = fopen("Gamera2.MM","a");
	fprintf(outMM,"\n\n;----------Feature to Add Extensions built with MingW32---------\n");
	fprintf(outMM,"<$Feature \"mingw_ext\" Title=\"Gamera 2 Extensions (MingW32)\">\n");
	fprintf(outMM,"<$Files \"..\\dist\\mingw\\*\" SubDir=TREE DestDir=\"[PY23LOC]\">\n");
	fprintf(outMM,"<$Files \"..\\mgwz.dll\" DestDir=\"[PY23LOC]\\Lib\\site-packages\\Gamera\\plugins\">\n");
	fprintf(outMM,"<$/Feature>");
	fclose(outMM);
}
void doMSVC()
{
  if(compilerActive[1]==1)
    {
	char buff[2048];
	printf("Compiling extensions with Microsoft Visual C compiler.\n");
	getcwd(buff,2048);
	chdir("..");
	system("rmdir /S /Q build");
	system("python setup.py build_ext");
	system("mkdir .\\dist\\msvc\\Lib\\site-packages\\gamera\\");
	system("xcopy /Y /E /Q .\\build\\lib.win32-2.3\\gamera .\\dist\\msvc\\Lib\\site-packages\\gamera");
	system("rmdir /S /Q build");
	chdir(buff);
    }
	outMM = fopen("Gamera2.MM","a");
	fprintf(outMM,"\n\n;----------Feature to Add Extensions built with MSVC---------\n");
	fprintf(outMM,"<$Feature \"msvc_ext\" Title=\"Gamera 2 Extensions (MSVC)\">\n");
	fprintf(outMM,"<$Files \"..\\dist\\msvc\\*\" SubDir=TREE DestDir=\"[PY23LOC]\">\n");
	fprintf(outMM,"<$/Feature>");
	fclose(outMM);
}

void doICL()
{
  if(compilerActive[2]==1)
    {
	char buff[2048];
	printf("Compiling extensions with Intel compiler.\n");
	getcwd(buff,2048);
	chdir("..");
	system("rmdir /S /Q build");
	system("python setup.py build_ext --compiler=icl");
	system("mkdir .\\dist\\icl\\Lib\\site-packages\\gamera\\");
	system("xcopy /Y /E /Q .\\build\\lib.win32-2.3\\gamera .\\dist\\icl\\Lib\\site-packages\\gamera");
	system("rmdir /S /Q build");\
	chdir(buff);
    }
	outMM = fopen("Gamera2.MM","a");
	fprintf(outMM,"<$Feature \"icl_ext\" Title=\"Gamera 2 Extensions (ICL)\">\n");
	fprintf(outMM,"<$Files \"..\\dist\\icl\\*\" SubDir=TREE DestDir=\"[PY23LOC]\">\n");
	fprintf(outMM,"<$/Feature>\n");
	fclose(outMM);
}

void parseArgs(int argc, char** argv)
{
	int i = 1;
	for(;i<argc;i++)
	{
		if(argv[i][0]=='-' || argv[i][0] == '/')
		{
			char* temp = &argv[i][1];
			if(strcmp(temp,"c")==0 || strcmp(temp,"compiler")==0)
			{
				temp = argv[++i];
				if(strcmp(temp,"mingw")==0)
				{
					compilerActive[0] = 1;
				}
				else if(strcmp(temp,"msvc")==0)
				{
					compilerActive[1] = 1;
				}
				else if(strcmp(temp,"icl")==0)
				{
					compilerActive[2] = 1;
				}
				else{printf("Do not know how to handle compiler \"%s\".  Skipping it.\n",temp); continue;}
				numCompilers++;
			}
			else if(strcmp(temp,"i")==0 || strcmp(temp,"include")==0)
			{
				temp = argv[++i];
				if(strcmp(temp,"mingw")==0)
				{
					compilerActive[0] = (compilerActive[0]==1?1:-1);
				}
				else if(strcmp(temp,"msvc")==0)
				{
					compilerActive[1] = (compilerActive[1]==1?1:-1);
				}
				else if(strcmp(temp,"icl")==0)
				{
					compilerActive[2] = (compilerActive[2]==1?1:-1);
				}
				else{printf("Do not know how to handle compiler \"%s\".  Skipping it.\n",temp); continue;}
				numCompilers++;
			}
			else{printf("Skipping unknown flag %s\n",argv[i]);}
		}
		else{printf("Skipping unknown flag %s\n",argv[i]);}
	}
}

void printUsage()
{
	printf("Correct usage is:\ncreatemsi [options]\n\n");
	printf("Options:\n");          
	printf("  -c [mingw|msvc|icl]  Specify C compiler for Gamera extensions\n");
	printf("  -compiler [...]      Same as -c\n");
	printf("  -i [mingw|msvc|icl]  Include existing build products into MSI\n");
	printf("                       (please ensure these exist before attempting to\n");
	printf("                        include them!)\n");
	printf("  -include [...]       Same as -i\n");
}
