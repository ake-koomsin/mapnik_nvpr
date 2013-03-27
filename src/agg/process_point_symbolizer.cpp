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

// mapnik
#include <mapnik/agg_renderer.hpp>
#include <mapnik/agg_rasterizer.hpp>
#include <mapnik/image_util.hpp>

#include <mapnik/geom_util.hpp>
#include <mapnik/point_symbolizer.hpp>
#include <mapnik/expression_evaluator.hpp>
#include <mapnik/marker.hpp>
#include <mapnik/marker_cache.hpp>

// agg
#include "agg_trans_affine.h"

// stl
#include <string>

// boost
#include <boost/make_shared.hpp>

namespace mapnik {

template <typename T>
void agg_renderer<T>::process(point_symbolizer const& sym,
                              mapnik::feature_impl & feature,
                              proj_transform const& prj_trans)
{

   for (std::list<feature_ptr>::iterator f = featureList_->begin(); f != featureList_->end(); f++) {
        feature_ptr featurePtr = *f;

        mapnik::feature_impl &feature_ = *featurePtr;

    std::string filename = path_processor_type::evaluate(*sym.get_filename(), feature_);

    boost::optional<mapnik::marker_ptr> markerPtr;
    if ( !filename.empty() )
    {
        markerPtr = marker_cache::instance().find(filename, true);
    }
    else
    {
        markerPtr.reset(boost::make_shared<mapnik::marker>());
    }

    if (markerPtr)
    {
        box2d<double> const& bbox = (*markerPtr)->bounding_box();
        coord2d center = bbox.center();

        agg::trans_affine tr;
        evaluate_transform(tr, feature_, sym.get_image_transform());
        agg::trans_affine_translation recenter(-center.x, -center.y);
        agg::trans_affine recenter_tr = recenter * tr;
        box2d<double> label_ext = bbox * recenter_tr;

        for (unsigned i=0; i< feature_.num_geometries(); ++i)
        {
            geometry_type const& geom = feature_.get_geometry(i);
            double x;
            double y;
            double z=0;
            if (sym.get_point_placement() == CENTROID_POINT_PLACEMENT)
            {
                if (!label::centroid(geom, x, y))
                    return;
            }
            else
            {
                if (!label::interior_position(geom ,x, y))
                    return;
            }

            prj_trans.backward(x,y,z);
            t_.forward(&x,&y);
            label_ext.re_center(x,y);
            if (sym.get_allow_overlap() ||
                detector_->has_placement(label_ext))
            {


                    marker const& marker_ = **markerPtr;

                    image_data_32 const& src = **marker_.get_bitmap_data();
          double width =  src.width();
          double height =  src.height();

          const double shifted = 0;
          double markerX = x + shifted - width/2, markerY = height_ - y - shifted - height/2;


                  glMatrixLoadIdentityEXT(GL_PROJECTION);
                  glMatrixOrthoEXT(GL_PROJECTION, 0, width_, height_, 0, -1, 1);
                  glMatrixLoadIdentityEXT(GL_MODELVIEW);


                  GLubyte cmd[5];  
                  GLfloat coord[8];
                  int m = 0, n = 0;

                  cmd[m++] = GL_MOVE_TO_NV;
                  coord[n++] = markerX;
                  coord[n++] = markerY;
                  cmd[m++] = GL_LINE_TO_NV;
                  coord[n++] = markerX + width;
                  coord[n++] = markerY;
                  cmd[m++] = GL_LINE_TO_NV;
                  coord[n++] = markerX + width;
                  coord[n++] = markerY + height;
                  cmd[m++] = GL_LINE_TO_NV;
                  coord[n++] = markerX;
                  coord[n++] = markerY + height;
                  cmd[m++] = GL_CLOSE_PATH_NV;

                  glPathCommandsNV(pathObject_, m, cmd, n, GL_FLOAT, coord);
                
                  static GLuint texName;
                  glGenTextures(1, &texName);
                  glBindTexture(GL_TEXTURE_2D, texName);
                  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                  glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
                  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char *)src.getBytes());


                  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);




                  GLfloat data[2][3] = { { 1,0,0 },    /* s = 1*x + 0*y + 0 */
                                         { 0,1,0 } };  /* t = 0*x + 1*y + 0 */
                  
                    glEnable(GL_TEXTURE_2D);
                    glPathTexGenNV(GL_TEXTURE0, GL_PATH_OBJECT_BOUNDING_BOX_NV, 2, &data[0][0]);
                    glStencilFillPathNV(pathObject_, GL_COUNT_UP_NV, 0x1F);
                    glCoverFillPathNV(pathObject_, GL_BOUNDING_BOX_NV);
                    glDisable(GL_TEXTURE_2D);

                    pathObject_++;


          

                // render_marker(pixel_position(x, y),
                //               **marker,
                //               tr,
                //               sym.get_opacity(),
                //               sym.comp_op());

                if (!sym.get_ignore_placement())
                    detector_->insert(label_ext);

            }
        }




    }

  }
}

template void agg_renderer<image_32>::process(point_symbolizer const&,
                                              mapnik::feature_impl &,
                                              proj_transform const&);

}
