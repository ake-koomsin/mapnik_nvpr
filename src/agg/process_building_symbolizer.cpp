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
#include <mapnik/graphics.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/agg_rasterizer.hpp>
#include <mapnik/segment.hpp>
#include <mapnik/expression_evaluator.hpp>

// boost
#include <boost/scoped_ptr.hpp>

// stl
#include <deque>

// agg
#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_scanline_u.h"
#include "agg_renderer_scanline.h"
#include "agg_conv_stroke.h"

namespace mapnik
{

template <typename T>
void agg_renderer<T>::process(building_symbolizer const& sym,
                              mapnik::feature_impl & feature,
                              proj_transform const& prj_trans)
{


    typedef coord_transform<CoordTransform,geometry_type> path_type;
    // typedef agg::renderer_base<agg::pixfmt_rgba32> ren_base;
    // typedef agg::renderer_scanline_aa_solid<ren_base> renderer;

   for (std::list<feature_ptr>::iterator f = featureList_->begin(); f != featureList_->end(); f++) {
        feature_ptr featurePtr = *f;

        mapnik::feature_impl &feature_ = *featurePtr;


    // agg::rendering_buffer buf(current_buffer_->raw_data(),width_,height_, width_ * 4);
    // agg::pixfmt_rgba32 pixf(buf);
    // ren_base renb(pixf);

    // color const& fill_  = sym.get_fill();
    // unsigned r=fill_.red();
    // unsigned g=fill_.green();
    // unsigned b=fill_.blue();
    // unsigned a=fill_.alpha();
    // renderer ren(renb);
    // agg::scanline_u8 sl;

    // ras_ptr->reset();
    // ras_ptr->gamma(agg::gamma_power());

    double height = 0.0;
    expression_ptr height_expr = sym.height();
    if (height_expr)
    {
        value_type result = boost::apply_visitor(evaluate<Feature,value_type>(feature_), *height_expr);
        height = result.to_double() * scale_factor_;
    }



    for (unsigned i=0;i<feature_.num_geometries();++i)
    {

        geometry_type const& geom = feature_.get_geometry(i);
        


        if (geom.size() > 2)
        {

            boost::scoped_ptr<geometry_type> frame(new geometry_type(LineString));
            boost::scoped_ptr<geometry_type> roof(new geometry_type(Polygon));
            std::deque<segment_t> face_segments;
            double x0 = 0;
            double y0 = 0;
            double x,y;
            geom.rewind(0);
            for (unsigned cm = geom.vertex(&x, &y); cm != SEG_END;
                 cm = geom.vertex(&x, &y))
            {
                if (cm == SEG_MOVETO)
                {
                    frame->move_to(x,y);

                }
                else if (cm == SEG_LINETO || cm == SEG_CLOSE)
                {
                    frame->line_to(x,y);
                    face_segments.push_back(segment_t(x0,y0,x,y));
                }
                x0 = x;
                y0 = y;
            }


            std::sort(face_segments.begin(),face_segments.end(), y_order);
            std::deque<segment_t>::const_iterator itr=face_segments.begin();
            std::deque<segment_t>::const_iterator end=face_segments.end();

            for (; itr!=end; ++itr)
            {
   

                boost::scoped_ptr<geometry_type> faces(new geometry_type(Polygon));
                faces->move_to(itr->get<0>(),itr->get<1>());
                faces->line_to(itr->get<2>(),itr->get<3>());
                faces->line_to(itr->get<2>(),itr->get<3>() + height);
                faces->line_to(itr->get<0>(),itr->get<1>() + height);

                path_type faces_path (t_,*faces,prj_trans);
                pathStorage_.concat_path(faces_path);
                // ras_ptr->add_path(faces_path);
                // ren.color(agg::rgba8(int(r*0.8), int(g*0.8), int(b*0.8), int(a * sym.get_opacity())));
                // agg::render_scanlines(*ras_ptr, sl, ren);
                // ras_ptr->reset();

              
                //
                frame->move_to(itr->get<0>(),itr->get<1>());
                frame->line_to(itr->get<0>(),itr->get<1>()+height);


            }

                 unsigned int size_face = pathStorage_.total_vertices() * 2;

                GLfloat pathCoords_face[size_face][2];
                GLubyte pathCommands_face[size_face];

                unsigned int numberOfVertices_face = 0;
                unsigned int numberOfCommands_face = 0;

                extractVerticesInCurrentPathStorage(pathCoords_face, numberOfVertices_face, pathCommands_face, numberOfCommands_face);

                color const& fill = sym.get_fill();
                
                glPathCommandsNV(pathObject_, numberOfCommands_face, pathCommands_face, numberOfVertices_face * 2, GL_FLOAT, pathCoords_face);


                glStencilFillPathNV(pathObject_, GL_COUNT_UP_NV, 0x1F);
                agg::rgba8 color_face = agg::rgba8_pre(fill.red()*0.8, fill.green()*0.8, fill.blue()*0.8, int(fill.alpha() * sym.get_opacity()));
                glColor4ub(color_face.r, color_face.g, color_face.b, color_face.a);
                glCoverFillPathNV(pathObject_, GL_BOUNDING_BOX_NV);


                pathObject_++; 
                pathStorage_.remove_all();


            geom.rewind(0);
            for (unsigned cm = geom.vertex(&x, &y); cm != SEG_END;
                 cm = geom.vertex(&x, &y))
            {
                if (cm == SEG_MOVETO)
                {
                    frame->move_to(x,y+height);
                    roof->move_to(x,y+height);

                }
                else if (cm == SEG_LINETO || cm == SEG_CLOSE)
                {
                    frame->line_to(x,y+height);
                    roof->line_to(x,y+height);
                }
            }

            path_type path(t_,*frame,prj_trans);
            agg::conv_stroke<path_type> stroke(path);
            stroke.width(scale_factor_);
            pathStorage_.concat_path(stroke);
            // ras_ptr->add_path(stroke);
            // ren.color(agg::rgba8(int(r*0.8), int(g*0.8), int(b*0.8), int(255 * sym.get_opacity())));
            // agg::render_scanlines(*ras_ptr, sl, ren);
            // ras_ptr->reset();

            unsigned int size_frame = pathStorage_.total_vertices() * 2;

            GLfloat pathCoords_frame[size_frame][2];
            GLubyte pathCommands_frame[size_frame];

            unsigned int numberOfVertices_frame = 0;
            unsigned int numberOfCommands_frame = 0;

            extractVerticesInCurrentPathStorage(pathCoords_frame, numberOfVertices_frame, pathCommands_frame, numberOfCommands_frame);
                
            glPathCommandsNV(pathObject_, numberOfCommands_frame, pathCommands_frame, numberOfVertices_frame * 2, GL_FLOAT, pathCoords_frame);


            glStencilFillPathNV(pathObject_, GL_COUNT_UP_NV, 0x1F);
            agg::rgba8 color_frame = agg::rgba8_pre(fill.red()*0.8, fill.green()*0.8, fill.blue()*0.8, int(fill.alpha() * sym.get_opacity()));
            glColor4ub(color_frame.r, color_frame.g, color_frame.b, color_frame.a);
            glCoverFillPathNV(pathObject_, GL_BOUNDING_BOX_NV);


            pathObject_++; 
            pathStorage_.remove_all();


            path_type roof_path (t_,*roof,prj_trans);
            pathStorage_.concat_path(roof_path);

            // ras_ptr->add_path(roof_path);
            // ren.color(agg::rgba8(r, g, b, int(a * sym.get_opacity())));
            // agg::render_scanlines(*ras_ptr, sl, ren);


            unsigned int size_roof = pathStorage_.total_vertices() * 2;

            GLfloat pathCoords_roof[size_roof][2];
            GLubyte pathCommands_roof[size_roof];

            unsigned int numberOfVertices_roof = 0;
            unsigned int numberOfCommands_roof = 0;

            extractVerticesInCurrentPathStorage(pathCoords_roof, numberOfVertices_roof, pathCommands_roof, numberOfCommands_roof);
                
            glPathCommandsNV(pathObject_, numberOfCommands_roof, pathCommands_roof, numberOfVertices_roof * 2, GL_FLOAT, pathCoords_roof);


            glStencilFillPathNV(pathObject_, GL_COUNT_UP_NV, 0x1F);
            agg::rgba8 color_roof = agg::rgba8_pre(fill.red(), fill.green(), fill.blue(), int(fill.alpha() * sym.get_opacity()));
            glColor4ub(color_roof.r, color_roof.g, color_roof.b, color_roof.a);
            glCoverFillPathNV(pathObject_, GL_BOUNDING_BOX_NV);

            pathObject_++; 
            pathStorage_.remove_all();


        }
    }

  }
}

template void agg_renderer<image_32>::process(building_symbolizer const&,
                                              mapnik::feature_impl &,
                                              proj_transform const&);

}
