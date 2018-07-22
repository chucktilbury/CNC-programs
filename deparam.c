
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

double var_list[100];
FILE *infp, *outfp;
int finished = 0;
char infn[1024];
char outfn[1024];
char progname[1024];
int line_number = 0;
int expression_count = 0;
int verbose_flag = 0;
int deparam_flag = 0;
int swap_flag = 0;

char *help_text[] = {
    "",
    "Pre-process a g-code file",
    "",
    "Required parameters:",
    "  -i <input file name>",
    "  -o <output file name>",
    "",
    "One or more of these flags may be used:",
    "  -d de-parameterize the input file",
    //"  -s swap the x and the y axis",
    "  -v verbose operation",
    "  -h show this help text and do nothing",
    "",
    "Example:",
    "  deparam -vd -i input.nc -o output.nc",
    "  This command will verbosely remove the parameters",
    "  from the input file and write the result to the",
    "  output file.",
    "",
    NULL
};

/*
    Create a string with the time without the stupid '\n' that
    ctime() puts in.
*/
static inline const char* get_time(void) {
    
    static char str[100];
    char *strp;
    time_t t = time(NULL);

    memset(str, 0, sizeof(str));
    ctime_r(&t, str);
    if(NULL != (strp = strrchr(str, '\n')))
        *strp = 0;

    return str;
}

/*
    Show a message if verbosity is enabled.
*/
void show_msg(const char* fmt, ...) {
    va_list args;

    if(verbose_flag) {
        va_start(args, fmt);
        vfprintf(stdout, fmt, args);
        va_end(args);
        fprintf(stdout, "\n");
        fflush(stdout);
    }
}

void show_warning(const char* fmt, ...) {
    va_list args;

    fprintf(stderr, "%s: warning: ", progname);
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
    fflush(stdout);
}

/*
    Display an error and abort the program.
*/
void show_error(const char* fmt, ...) {
    va_list args;

    fprintf(stderr, "%s: error: %d: ", progname, line_number);
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
        
    memset(str, 0, sizeof(str));
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
    double num = 0.0;
    int var_val;

    if(!deparam_flag) {
        fputc('[', outfp);
        return;
    }

    expression_count++;
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

    if(!deparam_flag) {
        fputc('#', outfp);
        return;
    }

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
    Show the help text and exit.
*/
void show_help(void) {

    int i;

    for(i = 0; help_text[i] != NULL; i++)
        printf("%s\n", help_text[i]);
}

/*
    Parse the command line.
    Accepted parameters:
    -i input file name (required)
    -o output file name (required)
    -d deparameterize the input
    //-s swap the x and y axis
    -v verbose operation
    -h show help
    If neither -d or -s are present, then a simple copy is made.
*/
void cmd_line(int argc, char** argv) {

    int i, j;
    int found_inf = 0, found_outf = 0;

    strncpy(progname, argv[0], sizeof(progname));
    if(argc < 2) {
        strncpy(infn, "none", sizeof(infn));
        show_help();
        show_error("nothing to do!");
    }

    i = 1;
    j = 0;
    while(1) {
        if(i >= argc)
            break;

        switch(argv[i][j]) {
            case '-':
                j = 1;
                break; // continue and do nothing
            case 'v':
                verbose_flag++;
                j++;
                break;
            case 'd':
                deparam_flag++;
                j++;
                break;
            //case 's':
            //    //swap_flag++;
            //    show_warning("swapping is broken/disabled");
            //    j++;
            //    break;
            case 'h':
                show_help();
                exit(0);
                break;  // keep the compiler happy
            case 'i':
                i++;
                strncpy(infn, argv[i], sizeof(infn));
                j = 0;
                found_inf++;
                i++;
                break;
            case 'o':
                i++;
                strncpy(outfn, argv[i], sizeof(outfn));
                j = 0;
                found_outf++;
                i++;
                break;
            case 0: // end of this parameter
                j = 0; 
                i++;
                break;
            default:
                show_help();
                show_error("command line error at \'%c\'", argv[i][j]);
                break; // keep the compiler happy
        }
    }

    if(!found_inf || !found_outf) {
        show_help();
        show_error("both input and output files must be specified");
    }

    if(!deparam_flag && !swap_flag) {
        show_warning("no flags set; copy only");
    }
}

/*
    Main function. Handle file iopening and closing, etc.
*/
int main(int argc, char** argv) {

    cmd_line(argc, argv);

    memset(var_list, 0, sizeof(var_list));
    show_msg("opening input file: %s", infn);
    if(NULL == (infp = fopen(infn, "r"))) 
        show_error("cannot open input file: %s: %s\n", infn, strerror(errno));

    show_msg("opening output file: %s", outfn);
    if(NULL == (outfp = fopen(outfn, "w"))) 
        show_error("cannot open output file: %s: %s\n", outfn, strerror(errno));

    line_number++;
    fprintf(outfp, "(input file: %s processed by deparam on: %s)\n", infn, get_time());
    fprintf(outfp, "(de-paramaterize: %s)\n", (deparam_flag)? "YES":"NO");
    fprintf(outfp, "(swap x/y: %s)\n", (swap_flag)? "YES":"NO");

    while(1) {
        int ch = get_char();

        if(ch == EOF) {
            break;
        }

        switch(ch) {
            case '[':
                do_the_expression();
                break;
            case '#':
                do_the_definition();
                break;
            /*
            case 'X':
            case 'x':
                if(swap_flag)
                    fputc('Y', outfp);
                else
                    fputc('X', outfp);
                break;
            case 'Y':
            case 'y':
                if(swap_flag)
                    fputc('X', outfp);
                else
                    fputc('Y', outfp);
                break;
            */
            default:
                fputc(ch, outfp);
                break;
        }
    }

    fclose(infp);
    fclose(outfp);
    show_msg("%d lines, %d expressions", line_number-1, expression_count);
    return(0);
}

/*
    TODO: Swapping is broken. In order to get it working, it will be nessessary 
    to swap the objects on the line instead of just swapping the letters. 
    Apparently, at least for Camotics, the order in which the X and Y appear is
    significant.

    In order to get X/Y swapping to work, I am going to need to refactor most of 
    the code. Instead of loading it character-by-character, I will need to do it
    line-by-line and do all of the processing on the line before writing it out.
*/