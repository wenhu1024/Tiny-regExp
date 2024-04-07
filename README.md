# Tiny-regExp
Tiny regExp in c.
Supports only ( | ) * + ? .
Compiles to NFA and then simulates NFA
using Thompson's algorithm

[reference](https://swtch.com/~rsc/regexp/regexp1.html)

## How to run
```
only for linux environment:

gcc RegExp_NFA.c -o regexp 
./regexp  "[input your reg]"   "[input string1 to match]"  "[input string2 to match]"   "..."

(please put string into " ", for the shell maybe regard '|' as a pipe not a regexp)

output:  some string match the regexp format 
```


## instruction of the code
