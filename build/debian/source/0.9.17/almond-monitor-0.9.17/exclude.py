import os
import subprocess

def is_elf(file_path):
    """Check if a file is an ELF binary using the 'file' command."""
    try:
        output = subprocess.check_output(['file', '--brief', '--mime-type', file_path], text=True)
        return output.strip() == 'application/x-executable'
    except Exception:
        return False

def collect_non_elf_files(root_dir):
    """Traverse the package directory and collect non-ELF file names."""
    exclude_flags = []
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            full_path = os.path.join(dirpath, filename)
            if not is_elf(full_path):
                exclude_flags.append(f"-X{filename}")
    return exclude_flags

# Set the path to the package directory
package_dir = "debian/almond-monitor"

# Collect exclusion flags
exclusion_flags = collect_non_elf_files(package_dir)

# Generate the override_dh_strip line
override_line = f"override_dh_strip:\n\t" + "dh_strip " + " ".join(exclusion_flags)

# Output the result
print("Add the following to your debian/rules file:\n")
print(override_line)
