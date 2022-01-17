Pywfplan: Call center planning
==============================

Pywfplan is a Python library that helps planning the activity of a call
center.

It is based on a C++ application I wrote a few years ago that has been used in
production to plan call centers of sizes between 50 to 200 agents.

Pywfplan is a stripped down version of that application that retains the basic
functionality and exports it as a Python module.

Installation
------------

Ensure that you have a C++ compiler and the boost library installed, then run

    python setup.py install

Or use the provided docker file:

    docker build -t pywfplan
    docker run -p 8888:8888 -it pywfplan
