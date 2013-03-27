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
#include <mapnik/agg_helpers.hpp>
#include <mapnik/agg_rasterizer.hpp>

#include <mapnik/line_symbolizer.hpp>
#include <mapnik/vertex_converters.hpp>

// agg
#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_scanline_u.h"
#include "agg_renderer_scanline.h"
#include "agg_scanline_p.h"
#include "agg_conv_stroke.h"
#include "agg_conv_dash.h"
#include "agg_renderer_outline_aa.h"
#include "agg_rasterizer_outline_aa.h"

// boost
#include <boost/foreach.hpp>

// stl
#include <string>

namespace mapnik {

template <typename T>
void agg_renderer<T>::setJoinCaps(stroke const& stroke) {

    line_join_e join=stroke.get_line_join();
    switch (join) {
        case MITER_JOIN:
            glPathParameteriNV(pathObject_, GL_PATH_JOIN_STYLE_NV, GL_MITER_TRUNCATE_NV);
            // stroke.generator().line_join(agg::miter_join);
            break;
        case MITER_REVERT_JOIN:
            glPathParameteriNV(pathObject_, GL_PATH_JOIN_STYLE_NV, GL_MITER_REVERT_NV);
            // stroke.generator().line_join(agg::miter_join);
            break;
        case ROUND_JOIN:
            // stroke.generator().line_join(agg::round_join);
            glPathParameteriNV(pathObject_, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
            break;
        default:
            glPathParameteriNV(pathObject_, GL_PATH_JOIN_STYLE_NV, GL_BEVEL_NV);
            // stroke.generator().line_join(agg::bevel_join);
    }

    line_cap_e cap=stroke.get_line_cap();
    switch (cap) {
        case BUTT_CAP:
            glPathParameteriNV(pathObject_, GL_PATH_INITIAL_END_CAP_NV, GL_FLAT);
            glPathParameteriNV(pathObject_, GL_PATH_TERMINAL_END_CAP_NV, GL_FLAT);
            // stroke.generator().line_cap(agg::butt_cap);
            break;
        case SQUARE_CAP:
            glPathParameteriNV(pathObject_, GL_PATH_INITIAL_END_CAP_NV, GL_SQUARE_NV);
            glPathParameteriNV(pathObject_, GL_PATH_TERMINAL_END_CAP_NV, GL_SQUARE_NV);
            // stroke.generator().line_cap(agg::square_cap);
            break;
        default:
            glPathParameteriNV(pathObject_, GL_PATH_INITIAL_END_CAP_NV, GL_ROUND_NV);
            glPathParameteriNV(pathObject_, GL_PATH_TERMINAL_END_CAP_NV, GL_ROUND_NV);
            // stroke.generator().line_cap(agg::round_cap);
    }
}

template <typename T>
void agg_renderer<T>::setMiterLimit(stroke const& stroke) {
    glPathParameterfNV(pathObject_, GL_PATH_MITER_LIMIT_NV, stroke.get_miterlimit());
}

template <typename T>
void agg_renderer<T>::setWidth(stroke const& stroke, double scale_factor) {
    glPathParameterfNV(pathObject_, GL_PATH_STROKE_WIDTH_NV, stroke.get_width() * scale_factor);
}

template <typename T>
void agg_renderer<T>::setDash(stroke const& stroke, double scale_factor) {
    dash_array const& d = stroke.get_dash_array();
    dash_array::const_iterator itr = d.begin();
    dash_array::const_iterator end = d.end();

    GLfloat *dashArray = (float *)calloc(d.size() * 2, sizeof(GLfloat *));
    unsigned int i = 0;
    for (;itr != end;++itr)
    {
        dashArray[i] = (GLfloat)(itr->first * scale_factor);
        i++;
        dashArray[i] = (GLfloat)(itr->second * scale_factor);
        i++;
    }

    glPathDashArrayNV(pathObject_, i, dashArray);

    free(dashArray);
}

// template <typename T>
// void agg_renderer<T>::process(line_symbolizer const& sym,
//                               mapnik::feature_impl & feature,
//                               proj_transform const& prj_trans)

// {
//     // temporary disable
//     // return;
//     //printf("start line\n");

//     stroke const& stroke_ = sym.get_stroke();
//     color const& col = stroke_.get_color();
//     unsigned r=col.red();
//     unsigned g=col.green();
//     unsigned b=col.blue();
//     unsigned a=col.alpha();

//     ras_ptr->reset();
//     set_gamma_method(stroke_, ras_ptr);

//     agg::rendering_buffer buf(current_buffer_->raw_data(),width_,height_, width_ * 4);

//     typedef agg::rgba8 color_type;
//     typedef agg::order_rgba order_type;
//     typedef agg::pixel32_type pixel_type;
//     typedef agg::comp_op_adaptor_rgba_pre<color_type, order_type> blender_type; // comp blender
//     typedef agg::pixfmt_custom_blend_rgba<blender_type, agg::rendering_buffer> pixfmt_comp_type;
//     typedef agg::renderer_base<pixfmt_comp_type> renderer_base;
//     typedef boost::mpl::vector<clip_line_tag, transform_tag,
//                                offset_transform_tag, affine_transform_tag,
//                                simplify_tag, smooth_tag, dash_tag, stroke_tag> conv_types;

//     pixfmt_comp_type pixf(buf);
//     pixf.comp_op(static_cast<agg::comp_op_e>(sym.comp_op()));
//     renderer_base renb(pixf);

//     agg::trans_affine tr;
//     evaluate_transform(tr, feature, sym.get_transform());

//     box2d<double> clipping_extent = query_extent_;
//     if (sym.clip())
//     {
//         double padding = (double)(query_extent_.width()/pixmap_.width());
//         float half_stroke = stroke_.get_width()/2.0;
//         if (half_stroke > 1)
//             padding *= half_stroke;
//         if (fabs(sym.offset()) > 0)
//             padding *= fabs(sym.offset()) * 1.2;
//         double x0 = query_extent_.minx();
//         double y0 = query_extent_.miny();
//         double x1 = query_extent_.maxx();
//         double y1 = query_extent_.maxy();
//         clipping_extent.init(x0 - padding, y0 - padding, x1 + padding , y1 + padding);
//         // debugging
//         //box2d<double> inverse(x0 + padding, y0 + padding, x1 - padding , y1 - padding);
//         //draw_geo_extent(inverse,mapnik::color("red"));
//     }

//     typedef typename agg::conv_clip_polyline<geometry_type> Clipper;
//     typedef coord_transform<CoordTransform, Clipper> Transformer;

//     boost::timer t;

//     BOOST_FOREACH( geometry_type & geom, feature.paths()) {
//         if (geom.size() > 1) {
//             // Set up clipper
//             Clipper clipper(geom);
//             clipper.clip_box(clipping_extent.minx(), clipping_extent.miny(), clipping_extent.maxx(), clipping_extent.maxy());

//             // Set up transformer
//             Transformer transformer(clipper);
//             transformer.set_proj_trans(prj_trans);
//             transformer.set_trans(t_);

//             // Add paths to storage
//             pathStorage_.concat_path(transformer);
//         }
//     }

//     unsigned int size = pathStorage_.total_vertices() * 2;

//     GLfloat pathCoords[size][2];
//     GLubyte pathCommands[size];

//     unsigned int numberOfVertices = 0;
//     unsigned int numberOfCommands = 0;

//     extractVerticesInCurrentPathStorage(pathCoords, numberOfVertices, pathCommands, numberOfCommands);

//     accTime_ += t.elapsed();

//     //glMatrixPushEXT(GL_MODELVIEW);

    

//     glPathCommandsNV(pathObject_, numberOfCommands, pathCommands, numberOfVertices * 2, GL_FLOAT, pathCoords);

//     float affineMatrix[16] = {0};
//     affineMatrix[0] = tr.sx;
//     affineMatrix[1] = tr.shy;
//     affineMatrix[2] = tr.shx;
//     affineMatrix[3] = tr.sy;
//     affineMatrix[4] = tr.tx;
//     affineMatrix[5] = tr.ty;

//     // Parameters
//     color const& fill = stroke_.get_color();

//     setJoinCaps(stroke_);
//     setMiterLimit(stroke_);
//     setWidth(stroke_, scale_factor_);
//     if (stroke_.has_dash()) {
//         setDash(stroke_, scale_factor_);
//     }

//     // Affine transform
//     glTransformPathNV(pathObject_, pathObject_, GL_AFFINE_2D_NV, affineMatrix);

//     boost::timer t2;

//     glStencilStrokePathNV(pathObject_, 0x1, 0x1F);

//     agg::rgba8 color = agg::rgba8_pre(fill.red(), fill.green(), fill.blue(), int(fill.alpha() * stroke_.get_opacity()));
   
//     glColor4ub(color.r, color.g , color.b , color.a);
//     glCoverStrokePathNV(pathObject_, GL_BOUNDING_BOX_NV);

//     accTime2_ += t2.elapsed();

//     // -----------------------------------------------------------------

//     //compositeTexture(featureTexture_, currentBackgroundTexture_, static_cast<composite_mode_e>(sym.comp_op()));
//     //glDisable(GL_MULTISAMPLE);

//     //glMatrixPopEXT(GL_MODELVIEW);

//     pathObject_++; // increase for a new path object
//     pathStorage_.remove_all();

//     // if (sym.get_rasterizer() == RASTERIZER_FAST)
//     // {
//     //     typedef agg::renderer_outline_aa<renderer_base> renderer_type;
//     //     typedef agg::rasterizer_outline_aa<renderer_type> rasterizer_type;
//     //     agg::line_profile_aa profile(stroke_.get_width() * scale_factor_, agg::gamma_power(stroke_.get_gamma()));
//     //     renderer_type ren(renb, profile);
//     //     ren.color(agg::rgba8_pre(r, g, b, int(a*stroke_.get_opacity())));
//     //     rasterizer_type ras(ren);
//     //     set_join_caps_aa(stroke_,ras);

//     //     vertex_converter<box2d<double>, rasterizer_type, line_symbolizer,
//     //                      CoordTransform, proj_transform, agg::trans_affine, conv_types>
//     //         converter(clipping_extent,ras,sym,t_,prj_trans,tr,scale_factor_);
//     //     if (sym.clip()) converter.set<clip_line_tag>(); // optional clip (default: true)
//     //     converter.set<transform_tag>(); // always transform
//     //     if (fabs(sym.offset()) > 0.0) converter.set<offset_transform_tag>(); // parallel offset
//     //     converter.set<affine_transform_tag>(); // optional affine transform
//     //     if (sym.simplify_tolerance() > 0.0) converter.set<simplify_tag>(); // optional simplify converter
//     //     if (sym.smooth() > 0.0) converter.set<smooth_tag>(); // optional smooth converter

//     //     BOOST_FOREACH( geometry_type & geom, feature.paths())
//     //     {
//     //         if (geom.size() > 1)
//     //         {
//     //             converter.apply(geom);
//     //         }
//     //     }
//     // }
//     // else
//     // {
//     //     vertex_converter<box2d<double>, rasterizer, line_symbolizer,
//     //                      CoordTransform, proj_transform, agg::trans_affine, conv_types>
//     //         converter(clipping_extent,*ras_ptr,sym,t_,prj_trans,tr,scale_factor_);

//     //     if (sym.clip()) converter.set<clip_line_tag>(); // optional clip (default: true)
//     //     converter.set<transform_tag>(); // always transform
//     //     if (fabs(sym.offset()) > 0.0) converter.set<offset_transform_tag>(); // parallel offset
//     //     converter.set<affine_transform_tag>(); // optional affine transform
//     //     if (sym.simplify_tolerance() > 0.0) converter.set<simplify_tag>(); // optional simplify converter
//     //     if (sym.smooth() > 0.0) converter.set<smooth_tag>(); // optional smooth converter
//     //     if (stroke_.has_dash()) converter.set<dash_tag>();
//     //     converter.set<stroke_tag>(); //always stroke

//     //     BOOST_FOREACH( geometry_type & geom, feature.paths())
//     //     {
//     //         if (geom.size() > 1)
//     //         {
//     //             converter.apply(geom);
//     //         }
//     //     }

//     //     typedef agg::renderer_scanline_aa_solid<renderer_base> renderer_type;
//     //     renderer_base renb(pixf);
//     //     renderer_type ren(renb);
//     //     ren.color(agg::rgba8_pre(r, g, b, int(a * stroke_.get_opacity())));
//     //     agg::scanline_u8 sl;
//     //     agg::render_scanlines(*ras_ptr, sl, ren);
//     // }
// }

template <typename T>
void agg_renderer<T>::process(line_symbolizer const& sym,
                              mapnik::feature_impl & feature,
                              proj_transform const& prj_trans)
{

    stroke const& stroke_ = sym.get_stroke();

    // ras_ptr->reset();
    // set_gamma_method(stroke_, ras_ptr);

    typedef boost::mpl::vector<clip_line_tag, transform_tag,
                               offset_transform_tag, affine_transform_tag,
                               simplify_tag, smooth_tag, dash_tag, stroke_tag> conv_types;

    box2d<double> clipping_extent = query_extent_;
    if (sym.clip())
    {
        double padding = (double)(query_extent_.width()/pixmap_.width());
        float half_stroke = stroke_.get_width()/2.0;
        if (half_stroke > 1)
            padding *= half_stroke;
        if (fabs(sym.offset()) > 0)
            padding *= fabs(sym.offset()) * 1.2;
        double x0 = query_extent_.minx();
        double y0 = query_extent_.miny();
        double x1 = query_extent_.maxx();
        double y1 = query_extent_.maxy();
        clipping_extent.init(x0 - padding, y0 - padding, x1 + padding , y1 + padding);
        // debugging
        //box2d<double> inverse(x0 + padding, y0 + padding, x1 - padding , y1 - padding);
        //draw_geo_extent(inverse,mapnik::color("red"));
    }
    
    typedef boost::mpl::vector<clip_line_tag, transform_tag,
                               offset_transform_tag, affine_transform_tag,
                               simplify_tag, smooth_tag, dash_tag, stroke_tag> conv_types;


    for (std::list<feature_ptr>::iterator f = featureList_->begin(); f != featureList_->end(); f++) {
        feature_ptr featurePtr = *f;

        agg::trans_affine tr;
        evaluate_transform(tr, *featurePtr, sym.get_transform());

        vertex_converter<box2d<double>, agg::path_storage, line_symbolizer,
                         CoordTransform, proj_transform, agg::trans_affine, conv_types>
            converter(query_extent_, pathStorage_, sym, t_, prj_trans, tr, scale_factor_);

        if (sym.clip()) converter.set<clip_line_tag>(); // optional clip (default: true)
        converter.set<transform_tag>(); // always transform
        if (fabs(sym.offset()) > 0.0) converter.set<offset_transform_tag>(); // parallel offset
        converter.set<affine_transform_tag>(); // optional affine transform
        if (sym.simplify_tolerance() > 0.0) converter.set<simplify_tag>(); // optional simplify converter
        if (sym.smooth() > 0.0) converter.set<smooth_tag>(); // optional smooth converter

        BOOST_FOREACH(geometry_type & geom, featurePtr->paths()) {
            if (geom.size() > 1) {
                converter.apply(geom);
            }
        }


    unsigned int size = pathStorage_.total_vertices() * 2;

        if(size >= 900000){

    GLfloat pathCoords[size][2];
    GLubyte pathCommands[size];

    unsigned int numberOfVertices = 0;
    unsigned int numberOfCommands = 0;

    extractVerticesInCurrentPathStorage(pathCoords, numberOfVertices, pathCommands, numberOfCommands);

    glPathCommandsNV(pathObject_, numberOfCommands, pathCommands, numberOfVertices * 2, GL_FLOAT, pathCoords);

    // Parameters
    color const& fill = stroke_.get_color();

    setJoinCaps(stroke_);
    setMiterLimit(stroke_);
    setWidth(stroke_, scale_factor_);
    if (stroke_.has_dash()) {
        setDash(stroke_, scale_factor_);
    }

    boost::timer t2;

    glStencilStrokePathNV(pathObject_, 0x1, 0x1F);

    accTime2_ += t2.elapsed();

    agg::rgba8 color = agg::rgba8_pre(fill.red(), fill.green(), fill.blue(), int(fill.alpha() * stroke_.get_opacity()));
   
    glColor4ub(color.r, color.g , color.b , color.a);

        
    boost::timer t3;
    glCoverStrokePathNV(pathObject_, GL_BOUNDING_BOX_NV);
     accTime3_ += t3.elapsed();
     


    pathObject_++; // increase for a new path object
    pathStorage_.remove_all();


        }

    }


    unsigned int size = pathStorage_.total_vertices() * 2;

    GLfloat pathCoords[size][2];
    GLubyte pathCommands[size];

    unsigned int numberOfVertices = 0;
    unsigned int numberOfCommands = 0;

    extractVerticesInCurrentPathStorage(pathCoords, numberOfVertices, pathCommands, numberOfCommands);

    glPathCommandsNV(pathObject_, numberOfCommands, pathCommands, numberOfVertices * 2, GL_FLOAT, pathCoords);

    // Parameters
    color const& fill = stroke_.get_color();

    setJoinCaps(stroke_);
    setMiterLimit(stroke_);
    setWidth(stroke_, scale_factor_);
    if (stroke_.has_dash()) {
        setDash(stroke_, scale_factor_);
    }

    boost::timer t2;

    glStencilStrokePathNV(pathObject_, 0x1, 0x1F);

    accTime2_ += t2.elapsed();

    agg::rgba8 color = agg::rgba8_pre(fill.red(), fill.green(), fill.blue(), int(fill.alpha() * stroke_.get_opacity()));
   
    glColor4ub(color.r, color.g , color.b , color.a);

        
    boost::timer t3;
    glCoverStrokePathNV(pathObject_, GL_BOUNDING_BOX_NV);
     accTime3_ += t3.elapsed();
     


    pathObject_++; // increase for a new path object
    pathStorage_.remove_all();



}


template void agg_renderer<image_32>::process(line_symbolizer const&,
                                              mapnik::feature_impl &,
                                              proj_transform const&);

}
