#!/usr/bin/env python3

from urllib.request import urlopen, urlretrieve
from urllib.parse import urlparse
from gzip import GzipFile
from pathlib import Path
import subprocess 
import tarfile
import pickle
import os

# WARNING: this script does not verify integrity and signature of packages!

RASPBIAN_VERSION = "stretch"
RASPBIAN_ARCHIVE = "http://archive.raspberrypi.org/debian"
RASPBIAN_MAIN = "http://raspbian.raspberrypi.org/raspbian"

ALARM = "http://mirror.archlinuxarm.org"
ALARM_REPOS = ["alarm", "core", "extra", "community"]

IGNORED_PACKAGES = [
  "raspberrypi-bootloader", "libasan3", "libubsan0", "libgomp1", "libatomic1",
  "sh", "filesystem", "tzdata", "iana-etc",
]

# db is dict from package name to two element list - url and list of dependencies
def raspbian_collect_packages(url, version, db):
  with urlopen(f"{url}/dists/{version}/main/binary-armhf/Packages.gz") as req:
    with GzipFile(fileobj=req) as gz:
      for line in gz:
        line = line.strip().decode("utf-8")
        if ": " not in line:
          continue
        key, value = line.split(": ", 1)
        if key == "Package":
          pkg = [url, []]
          db[value] = pkg
        elif key == "Depends":
          deps = value.split(", ")
          pkg[1] += [dep.split(" (")[0].split(" | ")[0] for dep in deps]
        elif key == "Filename":
          pkg[0] += "/" + value


def alarm_collect_packages(arch, repo, db):
  base = f"{ALARM}/{arch}/{repo}"
  with urlopen(f"{base}/{repo}.db.tar.gz") as req:
    with tarfile.open(fileobj=req, mode="r:gz") as tar:
      name = None
      url = None
      depends = []
      processed = 0
      for info in tar:
        if not info.isfile():
          continue

        p = Path(info.name)
        if p.stem == "desc":
          next_name = False
          next_url = False
          for line in tar.extractfile(info):
            line = line.strip().decode("utf-8")
            if next_url:
              url = f"{base}/{line}"
              next_url = False
            elif next_name:
              name = line
              next_name = False
            elif line == "%FILENAME%":
              next_url = True
            elif line == "%NAME%":
              next_name = True
          processed += 1

        elif p.stem == "depends":
          next_depends = False
          for line in tar.extractfile(info):
            line = line.strip().decode("utf-8")
            if next_depends:
              if line:
                depends.append(line.split("<")[0].split(">")[0].split("=")[0])
              else:
                next_depends = False
                break
            elif line == "%DEPENDS%":
              next_depends = True
          processed += 1

        if processed == 2:
          processed = 0
          db[name] = [url, depends]
          name = None
          url = None
          depends = []


def resolve(package, packages, db):
  if package in packages:
    return
  packages.add(package)
  for dep in db[package][1]:
    if dep not in IGNORED_PACKAGES:
      resolve(dep, packages, db)


def install(distro, version, target, sysroot, packages):

  sysroot = Path(sysroot)
  cache = sysroot / ".db.pickle"

  if cache.is_file():
    print("Loading package database...")
    
    with open(cache, "rb") as f:
      db = pickle.load(f)

    if distro == "alarm":
      print("WARNING: ArchLinux package database can be out-of-date pretty quickly, remove db if downloading fails")

  else:
    print("Downloading package database...")
  
    db = {}

    if distro == "raspbian":
      raspbian_collect_packages(RASPBIAN_ARCHIVE, version, db)
      raspbian_collect_packages(RASPBIAN_MAIN, version, db)

    elif distro == "alarm":
      if target is None:
        raise Exception("ALARM target not specified (use --target argument)")
      elif target.startswith("armv6"):
        arch = "armv6h"
      elif target.startswith("armv7"):
        arch = "armv7h"
      elif target.startswith("aarch64"):
        arch = "aarch64"
      else:
        raise Exception(f"Unsupported ALARM target {target}")

      for repo in ALARM_REPOS:
        alarm_collect_packages(arch, repo, db)
     
    with open(cache, "wb") as f:
      pickle.dump(db, f, pickle.HIGHEST_PROTOCOL)

  print("Resolving dependencies...")

  process = set()
  for pkg in packages:
    resolve(pkg, process, db)

  print("Downloading...")

  for i, pkg in enumerate(process):
    print(f"({i+1}/{len(process)}) {pkg}")
    url = db[pkg][0]
    name = sysroot / Path(urlparse(url).path).name

    if not name.is_file():
      urlretrieve(url, name)

      if distro == "raspbian":
        subprocess.check_call(["dpkg-deb", "-x", name, sysroot])

      elif distro == "alarm":
        subprocess.check_call(["tar", "--force-local", "-C", sysroot, "-xJf", name], stderr=subprocess.DEVNULL)

  print("Fixing symlinks...")

  for p in sysroot.glob("**/*"):
    if p.is_symlink():
      link = os.readlink(p)
      if Path(link).is_absolute():
        full = sysroot / ("." + link)
        fixed = os.path.relpath(full.resolve(), p.parent)
        p.unlink()
        p.symlink_to(fixed)

  print("Done!")


if __name__ == "__main__":
  from argparse import ArgumentParser

  ap = ArgumentParser(description="Download and install Raspbian packages to sysroot")
  ap.add_argument("--distro", required=True, choices=["raspbian", "alarm"], help="distribution to use")
  ap.add_argument("--version", default=RASPBIAN_VERSION, help=f"distribution version to use for raspbian (default: {RASPBIAN_VERSION})")
  ap.add_argument("--target", help="target to download for alarm (ex: armv6l-unknown-linux-gnueabihf)")
  ap.add_argument("--sysroot", required=True, help="sysroot folder")
  ap.add_argument("packages", nargs="+")
  args = ap.parse_args()

  os.makedirs(args.sysroot, exist_ok=True)
  install(args.distro, args.version, args.target, args.sysroot, args.packages)
