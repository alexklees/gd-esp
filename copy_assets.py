import os
import shutil
Import("env")

def copy_assets(*args, **kwargs):
    print("Copying assets to build and data directories...")
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    data_dir = os.path.join(project_dir, "data")
    assets_dir = os.path.join(project_dir, "assets")
    
    # Check if this is an ESP32 build
    pio_env = env.subst("$PIOENV")
    is_esp32 = "m5stack" in pio_env

    if os.path.exists(data_dir):
        shutil.rmtree(data_dir)
    os.makedirs(data_dir)

    assets_to_copy = [] # list of (full_src_path, relative_path)
    total_size = 0
    if os.path.exists(assets_dir):
        for root, dirs, files in os.walk(assets_dir):
            for f in files:
                if f.endswith((".mp3", ".png", ".json")):
                    src_path = os.path.join(root, f)
                    rel_path = os.path.relpath(src_path, assets_dir)
                    
                    # If ESP32, only allow bgm.mp3 for audio files
                    if is_esp32 and rel_path.startswith("audio") and f != "bgm.mp3":
                        continue
                        
                    assets_to_copy.append((src_path, rel_path))
                    total_size += os.path.getsize(src_path)

    print(f"Total asset size: {total_size / 1024 / 1024:.2f} MB")
    if total_size > 4 * 1024 * 1024:
        print("WARNING: Total asset size exceeds 4MB! LittleFS upload might fail.")

    for src, rel_path in assets_to_copy:
        # Copy to build dir (for emulator)
        dst_build = os.path.join(build_dir, rel_path)
        os.makedirs(os.path.dirname(dst_build), exist_ok=True)
        # Copy to data dir (for uploadfs)
        dst_data = os.path.join(data_dir, rel_path)
        os.makedirs(os.path.dirname(dst_data), exist_ok=True)
        
        try:
            shutil.copy(src, dst_build)
            shutil.copy(src, dst_data)
        except Exception as e:
            print(f"Failed to copy {rel_path}: {e}")
    
    print(f"Sync complete. {len(assets_to_copy)} assets copied to data/ and build/")

# Add post-action to the program
env.AddPostAction("$BUILD_DIR/${PROGNAME}", copy_assets)
if "PROGPATH" in env:
    env.AddPostAction("$PROGPATH", copy_assets)
