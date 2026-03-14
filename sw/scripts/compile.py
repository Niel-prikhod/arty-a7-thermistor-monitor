import vitis
import os
import sys

script_path = os.path.abspath(__file__)
repo_root = os.path.dirname(os.path.dirname(os.path.dirname(script_path)))
ws_path = os.path.join(repo_root, "sw/vitis_workspace")
app_name = "thermistor_app"

# src_dir = os.path.join(repo_root, "sw/src")
# if len(sys.argv) > 1:
#     source_files = sys.argv[1:]          
# else:
#     source_files = ["*.c"]

client = vitis.create_client()
client.set_workspace(path=ws_path)
app = client.get_component(name=app_name)
# app.import_files(
#         from_loc=src_dir, 
#         files=["main.c"],
#         is_skip_copy_sources=True
#         )
app.build()
