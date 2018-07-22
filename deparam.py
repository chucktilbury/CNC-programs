
import sys
import re
import argparse
import datetime

def save_block(ofp, block):
    for item in block:
        ofp.write(item)
        ofp.write(' ')
    ofp.write('\n')

def store_var(ofp, s):
    if args.dep_flag:
        r = re.findall(r'(#[0-9]+)\s*=\s*([-+]?\d+\.?\d*)\s*\((.+)\)', s)
        if len(r) == 0:
            sys.stderr.write("error: cannot parse input file var\n")
            sys.stderr.write(s+'\n')
            sys.exit(1)

        var_table[r[0][0]] = float(r[0][1])
        ofp.write("(%s = %s [%s])\n"%(r[0][2], r[0][1], r[0][0]))
    else:
        ofp.write(s+'\n')

def deparam_block(s):
    r = re.findall(r'\[(([-+]?\d+\.?\d*)|#\d{1,2})\s*\*?(\s*#\d{1,2})?\s*\+?(\s*#\d{1,2})?\]', s)
    if len(r) == 0:
        return s

    num1 = 0.0
    num2 = 0.0
    num3 = 0.0
    t = r[0]
    term1 = t[0]
    term2 = t[1]
    term3 = t[2]
    term4 = t[3]

    if len(term1) == 0:
        # always an error; corrupt input
        sys.stderr.write("error: cannot parse input file expression (2)\n")
        sys.stderr.write(str(r)+'\n')
        sys.exit(1)
    elif term1[0] == '#':
        # term1 is a var
        num1 = var_table[term1]
        return str(num1)
    elif len(term2) == 0:
        # error if term1 is not a var
        sys.stderr.write("error: cannot parse input file expression (2)\n")
        sys.stderr.write(str(r)+'\n')
        sys.exit(1)
    else:
        num1 = float(term2)

    if len(term3) != 0:
        num2 = var_table[term3]

    if len(term4) != 0:
        num3 = var_table[term4]

    v = num1*num2+num3
    return str(v)

def do_g_features(ofp, s):

    nblk = []    
    s = s.replace('[', ' [')
    s = re.sub(r' +', r' ', s)
    block = s.split(' ')

    if args.feature:
        if args.dep_flag:
            for item in block:
                if item[0] == '[':
                    #num_expers += 1
                    b = deparam_block(item)
                    nblk.append(b)
                else:
                    nblk.append(item)

        if args.swap_flag and len(block) > 4:
            nblk[4], nblk[2] = nblk[2], nblk[4]
    else:
        nblk = block

    save_block(ofp, nblk)

def process_line(ofp, line):
    #num_lines = num_lines + 1
    l = line.rstrip('\n').upper()
    if len(l) > 0:
        if l[0] == '#':
            #num_vars += 1
            store_var(ofp, l)
        elif l[0] == 'G':
            do_g_features(ofp, l)
        else:
            ofp.write(line+'\n')
    else:
        ofp.write('\n')

def cmd_line():

    parser = argparse.ArgumentParser(description="post-process cnc file")
    parser.add_argument('-i', dest='infile', help="input file", required=True)
    parser.add_argument('-o', dest='outfile', help="output file", required=True)
    #parser.add_argument('-v', dest='verbose', action='store_true', help="verbose flag")
    parser.add_argument('-d', dest='dep_flag', action='store_true', help="deparam flag")
    parser.add_argument('-s', dest='swap_flag', action='store_true', help="swap flag")
    args = parser.parse_args();

    if args.dep_flag or args.swap_flag:
        args.feature = True
    else:
        args.feature = False

    return args;



def process_line_buffer(ofname, l_buffer):
    with open(ofname, "w") as ofp:
        ofp.write("(post-processed by deparam.py)\n")
        ofp.write("(input file name: %s)\n"%(args.infile))
        ofp.write("(output file name: %s)\n"%(args.outfile))
        ofp.write("(date: %s)\n"%(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")))
        ofp.write("(deparam: %s)\n"%("TRUE" if args.dep_flag else "FALSE"))
        ofp.write("(swapping: %s)\n"%("TRUE" if args.swap_flag else "FALSE"))
        for line in l_buffer:
            process_line(ofp, line)

def read_file(ifname, ofname):
    with open(ifname) as ifp:
        line_buffer = ifp.readlines()
    process_line_buffer(ofname, line_buffer)



var_table = {}
num_vars = 0
num_expers = 0
num_lines = 0

args = cmd_line()
read_file(args.infile, args.outfile)
#if args.verbose:
#    print "lines:", num_lines, "vars:", num_vars, "expressions:", num_exprs
#print var_table