#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>

struct PluginOutput {
        int retCode;
        int prevRetCode;
        char retString[255];
};

int runPlugin()
{
        FILE *fp;
        char command[100];
        char retString[1035];
        struct PluginOutput output;
        char ch = '/';
        int rc = 0;


        printf("Enter the full path and name for the plugin to run: ");
        fgets(command, 100, stdin);
        printf("Try to run the following command:\n%s", command);
        fp = popen(command, "r");
        if (fp == NULL) {
                printf("Failed to run command\n");
                return 1;
        }
        while (fgets(retString, sizeof(retString), fp) != NULL) {
                printf("%s", retString);
        }
        rc = pclose(fp);
        if (rc > 0)
        {
                if (rc == 256)
                        output.retCode = 1;
                else if (rc == 512)
                        output.retCode = 2;
                else
                        output.retCode = rc;
        }
        strcpy(output.retString, retString);
        printf("\n\nRESULT OF EXECUTION:\n");
        printf("Return code is: %d", output.retCode);
        printf("\nReturn string is: %s\n", output.retString);
        return 0;
}

int main()
{
        printf("Test plugin...\n\n");
        runPlugin();
        return 0;
}
