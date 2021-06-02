import sys
from pathlib import Path
from zipfile import ZipFile
from datetime import datetime


def package_files(base_name, file_lst):
    tpath = Path("bin")
    release_root = tpath / Path("release")
    if not release_root.exists():
        release_root.mkdir(exist_ok=True,parents=True)
    out_path = release_root / f"{base_name}_{datetime.today().strftime('%Y%m%d')}.zip"
    with ZipFile(out_path,'w') as zipObj:        
        for item in file_lst:
            item = tpath / Path(item)             
            zipObj.write(item,item.relative_to(tpath))

if __name__ == "__main__":
    base_name = sys.argv[1] 
    file_lst = list(sys.argv[2:])

    package_files(base_name, file_lst)
