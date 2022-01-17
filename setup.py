import sys
from setuptools import setup, Extension

VERSION = "0.5.2"

pywfplan_ext = Extension("pywfplan.pywfplan_ext",

                         sources=["src/shift.cpp",
                                  "src/staff_energy.cpp",
                                  "src/staff_planner.cpp",
                                  "src/pywfplan_ext.cpp"],

                         libraries=["boost_python3{}".format(sys.version_info[1])],

                         include_dirs=["src",
                                       "pywfplan/pywfplan_ext",
                                       "/usr/local/include"],

                         library_dirs=["/usr/local/lib"],

                         extra_compile_args=["-std=c++17"])

setup(name="pywfplan",

      license="MIT",
      version=VERSION,

      description="Work force planner",
      long_description="""

      A Python library to plan a call center workforce based on expected call
      distribution and operator contract rules, availability and preferences.

      """,

      author="Luca Marx",
      author_email="luca@lucamarx.com",

      url="https://github.com/lucamarx/pywfplan",

      ext_modules=[pywfplan_ext],

      packages=["pywfplan"],

      python_requires=">=3.8",

      install_requires=[
          "tabulate>=0.8.0",
          "matplotlib>=3.5.0"
      ])
