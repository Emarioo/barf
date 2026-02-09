

static int some_static = 100;

int counter = 0;

static void fixed_update(int value) {
    some_static += value;
}

int update(int value) {
    counter += 1;
    fixed_update(value);
    return some_static;
}


