"""Setup module

For more information, see:
https://packaging.python.org/en/latest/distributing.html
https://github.com/pypa/sampleproject
"""

import os

from setuptools import setup, find_packages
from codecs import open as codecs_open


here = os.path.abspath(os.path.dirname(__file__))

# Read package information from atlasplots/__version__.py
about = {}
with open(os.path.join(here, 'atlasplots', '__version__.py'), 'r') as f:
    exec(f.read(), about)

# Get the long description from the relevant file
with codecs_open('README.rst', encoding='utf-8') as f:
    long_description = f.read()

setup(
    name=about['__title__'],
    version=about['__version__'],
    description=about['__description__'],
    long_description=long_description,
    url=about['__url__'],
    author=about['__author__'],
    author_email=about['__author_email__'],
    license=about['__license__'],
    classifiers=[
        'Development Status :: 3 - Alpha',

        'Intended Audience :: Science/Research',
        'Topic :: Scientific/Engineering :: Physics',

        'License :: OSI Approved :: MIT License',

        'Natural Language :: English',

        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
    ],
    keywords='CERN ATLAS PyROOT plotting',
    packages=find_packages(exclude=['docs', 'tests']),
    install_requires=['numpy', 'toml'],
    extras_require={
        'test': ['pytest', 'coverage'],
    },
    entry_points={
        'console_scripts': [
            'slim-trees=atlasplots.slim_trees:main',
        ],
    },
)
