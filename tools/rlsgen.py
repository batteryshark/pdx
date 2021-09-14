# A quick release generator script using GithubCLI: https://github.com/cli/cli/
import os
import sys
import requests
from pathlib import Path
from zipfile import ZipFile
from datetime import datetime

def find_available_tag(repo_release_url):
    r = requests.get(repo_release_url)
    if(r.status_code != 200):
        return ""
    
    data = r.json()
    tags = []
    for release in data:
        tags.append(release['tag_name'])
    rev = 0
    while 1:
        tag_name = f"{datetime.today().strftime('%Y%m%d')}.{rev}"
        if tag_name not in tags:
            return tag_name
        rev +=1
    
def create_release(username, repository_name, path_to_zip):
    repo_release_url = f"https://api.github.com/repos/{username}/{repository_name}/releases"
    available_tag = find_available_tag(repo_release_url)
    cmd = f"gh release create \"{available_tag}\" --title \"Release\" --notes \"Binaries\" \"{path_to_zip}\""

    os.system(cmd)
    
def package_files(base_name, file_lst):
    tpath = Path("bin")
    release_root = tpath / Path("release")
    if not release_root.exists():
        release_root.mkdir(exist_ok=True,parents=True)
    out_path = release_root / f"{base_name}.zip"
    with ZipFile(out_path,'w') as zipObj:        
        for item in file_lst:
            item = tpath / Path(item)             
            zipObj.write(item,item.relative_to(tpath))
    return out_path

if __name__ == "__main__":
    username = sys.argv[1]
    repository_name = sys.argv[2]
    base_name = sys.argv[3] 
    file_lst = list(sys.argv[4:])

    path_to_zip = package_files(base_name, file_lst)
    create_release(username,repository_name, path_to_zip)
    print("Done!")
