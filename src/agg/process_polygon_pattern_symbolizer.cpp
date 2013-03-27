/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2011 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

// boost
#include <boost/foreach.hpp>
// mapnik
#include <mapnik/debug.hpp>
#include <mapnik/graphics.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/agg_helpers.hpp>
#include <mapnik/agg_rasterizer.hpp>
#include <mapnik/marker.hpp>
#include <mapnik/marker_cache.hpp>
#include <mapnik/expression_evaluator.hpp>
#include <mapnik/vertex_converters.hpp>
// agg
#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_scanline_u.h"
// for polygon_pattern_symbolizer
#include "agg_renderer_scanline.h"
#include "agg_span_allocator.h"
#include "agg_span_pattern_rgba.h"
#include "agg_image_accessors.h"
#include "agg_conv_clip_polygon.h"

namespace mapnik {

template <typename T>
void agg_renderer<T>::process(polygon_pattern_symbolizer const& sym,
                              mapnik::feature_impl & feature,
                              proj_transform const& prj_trans)
{

    int markerWidth = 0;
    int markerHeight = 0;


    typedef boost::mpl::vector<clip_poly_tag,transform_tag,affine_transform_tag,simplify_tag,smooth_tag> conv_types;

    for (std::list<feature_ptr>::iterator f = featureList_->begin(); f != featureList_->end(); f++) {
        feature_ptr featurePtr = *f;

        agg::trans_affine tr;
        evaluate_transform(tr, *featurePtr, sym.get_transform());

 
            vertex_converter<box2d<double>, agg::path_storage, polygon_pattern_symbolizer,
                         CoordTransform, proj_transform, agg::trans_affine, conv_types>
            converter(query_extent_, pathStorage_, sym, t_, prj_trans, tr, scale_factor_);


        if (prj_trans.equal() && sym.clip()) converter.set<clip_poly_tag>(); //optional clip (default: true)
        converter.set<transform_tag>(); //always transform
        converter.set<affine_transform_tag>(); 
        if (sym.simplify_tolerance() > 0.0) converter.set<simplify_tag>(); // optional simplify converter
        if (sym.smooth() > 0.0) converter.set<smooth_tag>(); // optional smooth converter

        BOOST_FOREACH(geometry_type & geom, featurePtr->paths()) {
            if (geom.size() > 2) {
                converter.apply(geom);
            }
        }


    boost::optional<mapnik::marker_ptr> markerPtr;
    std::string filename = path_processor_type::evaluate( *sym.get_filename(), *featurePtr);

    if ( !filename.empty() )
    {
        markerPtr = marker_cache::instance().find(filename, true);
    }
    else
    {
        MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: File not found=" << filename;
    }

    if (!markerPtr) return;

    if (!(*markerPtr)->is_bitmap())
    {
        MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: Only images (not '" << filename << "') are supported in the polygon_pattern_symbolizer";

        return;
    }

        marker const& marker_ = **markerPtr;

        image_data_32 const& src = **marker_.get_bitmap_data();
        double width =  src.width();
        double height =  src.height();

        if(width) markerWidth = width;
        if(height) markerHeight = height;


        static GLuint texName;
        glGenTextures(1, &texName);
        glBindTexture(GL_TEXTURE_2D, texName);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char *)src.getBytes());

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    
    unsigned int size = pathStorage_.total_vertices() * 2;

    if(size >= 900000){
    GLfloat pathCoords[size][2];
    GLubyte pathCommands[size];

    unsigned int numberOfVertices = 0;
    unsigned int numberOfCommands = 0;
    double minX = 0.0;
    double maxX = 0.0;
    double minY = 0.0;
    double maxY = 0.0;

    extractVerticesInCurrentPathStorage(pathCoords, numberOfVertices, pathCommands, numberOfCommands,minX,maxX,minY,maxY);

    
    glPathCommandsNV(pathObject_, numberOfCommands, pathCommands, numberOfVertices * 2, GL_FLOAT, pathCoords);


    int numOfTextureCol = (maxX - minX) / markerWidth;
    int numOfTextureRow = (maxY - minY) / markerHeight;


    GLfloat data[2][3] = { { numOfTextureCol,0,0 },    /* s = 1*x + 0*y + 0 */
                           { 0,numOfTextureRow,0 } };  /* t = 0*x + 1*y + 0 */
    
    //glColor3f(1,0,1);
      
    glEnable(GL_TEXTURE_2D);
    glPathTexGenNV(GL_TEXTURE0, GL_PATH_OBJECT_BOUNDING_BOX_NV, 2, &data[0][0]);
    glStencilFillPathNV(pathObject_, GL_COUNT_UP_NV, 0x1F);
    glCoverFillPathNV(pathObject_, GL_BOUNDING_BOX_NV);
    glDisable(GL_TEXTURE_2D);



    pathObject_++; // increase for a new path object
    pathStorage_.remove_all();

    }

    }


  


    unsigned int size = pathStorage_.total_vertices() * 2;

    GLfloat pathCoords[size][2];
    GLubyte pathCommands[size];

    unsigned int numberOfVertices = 0;
    unsigned int numberOfCommands = 0;
    double minX = 0.0;
    double maxX = 0.0;
    double minY = 0.0;
    double maxY = 0.0;

    extractVerticesInCurrentPathStorage(pathCoords, numberOfVertices, pathCommands, numberOfCommands,minX,maxX,minY,maxY);


    glPathCommandsNV(pathObject_, numberOfCommands, pathCommands, numberOfVertices * 2, GL_FLOAT, pathCoords);


    int numOfTextureCol = (maxX - minX) / markerWidth;
    int numOfTextureRow = (maxY - minY) / markerHeight;


    GLfloat data[2][3] = { { numOfTextureCol,0,0 },    /* s = 1*x + 0*y + 0 */
                           { 0,numOfTextureRow,0 } };  /* t = 0*x + 1*y + 0 */
                  
    glEnable(GL_TEXTURE_2D);

    glPathTexGenNV(GL_TEXTURE0, GL_PATH_OBJECT_BOUNDING_BOX_NV, 2, &data[0][0]);
    glStencilFillPathNV(pathObject_, GL_COUNT_UP_NV, 0x1F);
    glCoverFillPathNV(pathObject_, GL_BOUNDING_BOX_NV);
    glDisable(GL_TEXTURE_2D);



    pathObject_++; // increase for a new path object
    pathStorage_.remove_all();






}


template void agg_renderer<image_32>::process(polygon_pattern_symbolizer const&,
                                              mapnik::feature_impl &,
                                              proj_transform const&);

}
