#!/usr/bin/python3

from setuptools import setup


setup(
    packages=[],
    name="nemo-action-bar",
    version="2.0.0",
    description="A configurable GTK action bar for the Nemo file manager",
    author="Claudiu Schuster",
    author_email="info@claudiuschuster.de",
    url="https://github.com/oss-singularity/nemo-action-bar",
    license="GPL-2+",
    data_files=[
        (
            "/usr/share/nemo-python/extensions",
            ["src/nemo_action_bar.py"],
        ),
        ("/usr/share/nemo-action-bar", ["src/buttons.json"]),
        (
            "/usr/share/nemo-action-bar/icons",
            ["src/icons/nemo-action-bar-duplicate-symbolic.svg"],
        ),
    ],
)
