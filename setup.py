from setuptools import setup, find_packages

setup(
    name="toralizer",
    version="0.1.0",
    packages=find_packages(),
    install_requires=[
        "stem",
        "requests",
        "pysocks",
    ],
    entry_points={
        'console_scripts': [
            'toralize=toralizer.cli:main',
        ],
    },
    author="Filip Snitil",
    description="A command line tool to redirect network traffic through Tor",
)