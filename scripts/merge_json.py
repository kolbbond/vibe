import json
import os
import shutil

KNRM = '\033[0m'
KCYN = '\033[0;36m'
print(f"{KCYN}--- Merging compile_commands.json ---{KNRM}")

# directories to search through
# @hey: add a check for which directory we are in
mydirs = ["../.deps/build/src", "."]

# backup current compile_commands.json
if not os.path.exists("compile_commands2.json") and os.path.exists("compile_commands.json"):
    shutil.copy("compile_commands.json", "compile_commands2.json")

if os.path.exists("compile_commands2.json"):
    shutil.copy("compile_commands2.json", "compile_commands.json")

merged_data = []

debug = False

# walk over directories in dependencies
for mydir in mydirs:
    if os.path.exists(mydir):
        for root, dirs, files in os.walk(mydir):
            if debug:
                print(f"Checking directory: {root}")
                print(f"Directories: {dirs}")
                print(f"Files: {files}")

            if 'compile_commands.json' in files:
                print(f"Found compile_commands.json in {root}")
                compile_commands_path = os.path.join(
                    root, 'compile_commands.json')
                with open(compile_commands_path, 'r') as f:
                    data = json.load(f)
                    merged_data.extend(data)
                    # Process or print the data as needed
                    # print(f"Found compile_commands.json in {root}: {data}")

print('')

# print current directory
print(f"Current directory: {os.getcwd()}")

# print(f"Merged {len(merged_data)} entries from compile_commands.json files.")
# dump output file
output_file = 'compile_commands.json'
with open(output_file, 'w') as f:
    json.dump(merged_data, f, indent=4)

# need an end
print(f"{KCYN}--- Wrote {output_file} with {len(merged_data)} entries ---{KNRM}")
