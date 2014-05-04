#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <new>

#include "../src/am_generic_stack.h"

struct A {
    virtual void method() = 0;
};

struct B : A {
    int i;
    B(int ii) {
        i = ii;
    }
    virtual void method() {
        printf("B %d\n", i);
    }
};

struct C : A {
    double dx;
    double y;
    int x;
    double z;
    C(int i) {
        x = i;
    }
    virtual void method() {
        printf("C %d\n", x);
    }
};

struct D : A {
    double x;
    double y;
    double z;
    int a, b, c, d, e,f, g, h, i, j, k;
    char str[256];
    D(int x) {
        j = x;
    }
    virtual void method() {
        printf("D %d\n", j);
    }
};

int main () {
    am_generic_stack<A> s;
    for (int i = 0; i < 3; i++) {
        new (s.push(0, sizeof(D))) D(1);
        new (s.push(0, sizeof(B))) B(2);
        new (s.push(0, sizeof(B))) B(3);
        new (s.push(0, sizeof(B))) B(4);
        new (s.push(0, sizeof(C))) C(5);
        new (s.push(0, sizeof(C))) C(6);
        new (s.push(0, sizeof(C))) C(7);
        new (s.push(0, sizeof(B))) B(8);
        new (s.push(0, sizeof(C))) C(9);
        s.get_top()->method();
        s.pop();
        s.get_top()->method();
        s.pop();
        new (s.push(0, sizeof(D))) D(10);
        new (s.push(0, sizeof(B))) B(11);
        new (s.push(0, sizeof(D))) D(12);
        new (s.push(0, sizeof(B))) B(13);
        new (s.push(0, sizeof(C))) C(14);
        new (s.push(0, sizeof(D))) D(15);
        while (!s.is_empty()) {
            s.get_top()->method();
            s.pop();
        }
    }
    return 0;
}
