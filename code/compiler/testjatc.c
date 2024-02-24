#include"jatcc.h"

int main(){
    const char * codeCstring =  "#include <stdio.h>\n\nint main () {\n    printf(\"Hello World\\n\");\n    return 0;\n}";
    char* result = Jatcc(codeCstring);
    printf("result: %s \n",result);
    return 0;
}