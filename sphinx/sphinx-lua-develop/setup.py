#!/usr/bin/env python

from setuptools import setup, find_packages

from version import get_git_version

try:
    import setuptools_git
except ImportError:
    print("WARNING!")
    print("We need the setuptools-git package to be installed for")
    print("some of the setup.py targets to work correctly.")

PACKAGE = 'sphinx-lua'
VERSION = get_git_version()

setup(
    name = PACKAGE,
    version = VERSION,
    package_dir = {'': 'src'},
    packages = find_packages('src'),
    namespace_packages = ['redjack', 'redjack.sphinx'],
    install_requires = ['Sphinx'],
)
