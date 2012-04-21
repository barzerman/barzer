#from distutils.core import setup
from setuptools import setup, find_packages


requires = ['lxml']

setup(name='barzer_client',
      version='1.0',
      description='Barzer client library',
      url='http://barzer.net/client/',
      py_modules=['barzer_client'],
      install_requires=requires
      )