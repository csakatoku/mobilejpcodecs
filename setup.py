# -*- coding: utf-8 -*-
# Bootstrap setuptools
from ez_setup import use_setuptools
use_setuptools()

from setuptools import setup, find_packages, Extension

version = '0.1.0'

codecs_mobilejp = Extension('mobilejpcodecs._codecs_mobilejp',
                            sources = ['src/_codecs_mobilejp.c'])

setup(name='mobilejpcodecs',
      version=version,
      description="mobilejpcodecs",
      long_description="""
""",
      classifiers=[], # Get strings from http://www.python.org/pypi?%3Aaction=list_classifiers
      keywords='',
      author='Chihiro SAKATOKU',
      author_email='csakatoku@gmail.com',
      url='',
      license='',
      packages=find_packages(exclude=['ez_setup', 'tests', 'src', 'dat', 'tools']),
      ext_modules=[codecs_mobilejp],
      include_package_data=True,
      zip_safe=False,
      install_requires=[
          # -*- Extra requirements: -*-
      ],
      entry_points="""
      # -*- Entry points: -*-
      """,
      test_suite='nose.collector'
      )
