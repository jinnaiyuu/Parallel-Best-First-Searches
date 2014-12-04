#!/usr/bin/python
# make_tiles.py
#
# Takes a series of single-line tiles boards in the format produced by gen.c
# and converts them to our format.
#
# sna4 December 2008

import sys, shutil, fileinput, math, os, subprocess, hashlib

def usage():
    print "usage: make_tiles.py [-t] [MAX] [weight]"
    print "-t test whether A* can solve boards and keep only solvable ones"
    print "runs A* with weight if provided"
    print "outputs MAX number of boards to files named by MD5 of their content"
    print "provide tile boards on stdin in the following format:"
    print "\t[num] [tile1, tile2...]"
    print "assumes board is square or one off from being square"
    print "ignores the first line, since gen.c gives a pre-solved board first"
    sys.exit(1)

max_exp = 3500000
width, height, n = None, None, None
nblocks, threads = "2", "8"
dir, model, executable = "~/aifs2/group/data/tiles_instances/", "snlemons_easy", "~/aifs2/eaburns/src/ocaml/rdb/rdb_get_path.unix_unknown"
search_exec = "~/aifs2/eaburns/src/cpp-search/src/tiles_search.x86_64.bin"
#search_exec = "../src/tiles_search.i386"
ulimit = "ulimit -v 15000000"

def switch_rep(tiles):
    other = [0]*len(tiles)
    tiles = [int(tile) for tile in tiles]
    for tile in tiles:
        other[tile] = str(tiles.index(tile))
    return other

def make_board(in_data, test, weight):
    global width, height, n
    in_data = in_data.split()
    num = in_data[0]
    tiles = switch_rep(in_data[1:])
    if n == None:
        n = len(tiles)
        height = math.sqrt(n)
        if height != int(height):
            height = int(height)
            width = height+1
        else:
            height = int(height)
            width = height
        width, height = str(width), str(height)
    tiles = "\n".join(tiles)
    path = num+"_"+width+"x"+height+".tile"
    outfile = open(path, "w")
    outfile.write(width+" "+height+"\n")
    outfile.write("starting positions for each tile:\n")
    outfile.write(tiles+"\n")
    outfile.write("goal positions:\n")
    outfile.write("\n".join([str(x) for x in range(n)]))
    outfile.close()
    
#    m = hashlib.md5()
#    m.update(open(path).read())
#    new_path=subprocess.Popen(executable+" "+dir+" model="+model+" rows="+height+" cols="+width+" hash="+m.hexdigest(), shell=True, stdout=subprocess.PIPE, executable="/bin/bash").stdout.readlines()[-1].split()[-1]
    #new_path = m.hexdigest()+".tile"

#    if os.path.exists(new_path):
#        os.remove(path)
#        return False

#    shutil.move(path, new_path)
#    path = new_path

    if test:
        #run A* or wA* to see if the board is solvable
        if weight > 1:
            weight = str(weight)
            algs = ["wastar-"+str(weight)]
#             algs += ["pbnf-"+weight+"-64-"+threads+"-"+nblocks,
#                      "safepbnf-"+weight+"-64-"+threads+"-"+nblocks,
#                      "wprastar-"+weight+"-"+threads,
#                      "waprastar-"+weight+"-"+threads+"-"+nblocks,
#                      "whdastar-"+weight+"-"+threads,
#                      "wahdastar-"+weight+"-"+threads+"-"+nblocks]
        else:
            algs = ["astar"]
#             algs += ["pbnf-64-"+threads+"-"+nblocks,
#                      "safepbnf-64-"+threads+"-"+nblocks,
#                      "prastar-"+threads,
#                      "aprastar-"+threads+"-"+nblocks,
#                      "hdastar-"+threads,
#                      "ahdastar-"+threads+"-"+nblocks]
        for alg in algs:
            cmd = ulimit+"; "+search_exec+" "+alg+" < "+path
            results = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, executable="/bin/bash").stdout.readlines()
            #print results
            is_output = len(results) > 0
            if is_output:
                finished = "cost:" in "".join(results)
                solved = not "No Solution" in results[0]
            else:
                finished = False
            #if not, delete the board and return false
            if not finished:
                if is_output and not solved:
                    print results
                    print alg, "found no solution!"
                else:
                    print alg, "failed to finish"
                os.remove(path)
                return False
            else:
                print alg, "finished"
                for line in results:
                    if "expan" in line or "Expan" in line:
                        exp = int(line[line.index(":")+2:])
                if exp > max_exp:
                    print "but too many nodes were expanded!"
                    os.remove(path)
                    return False
                #cost = results[0].split()[1]
                #gen = results[-1].split()[1]
                #print "finished with cost", cost, "generated", gen
    
    return True

if __name__ == '__main__':
    if "--usage" in sys.argv or "--help" in sys.argv:
        usage()
    else:
        if "-t" in sys.argv:
            test = True
            sys.argv.remove("-t")
        else:
            test = False
        if len(sys.argv) > 1:
            max = int(sys.argv[1])
        else:
            max = sys.maxint-2
        if len(sys.argv) > 2:
            weight = float(sys.argv[2])
        else:
            weight = 1
        
        sys.argv = []
        made = 0
        for line in fileinput.input():
            made += 1
            if made > max:
                sys.exit(0)
            print "trying to make board #"+str(made)
            if not make_board(line, test, weight):
                made -= 1
