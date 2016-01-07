//---------------------------------------------------------------------------------------
//      Script.ino -- Script processor
//---------------------------------------------------------------------------------------

#include <setjmp.h>

jmp_buf environment;    // A non-local jump is used to recover from 'alloc' errors.

struct code {                          // Linked list node structure
    struct code *next;
    char *text;
} head = {NULL, NULL}, *tail = &head;  // List management variables


static char *parmList[MaxTokens + 1];  // Traditional argument list passed to commands


//---------------------------------------------------------------------------------------
//      CompileCmd -- Compile a script presented on the serial input stream
//---------------------------------------------------------------------------------------

exitStatus CompileCmd (char **args)
{
    char **t;

    if (setjmp(environment) != 0) {    // Bounce back here on alloc error
        handleOverflow();
        return ParameterError;
    }
    freeScriptMemory();

    while (*(t = util.GetCommand()) != NULL) {    // Blank line terminates compilation
        do {
            tail = append(tail, *t);
        } while (*++t != NULL);

        tail = append(tail, NULL);
    }
    return Success;

}


//---------------------------------------------------------------------------------------
//      ListCmd -- List out the contents of the currently compiled script
//---------------------------------------------------------------------------------------

exitStatus ListCmd (char **args)
{
    struct code *p;

    for (p = head.next; p != NULL; p = p->next)
        if (p->text == NULL)
            Printf("\n");
        else
            Printf("%s ", p->text);

    return Success;

}


//---------------------------------------------------------------------------------------
//      RunCmd -- Run the current script
//---------------------------------------------------------------------------------------

exitStatus RunCmd (char **args)
{
    int i;
    char *name;
    exitStatus rc;
    struct code *p;

    static exitStatus goodRC[] =
        {Success, DipDetected, MaxTime, ChargeTempThreshold, SENTINEL};

    for (p = head.next; p != NULL; p = p->next) {
        for (i = 0; commandTable[i].command != NULL; i++)
            if (strcmp(commandTable[i].command, name = p->text) == 0)
                break;

        p = buildParmList(p);
        rc = (*commandTable[i].handler)(parmList);

        if (! memberQ(rc, goodRC)) {
            Printf("Untoward return code from '%s' (rc = %d) ", name, rc);
            return rc;
        }
    }
    return Success;

}


//---------------------------------------------------------------------------------------
//      Private routines...
//---------------------------------------------------------------------------------------

static struct code *append (struct code *tail, char *str)
{
    struct code *node = (struct code *) alloc(sizeof(struct code));

    node->next = NULL;
    node->text = NULL;
    tail->next = node;
    if (str != NULL)
        node->text = saveString(str);    // 'alloc' failure possible here too
    return node;

}


static struct code *buildParmList (struct code *p)
{
    memset(parmList, 0, sizeof(parmList));

    for (int i = 0; p->text != NULL; (i++, p = p->next))
        parmList[i] = p->text;

    return p;    // Output list size can't exceed input, therefore will fit.

}


static boolean memberQ (exitStatus element, exitStatus *set)
{
    for (int i = 0; set[i] != SENTINEL; i++)
        if (element == set[i])
            return true;

    return false;

}


static char *saveString (char *s)
{
    char *p = alloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}


static void handleOverflow (void)
{
    freeScriptMemory();                             // No partial scripts
    Printf("Out of memory in script compiler: ");

    delay(50);                          // Let rest of script flow
    while (Serial.available()) {        // ...in, and then discard it.
        (void) Serial.read();
        delay(50);
    }
}


//---------------------------------------------------------------------------------------
//      Dynamic memory manager, used exclusively by 'append' & 'saveString'
//---------------------------------------------------------------------------------------

#define PoolSize 600

static char memoryPool[PoolSize];       // fixed size memory pool
static char *allocp = memoryPool;       // next free position

static char *alloc (int n)
{
    if (memoryPool + PoolSize - allocp < n)    // Fatal error: out of memory
        longjmp(environment, 1);
    else {
        allocp += n;
        return allocp - n;
    }
}


static void freeScriptMemory (void)
{
    allocp = memoryPool;
    head.next = NULL;
    tail = &head;

}


exitStatus MemQCmd (char **args)        // <<< TESTING >>>
{
    int used = allocp - memoryPool;
    int avail = PoolSize - used;
    Printf("In use: %d bytes. Available: %d bytes.\n", used, avail);

    return Success;

}
