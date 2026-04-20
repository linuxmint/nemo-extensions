#!/usr/bin/python3

from setuptools import setup

setup(
    name='nemo-tags',
    version='1.0.0',
    description='A powerful tagging system for Nemo File Manager',
    author='DIMFLIX',
    author_email='dimflix.official@gmail.com',
    url='https://github.com/meowrch/nemo-tags',
    license='GPL-3.0',
    packages=[],
    data_files=[
        ('/usr/share/nemo-python/extensions', ['nemo-extension/nemo-tags.py']),
    ]
)
