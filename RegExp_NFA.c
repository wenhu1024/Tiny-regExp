#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *
re2post(char *re)
{
    int nalt = 0;  // recored the number of '|'
    int natom = 0; // recored the number of atom

    static char buf[8000];
    char *dst;
    struct
    {
        int nalt;
        int natom;
    } paren[100], *p; // record the condtion before '('

    p = paren;
    dst = buf;

    if (strlen(re) >= sizeof buf / 2)
    { // avoid overflow
        return NULL;
    }

    for (; *re; ++re)
    {
        switch (*re)
        {
        case '(':
            if (nalt > 1)
            {
                --nalt;
                *dst++ = '.';
            }
            if (p >= paren + 100)
            { // avoid overflow
                return NULL;
            }
            p->nalt = nalt;
            p->natom = natom;
            ++p;
            nalt = 0;
            natom = 0;
            break;
        case '|':
            if (natom == 0)
            { // wrong re
                return NULL;
            }
            while (--natom > 0)
            {
                *dst++ = '.';
            }
            ++nalt;
            break;
        case ')':
            if (p == paren)
            { // no '(' , wrong re
                return NULL;
            }
            if (natom == 0)
            { // wrong re
                return NULL;
            }
            while (--natom > 0)
            {
                *dst++ = '.';
            }
            for (; nalt > 0; --nalt)
            {
                *dst++ = '|';
            }
            --p;
            nalt = p->nalt;
            natom = p->natom;
            ++natom;
            break;
        case '*':
        case '+':
        case '?':
            if (natom == 0)
            {
                return NULL;
            }
            *dst++ = *re;
            break;
        default:
            if (natom > 1)
            {
                --natom;
                *dst++ = '.';
            }
            *dst++ = *re;
            ++natom;
            break;
        }
    }

    if (p != paren)
    {
        return NULL;
    }
    while (--natom > 0)
    {
        *dst++ = '.';
    }
    for (; nalt > 0; nalt--)
    {
        *dst++ = '|';
    }

    *dst = 0;
    return buf;
}

enum
{
    Split = 256,
    Match = 257
};
typedef struct State State;
struct State
{
    int c;
    State *out;
    State *out1;
    int lastList;
};

State matchstate = {Match};
int nstate;

State *
state(int c, State *out, State *out1)
{
    State *s;

    ++nstate;
    s = malloc(sizeof *s);
    s->c = c;
    s->out = out;
    s->out1 = out1;
    s->lastList = 0;
    return s;
}

typedef struct Frag Frag;
typedef union Ptrlist Ptrlist;

struct Frag
{
    State *start;
    Ptrlist *out;
};

Frag frag(State *start, Ptrlist *out)
{
    Frag n = {start, out};
    return n;
}

union Ptrlist
{
    Ptrlist *next;
    State *s;
};

Ptrlist *
list1(State **outp)
{
    Ptrlist *l;

    l = (Ptrlist *)outp;
    l->next = NULL;
    return l;
}

void patch(Ptrlist *l, State *s)
{
    Ptrlist *next;

    for (; l; l = next)
    {
        next = l->next;
        l->s = s;
    }
}

Ptrlist *
append(Ptrlist *l1, Ptrlist *l2)
{
    Ptrlist *oldl1;

    oldl1 = l1;
    while (l1->next)
    {
        l1 = l1->next;
    }
    l1->next = l2;
    return oldl1;
}

State *
post2nfa(char *postfix)
{
    char *p;
    Frag stack[1000], *stackp, e1, e2, e;
    State *s;

    if (postfix == NULL)
    {
        return NULL;
    }

#define push(s) *stackp++ = s
#define pop() *--stackp

    stackp = stack;
    for (p = postfix; *p; ++p)
    {
        switch (*p)
        {
        default:
            s = state(*p, NULL, NULL);
            push(frag(s, list1(&s->out)));
            break;
        case '.':
            e2 = pop();
            e1 = pop();
            patch(e1.out, e2.start);
            push(frag(e1.start, e2.out));
            break;
        case '|':
            e2 = pop();
            e1 = pop();
            s = state(Split, e1.start, e2.start);
            push(frag(s, append(e1.out, e2.out)));
            break;
        case '?':
            e = pop();
            s = state(Split, e.start, NULL);
            push(frag(s, append(e.out, list1(&s->out1))));
            break;
        case '*':
            e = pop();
            s = state(Split, e.start, NULL);
            patch(e.out, s);
            push(frag(s, list1(&s->out1)));
            break;
        case '+':
            e = pop();
            s = state(Split, e.start, NULL);
            patch(e.out, s);
            push(frag(e.start, list1(&s->out1)));
            break;
        }
    }

    e = pop();
    if (stackp != stack)
    {
        return NULL;
    }

    patch(e.out, &matchstate);
    return e.start;

#undef push
#undef pop
}

typedef struct List List;

struct List
{
    State **s;
    int n;
};

List l1, l2;
static int listid;

void addstate(List *, State *);
void step(List *, int, List *);

List *
startlist(State *start, List *l)
{
    l->n = 0;
    ++listid;
    addstate(l, start);
    return l;
}

int 
ismatch(List *l)
{
    int i;

    for (i = 0; i < l->n; ++i)
    {
        if (l->s[i] == &matchstate)
        {
            return 1;
        }
    }
    return 0;
}

void 
addstate(List *l, State *s)
{
    if (s == NULL || s->lastList == listid)
    {
        return;
    }

    s->lastList = listid;
    if (s->c == Split)
    {
        addstate(l, s->out);
        addstate(l, s->out1);
        return;
    }

    l->s[l->n++] = s;
}

void 
step(List *clist, int c, List *nlist)
{
    int i;
    State *s;

    ++listid;
    nlist->n = 0;
    for (i = 0; i < clist->n; ++i)
    {
        s = clist->s[i];
        if (s->c == c)
        {
            addstate(nlist, s->out);
        }
    }
}

int 
match(State *start, char *s)
{
    int i, c;
    List *clist, *nlist, *tmp;

    clist = startlist(start, &l1);
    nlist = &l2;
    for (; *s; s++)
    {
        c = *s & 0xFF;
        step(clist, c, nlist);
        tmp = clist;
        clist = nlist;
        nlist = tmp;
    }
    return ismatch(clist);
}

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

    post = re2post(argv[1]);

    if (post == NULL)
    {
        fprintf(stderr, "bad regexp %s\n", argv[1]);
        return 1;
    }

    start = post2nfa(post);
    if (start == NULL)
    {
        fprintf(stderr, "error in post2nfa %s\n", post);
        return 1;
    }

    l1.s = malloc(nstate * sizeof l1.s[0]);
    l2.s = malloc(nstate * sizeof l2.s[0]);

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