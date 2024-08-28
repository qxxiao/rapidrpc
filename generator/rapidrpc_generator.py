

import getopt
import sys
import os
import trace

from string import Template

project_name = ""
proto_file = ""
out_project_path = "./"

# main function
def generate_project():
    try:
        parseInput()

        print('='*150)
        print('Generating project...')

        generate_dir()

        generate_pb()

    except Exception as e:
        print("Failed to generate rapid rpc project: {}" .format(e))
        trace.print_exc()

        print('='*150)

def parseInput():
    opts, args = getopt.getopt(sys.argv[1:], "hi:o:", longopts=["help", "input=", "output="])
    for opts, arg in opts:
        if opts in ("-h", "--help"):
            print("Usage: python rapidrpc_generator.py -i <input> -o <output>")
            sys.exit(0)
        elif opts in ("-i", "--input"):
            global proto_file
            proto_file = arg
        elif opts in ("-o", "--output"):
            global out_project_path
            out_project_path = arg
            if out_project_path[-1] != '/':
                out_project_path += '/'
        else:
            raise Exception("Invalid option: [{}:{}]" .format(opts, arg))

    if not os.path.exists(proto_file):
        raise Exception("Generate error, proto file not found: {}" .format(proto_file))

    if ".proto" not in proto_file:
        raise Exception("Generate error, invalid proto file: {}" .format(proto_file))
    
    global project_name
    project_name = proto_file.split('/')[-1].replace(".proto", "")
    print("Project name: {}" .format(project_name))


def generate_dir():
    print("="*100)
    print("Generating project directory...")

    if out_project_path == "":
        


if __name__ == '__main__':
    generate_project()