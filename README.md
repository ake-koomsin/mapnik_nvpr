# About This Modification

This project is done by Ake Koomsin and Ronakorn Soponpunth.

We introduce Nvidia's Path Rendering to Mapnik. It also involves offscreen rendering and multisampling.

This works only with Nvidia's graphic card.

This project is based on Mapnik version 2.2.0-pre. We modified following files
- include/mapnik/nvpr_init.hpp
- include/mapnik/agg_renderer.hpp
- include/mapnik/feature_style_processor_impl.hpp
- src/agg/agg_renderer.cpp
- src/agg/process_building_symbolizer.cpp
- src/agg/process_line_symbolizer.cpp
- src/agg/process_point_symbolizer.cpp
- src/agg/process_polygon_pattern_symbolizer.cpp
- src/agg/process_polygon_symbolizer.cpp
- src/agg/process_shield_symbolizer.cpp
- src/agg/process_text_symbolizer.cpp
- Sconstruct
- include/build.py
- src/build.py

# Testing Platform

Here is the list of our hardware, compiler and driver:
- CPU: Intel i7-2600 @ 3.60 GHz • RAM: 8 GB
- GPU: Geforce GTX 460
- Compiler: GCC 4.6.3
- Nvidia Driver: 304.64

We test on Ubuntu 12.04 LTS. We discover that this implementation 30-60% faster than using AGG renderer.

Other than Mapnik's original dependencies, make sure you have: freeglut libX11-dev glew(at least 1.8)

We test the result based on Openstreetmap style from URL below.
https://trac.openstreetmap.org/browser/applications/rendering/mapnik

and geometry data from URL below.
http://download.geofabrik.de/openstreetmap/

To see example of the map, make sure that all required library is installed.
To check acquired library type following command in mapnik root directory

./configure
make && sudo make install

Then simply run example demo in demo/c++ or demo/python by type command below

For demo/c++ : ./rundemo /usr/local/lib/mapnik
For demo/python : python rundemo.py


```
    _/      _/                                _/  _/
   _/_/  _/_/    _/_/_/  _/_/_/    _/_/_/        _/  _/
  _/  _/  _/  _/    _/  _/    _/  _/    _/  _/  _/_/
 _/      _/  _/    _/  _/    _/  _/    _/  _/  _/  _/
_/      _/    _/_/_/  _/_/_/    _/    _/  _/  _/    _/
                     _/
                    _/
```

[![Build Status](https://secure.travis-ci.org/mapnik/mapnik.png)](http://travis-ci.org/mapnik/mapnik)

# What is Mapnik?

Mapnik is an open source toolkit and API for developing mapping applications. At the core is a C++ shared library providing algorithms and patterns for spatial data access and visualization. High-level bindings for Java, JavaScript, Python, and Ruby facilitate rapid application development in a variety of environments.

# Overview

Mapnik is basically a collection of geographic objects like maps, layers, datasources, features, and geometries. The library doesn't rely on any OS specific "windowing systems" and it can be deployed to any server environment. It is intended to play fair in a multi-threaded environment and is aimed primarily, but not exclusively, at web-based development.


For further information see [http://mapnik.org](http://mapnik.org) and also our [wiki documentation](https://github.com/mapnik/mapnik/wiki) here on GitHub.

# Installation

See [INSTALL.md](https://github.com/mapnik/mapnik/blob/master/INSTALL.md) for installation instructions.

# License

Mapnik software is free and is released under LGPL ([GNU Lesser General Public License](http://www.gnu.org/licenses/lgpl.html_). Please see [COPYING](https://github.com/mapnik/mapnik/blob/master/COPYING) for more information.
