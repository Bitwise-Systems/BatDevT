//---------------------------------------------------------------------------------------
//      Script.ino  --  Script compiler & interpreter
//---------------------------------------------------------------------------------------

char *commandText = NULL;           // Command text buffer
const int AllocSize = 200;          // Initial size for buffer allocation
int bufsize = 0;                    // Actual buffer size after truncation


#define MaxCommandsPerScript 15     // Maximum script length (in lines)
#define MaxTokensPerCommand 4       // Command name plus three arguments

static char *tokptr[MaxCommandsPerScript + 1][MaxTokensPerCommand + 1];


exitStatus CompileCmd (char **args)    // Compile a script
{
    int i, j;
    char **t, *bufptr;
    int bytesToCopy, bytesAvail = AllocSize;

    if (commandText != NULL)
        free(commandText);

    bufptr = commandText = (char *) malloc(AllocSize);
    if (commandText == NULL) {
        abortCompile("can't allocate memory");
        return ParameterError;
    }
    for (i = 0; *(t = util.GetCommand()) != NULL; i++) {
        j = 0;
        do {
            if (i >= MaxCommandsPerScript) {
                abortCompile("too many commands");
                return ParameterError;
            }
            if (j >= MaxTokensPerCommand) {
                abortCompile("too many tokens");
                return ParameterError;
            }
            if ((bytesToCopy = strlen(*t) + 1) > bytesAvail) {
                abortCompile("text buffer overflow");
                return ParameterError;
            }
            tokptr[i][j++] = bufptr;
            strncpy(bufptr, *t, bytesToCopy);
            bufptr += bytesToCopy;
            bytesAvail -= bytesToCopy;

        } while (*++t != NULL);
        tokptr[i][j] = NULL;
    }
    tokptr[i][0] = NULL;
    bufsize = bufptr - commandText;
    commandText = (char *) realloc(commandText, bufsize);

    return Success;

}


static void abortCompile (char *errorMessage)    // Recover from compiler errors
{
    Printf("Script compiler error: %s\n", errorMessage);
    if (commandText != NULL) {
        free(commandText);
        commandText = NULL;
    }
    bufsize = 0;
    memset(tokptr, 0, sizeof tokptr);

}


exitStatus RunCmd (char **args)      // Script command processor
{
    int i, j;
    exitStatus rc;
    static exitStatus goodRC[] = {Success, DipDetected, MaxTime, ChargeTempThreshold, SENTINEL};

    for (j = 0; tokptr[j][0] != NULL; j++) {
        for (i = 0; commandTable[i].command != NULL; i++)
            if (strcmp(commandTable[i].command, *tokptr[j]) == 0)
                break;

        rc = (*commandTable[i].handler)(tokptr[j]);

        if (! memberQ(rc, goodRC)) {
            Printf("Untoward return code from '%s' (rc = %d), ", *tokptr[j], rc);
            return rc;
        }
    }
    return Success;

}


static boolean memberQ (exitStatus element, exitStatus *set)    // Set membership query
{
    for (int i = 0; set[i] != SENTINEL; i++)
        if (element == set[i])
            return true;

    return false;

}


exitStatus ListScriptCmd (char **args)    // List script contents
{
    for (int i = 0; tokptr[i][0] != NULL; i++) {
        Printf(":   ");
        for (int j = 0; tokptr[i][j] != NULL; j++)
            Printf("%s ", tokptr[i][j]);
        Printf("\n");
    }
    return Success;

}
