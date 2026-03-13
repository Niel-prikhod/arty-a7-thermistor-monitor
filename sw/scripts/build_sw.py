import vitis
import shutil
import os

# Setup paths relative to the script location
script_path = os.path.abspath(__file__)
repo_root = os.path.dirname(os.path.dirname(os.path.dirname(script_path)))
xsa_path = os.path.join(repo_root, "hw", "prebuilt", "system.xsa")
workspace = os.path.join(repo_root, "sw/vitis_workspace")

# 1. Create a Vitis client
client = vitis.create_client()
client.set_workspace(path=workspace)

def force_clean_component(name):
    """
    Deletes a component if it already exists in the workspace;
    Pass, if there is no component found.
    """
    try:
        comp = client.get_component(name=name)
        if comp:
            print(f"Found existing component '{name}'. Deleting for fresh build...")
            client.delete_component(name=name)
    except:
        pass
app_name = "thermistor_app"
platform_name = "arty_platform"
force_clean_component(platform_name)

# 2. Create the Platform Component
platform = client.create_platform_component(
    name=platform_name,
    hw_design=xsa_path,
    os="standalone",
    cpu="microblaze_0"
)

# 3. Create the Application Component
app = client.create_app_component(
    name=app_name,
    platform=os.path.join(workspace, "arty_platform", "export", "arty_platform", "arty_platform.xpfm"),
    domain="standalone_microblaze_0"
)

# 4. Import your custom C code
app.import_files(
    from_loc=os.path.join(repo_root, "sw", "src"),
    files=["main.c"],
    dest_dir="src"
)

# 5. Build the components
platform.build()
app.build()

print("Vitis build complete! ELF file is ready.")

elf_src = os.path.join(workspace, app_name, "build", f"{app_name}.elf")
elf_dst = os.path.join(repo_root, "sw/prebuilt/thermistor.elf")

if os.path.exists(elf_src):
    shutil.copy(elf_src, elf_dst)
    print(f"Successfully copied {app_name}.elf to {elf_dst}")
else:
    print(f"Error: Could not find ELF at {elf_src}")
