#!/usr/bin/python3
# python3 compilerTestItr8r.pyi -d ../CompileDriver/testCSrcFiles/ -x ../CompileDriver/build/CompileDriver

import subprocess
import sys
from os import walk


# total arguments
numArgs = len(sys.argv)
print("Total arguments passed:", numArgs)

# Arguments passed
print("\nName of Python script:", sys.argv[0])

print("\nArguments passed:", end = " ")
for i in range(1, numArgs):
    print(sys.argv[i], end = " ")

print ("\n****************************************")

src_directory = ""
executable = ""
idx = 1
isFailed = False
while idx < numArgs and not isFailed:
    flag = sys.argv[idx]
    if flag == "-d" or flag == "--directory":
        idx += 1
        if idx >= numArgs:
            isFailed = True
        else:
            src_directory = sys.argv[idx]

    elif flag == "-x" or flag == "--executable":
        idx += 1
        if idx >= numArgs:
            isFailed = True
        else:
            executable = sys.argv[idx]

    # Increment loop counter
    idx += 1

if src_directory == "" or executable == "":
    isFailed = True

if not isFailed:
    file_nombres = []
    for (path, directories, filenames) in walk(src_directory):
        file_nombres.extend(filenames)
        break

    for src_file_name in file_nombres:
        if src_file_name.endswith(".c"):
            print ("\nWorking on " + src_file_name, end=" ")
            src_file_path = src_directory + src_file_name
            before_dot = src_file_name.removesuffix(".c")
            outfile_path = src_directory + before_dot + ".out"
            verbosity_lvl = "SILENT"
            if src_file_name.startswith ("illustrate_"):
                # Get effusive. Copy original C src file to further improve readability
                verbosity_lvl = "ILLUSTRATIVE"
                process_ret = subprocess.run(["cp", src_file_path, outfile_path], capture_output=True)
                if process_ret.returncode != 0:
                    isFailed = True
                else:
                    with open(outfile_path, "a") as outfile:
                        outfile.write("\n\n^^^^^^^^^^ Contents of " + src_file_name + " included above for reference ^^^^^^^^^^\n")
                        outfile.close()


            else:
                process_ret = subprocess.run (["rm", outfile_path])
                # if process_ret.returncode != 0:
                #    isFailed = True

            if not isFailed:
                process_ret = subprocess.run([executable, src_file_path, "-l", verbosity_lvl], capture_output=True)
                with open(outfile_path, "ab") as outfile:
                    outfile.write(process_ret.stdout)

# Be a good citizen - make sure next app output starts on new line
print ("\n")