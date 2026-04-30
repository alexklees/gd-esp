import os
import shutil
Import("env")

def copy_assets(*args, **kwargs):
    print("Copying assets to build directory...")
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    
    # Assets to copy
    assets = ["bgm.mp3", "cat.png"]
    src_dir = os.path.join(project_dir, "src")
    
    # Add all .json files from src
    if os.path.exists(src_dir):
        for f in os.listdir(src_dir):
            if f.endswith(".json"):
                assets.append(f)

    for asset in assets:
        src = os.path.join(src_dir, asset)
        dst = os.path.join(build_dir, asset)
        if os.path.exists(src):
            try:
                shutil.copy(src, dst)
                print(f"Copied {src} to {dst}")
            except Exception as e:
                print(f"Failed to copy {src} to {dst}: {e}")
        else:
            print(f"Asset {src} not found!")

# Add post-action to the program
env.AddPostAction("$BUILD_DIR/${PROGNAME}", copy_assets)
if "PROGPATH" in env:
    env.AddPostAction("$PROGPATH", copy_assets)
