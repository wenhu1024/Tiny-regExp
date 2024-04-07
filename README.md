# Tiny-regExp
Tiny regExp in c.
<br>
Supports only ( | ) * + ? .
<br>
Compiles to NFA and then simulates NFA
<br>
using Thompson's algorithm

[reference:  https://swtch.com/~rsc/regexp/regexp1.html](https://swtch.com/~rsc/regexp/regexp1.html)

## How to run
only for linux environment:
```
gcc RegExp_NFA.c -o regexp 
./regexp  "[input your reg]"   "[input string1 to match]"  "[input string2 to match]"   "..."

(please put string into " ", for the shell maybe regard '|' as a pipe not a regexp)
```
output:  some string match the regexp format 

## Instruction of the code
<br>
let us observe the code logic through main

```cpp
int main(int argc, char **argv)
{
    int i;
    char *post;
    State *start;

    if (argc < 3)
    {
        fprintf(stderr, "usage: nfa regexp string...\n");
        return 1;
    }
    /**
     * first, re2post function change reg to postorder, 
     * and return char * to post
     */
    post = re2post(argv[1]);    

    if (post == NULL)
    {
        fprintf(stderr, "bad regexp %s\n", argv[1]);
        return 1;
    }

    /**
     * second, post2nfa function compile the postorder reg to NFA, 
     * and return start of the NFA
     */
    start = post2nfa(post);
    if (start == NULL)
    {
        fprintf(stderr, "error in post2nfa %s\n", post);
        return 1;
    }

    l1.s = malloc(nstate * sizeof l1.s[0]);
    l2.s = malloc(nstate * sizeof l2.s[0]);

    /**
     * Third, match function can check the argv[i] is matched or not, 
     * return 1 or 0
     */
    for (i = 2; i < argc; i++)
    {
        if (match(start, argv[i]))
        {
            printf("%s\n", argv[i]);
        }
    }

    free(l1.s);
    free(l2.s);
    return 0;
}
```
