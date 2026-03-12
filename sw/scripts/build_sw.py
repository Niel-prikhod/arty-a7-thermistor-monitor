import vitis
import os

# Setup paths relative to the script location
script_path = os.path.abspath(__file__)
repo_root = os.path.dirname(os.path.dirname(os.path.dirname(script_path)))
xsa_path = os.path.join(repo_root, "hw", "prebuilt", "system.xsa")
workspace = os.path.join(repo_root, "vitis_workspace")

# 1. Create a Vitis client
client = vitis.create_client()
client.set_workspace(path=workspace)

# 2. Create the Platform Component
platform = client.create_platform_component(
    name="arty_platform",
    hw_design=xsa_path,
    os="standalone",
    cpu="microblaze_0"
)

# 3. Create the Application Component
app = client.create_app_component(
    name="thermistor_app",
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
