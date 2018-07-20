
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

double var_list[100];
FILE *infp, *outfp;
int finished = 0;
char infn[1024];
char outfn[1024];
int line_number = 1;

/*
    Display an error and abort the program.
*/
void show_error(const char* fmt, ...) {
    va_list args;

    fprintf(stderr, "error: %s: %d: ", infn, line_number);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    if(infp != NULL)
        fclose(infp);
    if(outfp != NULL)
        fclose(outfp);

    fprintf(stderr, "\n");
    exit(1);
}

/*
    Just make the code look better.
*/
static inline void unget_char(int ch) {
    ungetc(ch, infp);
}

/*
    Read a character from the input stream and increment line numbers.
*/
static inline int get_char(void) {

    register int ch = fgetc(infp);
    if(ch == '\n')
        line_number++;

    return ch;
}

/*
    Skip white space but write it to the output file.
*/
static inline void skip_ws(void) {
    int ch; 
    
    while(1) {
        ch = get_char();
        if(!isspace(ch)) {
            unget_char(ch);
            break;
        }
        else {
            fputc(ch, outfp);
        }
    }
}

/*
    Get a 2 digit decimal number from the input stream. Return the 
    value.
*/
int get_var_number(void) {

    int retv = 0;
    int ch;

    for(ch = get_char(); isdigit(ch); ch = get_char()) {
        retv *= 10;
        retv += ch - '0';
    }

    if(retv > 99) 
        show_error("variable is greater than 99. (%d)", retv);

    unget_char(ch);
    return retv;
}

/*
    Stupidly read a floating point number that can be negative, but that has
    no exponent syntax. Returns the value, not the string.
*/
double get_number(void) {

    char str[64];
    int idx = 0;
    int ch;
        
    for(idx = 0, ch = get_char(); isdigit(ch) || ch == '.' || ch == '-'; idx++, ch = get_char()) {
        str[idx] = ch;
        if(idx > sizeof(str)-2) 
            show_error("number over 64 characters was found. %s", str);
    }

    unget_char(ch);
    if(strchr(str, '.') != strrchr(str, '.')) 
        show_error("more than one \'.\' found in a number.");
    
    if(strchr(str, '-') != strrchr(str, '-')) 
        show_error("more than one \'-\' found in a number.");

    return strtod(str, NULL);
}

/*
    Handle expressions and output the actual value that the expression results
    in. Expressions are very simple. They can have the following forms:

    [VAR]
    [NUMBER * VAR]
    [NUMBER * VAR + VAR]
    All other forms are rejected with an error.
*/
void do_the_expression(void) {

    int ch;
    double num;
    int var_val;

    // if the next character is a '-' or isdigit() then it's a number,
    // else if it is a '#', then it's a var. Otherwise, it's an error.
    skip_ws();
    ch = get_char();
    if(isdigit(ch) || ch == '-') {
        unget_char(ch);
        num = get_number();
    }
    else if(ch == '#') {
        var_val = get_var_number();
        fprintf(outfp, "%0.06f", var_list[var_val]);
        skip_ws();
        ch = get_char();
        if(ch != ']')
            show_error("expected a \']\' but got a %c.", ch);
        return;
    }

    // if we reach here, we have a number, so expect a '*' next.
    skip_ws();
    ch = get_char();
    if(ch != '*')
        show_error("expected a \'*\' but got a %c", ch);

    skip_ws();
    ch = get_char();
    if(ch != '#')
        show_error("expected a \'#\' but got a %c", ch);
    
    var_val = get_var_number();
    num *= var_list[var_val];

    skip_ws();
    ch = get_char();
    if(ch == '+') {
        skip_ws();
        ch = get_char();
        if(ch != '#')
            show_error("expected a \'#\' but got a %c", ch);

        var_val = get_var_number();
        num += var_list[var_val];
    }
    else
        unget_char(ch);

    skip_ws();
    ch = get_char();
    if(ch != ']')
        show_error("expected a \']\' but got a %c", ch);

    fprintf(outfp, "%0.06f", num);
}

/*
    Handle var definitions on the form #nn where nn is a two digit number that
    is used to index the array of variables. Definitions are not output to the
    out file.

    Example:
    #10 = 123.099
*/
void do_the_definition(void) {

    int index;
    int ch;
    double num;

    // get one or two digits.
    index = get_var_number();
    skip_ws();

    // followed by an '='.
    ch = get_char();
    if(ch != '=')
        show_error("expected a \'=\' but got a \'%c\'.", ch);
    skip_ws();

    // followed by a number.
    num = get_number();
    var_list[index] = num;
}

/*
    Main function. Handle file iopening and closing, etc.
*/
int main(int argc, char** argv) {

    if(argc > 1) {
        strncpy(infn, argv[1], sizeof(infn));
        printf("opening input file: %s\n", infn);
        if(NULL == (infp = fopen(infn, "r"))) {
            fprintf(stderr, "error: cannot open input file: %s: %s\n", infn, strerror(errno));
            return(1);
        }
    }
    else {
        fprintf(stderr, "error: input file needed.\n");
        return(1);
    }

    strncpy(outfn, infn, sizeof(outfn));
    strncat(outfn, ".nc", sizeof(outfn));
    printf("opening output file: %s\n", infn);
    if(NULL == (outfp = fopen(outfn, "w"))) {
        fprintf(stderr, "error: cannot open output file: %s: %s\n", outfn, strerror(errno));
        fclose(infp);
        return(1);
    }

    while(1) {
        int ch = get_char();

        if(ch == EOF) {
            break;
        }

        if(ch == '[') {
            do_the_expression();
        }
        else if(ch == '#') {
            do_the_definition();
        }
        else {
            fputc(ch, outfp);
        }
    }

    fclose(infp);
    fclose(outfp);
    return(0);
}