#!/usr/bin/python

# This is a total hack to generate and upload the daily builds.
# Nothing about this file should be expected to be portable between
# machines.

# This script runs successfully on an Ubuntu 8.04 LTS system with
# (at least) the following packages installed:
#     python-dev
#     python2.4
#     python2.4-dev
#     mingw32
#     g++
#     libpng12-dev
#     libtiff4-dev
#     python-docutils
#     python-pygments
#     openssh-client
#     rsync

import datetime
import os
import shutil
import sys
from distutils.util import get_platform
import subprocess

import pysvn

ROOT_PATH = "/home/mdboom/JHU/builds/"
WORKING_PATH = "gamera-daily-build"
REPOS_PATH = "https://gamera.svn.sf.net/svnroot/gamera/trunk/gamera"
STAGING_PATH = "gamera-daily"
RSYNC_TARGET = "foo"
KEEP = 10

def mysystem(message, command):
    print message,
    try:
        process = subprocess.Popen(command, shell=True,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        retcode = process.wait()
        if retcode != 0:
            print "FAILED (%d)" % retcode
            print process.stderr.read()
            return
    except:
        print "FAILED"
    else:
        print "SUCCESS"

def myrmtree(path):
    if os.path.exists(path):
        shutil.rmtree(path)

def mymkdir(path):
    if not os.path.exists(path):
        os.mkdir(path)

def update_working_copy():
    client = pysvn.Client()

    push_update = False
    if os.path.exists(WORKING_PATH):
        current_working = client.info(WORKING_PATH)['revision'].number
        current_remote = 0
        for rev in client.update(WORKING_PATH):
            current_remote = max(current_remote, rev.number)
        if current_remote > current_working:
            print "Updates in SVN"
            push_update = True
    else:
        print "Checking out clean"
        client.checkout(REPOS_PATH, WORKING_PATH)
        current_working = client.info(WORKING_PATH)['revision'].number
        push_update = True
    return push_update, current_working

def get_version(now, current_working):
    return "daily-%s-SVN-r%s" % (now, current_working)

def update_version(version):
    fd = open(os.path.join(WORKING_PATH, "version"), "w")
    fd.write(version)
    fd.close()

def build(version):
    build_dir = os.path.abspath(os.path.join(WORKING_PATH, "build", "lib.%s-%s" % (get_platform(), sys.version[0:3])))
    if "--clean" in sys.argv:
        myrmtree(os.path.join(WORKING_PATH, "build"))
        myrmtree(os.path.join(WORKING_PATH, "dist"))
    myrmtree(os.path.join(WORKING_PATH, "doc/html"))
    os.chdir(WORKING_PATH)
    mysystem("Building for Linux (Python 2.5)...", "python setup.py build")
    mysystem("Building for Linux (Python 2.4)...", "python2.4 setup.py build")
    mysystem("Building source distribution...", "python setup.py sdist")
    mysystem("Building for Windows...", "python setup.py build --compiler=mingw32_cross bdist_wininst")
    os.chdir("doc")
    mysystem("Building docs...", "export PYTHONPATH=%s:$PYTHONPATH; python gendoc.py" % build_dir)
    os.rename("html", "gamera-doc-%s" % version)
    mysystem("tar-gzipping docs...", "tar czvf gamera-doc-%s.tar.gz gamera-doc-%s" % (version, version))
    mysystem("zipping docs...", "zip -r gamera-doc-%s.zip gamera-doc-%s" % (version, version))
    os.rename("gamera-doc-%s" % version, "html")
    os.chdir("../..")

def rotate(path, number=2):
    files = []
    for f in os.listdir(path):
        p = os.path.join(path, f)
        files.append((os.stat(p).st_mtime, p))
    if len(files) < KEEP * number:
        return
    files.sort()
    files.revert()
    remove = files[KEEP * number:]
    for t, f in remove:
        os.path.remove(f)

def stage(version):
    mymkdir(os.path.join(STAGING_PATH, "src"))
    mymkdir(os.path.join(STAGING_PATH, "win32"))
    mymkdir(os.path.join(STAGING_PATH, "doc"))
    shutil.copy2(os.path.join(WORKING_PATH, "dist", "gamera-%s.tar.gz" % version),
                 os.path.join(STAGING_PATH, "src"))
    shutil.copy2(os.path.join(WORKING_PATH, "dist", "gamera-%s.win32-py2.5.exe" % version),
                 os.path.join(STAGING_PATH, "win32"))
    shutil.copy2(os.path.join(WORKING_PATH, "doc", "gamera-doc-%s.tar.gz" % version),
                 os.path.join(STAGING_PATH, "doc"))
    shutil.copy2(os.path.join(WORKING_PATH, "doc", "gamera-doc-%s.zip" % version),
                 os.path.join(STAGING_PATH, "doc"))
    rotate(os.path.join(STAGING_PATH, "src"))
    rotate(os.path.join(STAGING_PATH, "win32"))
    rotate(os.path.join(STAGING_PATH, "doc"), 2)

def rsync():
    mysystem("rsync -e ssh -arvz --delete %s %s" % (STAGING_PATH, RSYNC_TARGET))

def main():
    os.chdir(ROOT_PATH)
    now = datetime.datetime.now().strftime("%Y%m%d")
    mymkdir(STAGING_PATH)
    log_file = open(os.path.join(STAGING_PATH, "daily-build.log"), "a")
    sys.stdout = log_file
    sys.stderr = log_file
    print "-" * 76
    print "RUNNING:", now
    push_update, current_working = update_working_copy()
    if '--force' in sys.argv:
        push_update = True
    if not push_update:
        print "No updates today."
        return
    version = get_version(now, current_working)
    update_version(version)
    build(version)
    stage(version)
    # rsync()

if __name__ == '__main__':
    main()
