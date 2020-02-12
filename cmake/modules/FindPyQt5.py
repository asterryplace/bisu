# Copyright (c) 2014, Simon Edwards <simon@simonzone.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

import sys
import os

try:
    # On Windows and Python 3.8+ python doesn't load module DLL's
    # from the current PATH environment, therefore we should add
    # path to Qt's DLL's manually. This variable is passed from
    # FindPyQt5.cmake
    os.add_dll_directory(os.environ['PYTHONDLLPATH'])
except:
    pass

import PyQt5.QtCore

print("pyqt_version:%06.0x" % PyQt5.QtCore.PYQT_VERSION)
print("pyqt_version_str:%s" % PyQt5.QtCore.PYQT_VERSION_STR)

pyqt_version_tag = ""
in_t = False
pyqt_config_list = PyQt5.QtCore.PYQT_CONFIGURATION["sip_flags"].split(' ')
for item in pyqt_config_list:
    if item == "-t":
        in_t = True
    elif in_t:
        if item.startswith("Qt_5"):
            pyqt_version_tag = item
    else:
        in_t = False
print("pyqt_version_tag:%s" % pyqt_version_tag)

try:
    index_n = pyqt_config_list.index('-n')
    pyqt_sip_name = '-n' + pyqt_config_list[index_n + 1]
    print("pyqt_sip_name:%s" % pyqt_sip_name)
except ValueError:
    pass

# FIXME This next line is just a little bit too crude.
pyqt_sip_dir = os.path.join(sys.prefix, "share", "sip", "PyQt5")
print("pyqt_sip_dir:%s" % pyqt_sip_dir)

print("pyqt_sip_flags:%s" % PyQt5.QtCore.PYQT_CONFIGURATION["sip_flags"])
