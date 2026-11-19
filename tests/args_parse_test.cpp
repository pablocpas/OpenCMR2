// Regression test for Args_Parse stack buffer overflow (Graphics.cpp).
//
// An over-long command-line token must not overflow the 260-byte word stack
// buffer, and more than 20 arguments must not overflow the 20-entry argument
// array. Both were unchecked before the fix: pOut wrote past word[] for any
// token longer than 259 chars, and g_unk0x00816820[g_unk0x00816974] wrote
// past the array for the 21st argument.
//
// Security: stack buffer overflow in command-line parsing. Reachable through
// the command-line arguments passed to the game binary.
extern int Args_Parse(char *);

#include <cstdio>
#include <cstring>

int main()
{
    // 1. Normal parsing works: --data, /path/to/game, --fullscreen = 3 args.
    char normal[] = "--data /path/to/game --fullscreen";
    int n = Args_Parse(normal);
    if (n != 3) {
        printf("FAIL: expected 3 args from normal command line, got %d\n", n);
        return 1;
    }

    // 2. Over-long token does not overflow word[260] or crash.
    char longToken[512];
    memset(longToken, 'A', sizeof(longToken) - 1);
    longToken[sizeof(longToken) - 1] = '\0';
    n = Args_Parse(longToken);
    if (n != 1) {
        printf("FAIL: expected 1 arg from over-long token, got %d\n", n);
        return 1;
    }

    // 3. More than 20 arguments does not overflow the 20-entry array.
    char manyArgs[600];
    int pos = 0;
    for (int i = 0; i < 25; i++)
        pos += snprintf(manyArgs + pos, sizeof(manyArgs) - pos, "arg%d ", i);
    manyArgs[sizeof(manyArgs) - 1] = '\0';
    n = Args_Parse(manyArgs);
    if (n != 20) {
        printf("FAIL: expected at most 20 args, got %d\n", n);
        return 1;
    }

    puts("args_parse: long token and argument-count bounds pass");
    return 0;
}
