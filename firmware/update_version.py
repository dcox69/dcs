import os
from datetime import datetime

# Compute absolute path of the script (project root folder)
project_dir = os.path.dirname(os.path.abspath(__file__))
file_path = os.path.join(project_dir, 'version.txt')

build_count = 0

# Read the existing file 
if os.path.exists(file_path):
    with open(file_path, 'r') as f:
        content = f.read().strip()
        parts = content.split('.')
        if len(parts) == 4 and parts[3].isdigit():
            build_count = int(parts[3])

# Increment build count and format string 
build_count += 1
date_str = datetime.now().strftime("%y.%m.%d")
new_version = f"{date_str}.{build_count:04d}"

# Save to the project root folder 
with open(file_path, 'w') as f:
    f.write(new_version)

print(f"--> Version updated : {new_version}")