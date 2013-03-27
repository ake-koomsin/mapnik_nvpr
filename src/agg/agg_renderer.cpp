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
#include <mapnik/agg_helpers.hpp>
#include <mapnik/graphics.hpp>

#include <mapnik/debug.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/feature_type_style.hpp>
#include <mapnik/marker.hpp>
#include <mapnik/marker_cache.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/font_set.hpp>
#include <mapnik/parse_path.hpp>
#include <mapnik/map.hpp>
#include <mapnik/svg/svg_converter.hpp>
#include <mapnik/svg/svg_renderer_agg.hpp>
#include <mapnik/svg/svg_path_adapter.hpp>

#include <mapnik/image_compositing.hpp>
#include <mapnik/image_filter.hpp>
#include <mapnik/image_util.hpp>
// agg
#define AGG_RENDERING_BUFFER row_ptr_cache<int8u>
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_scanline_u.h"
#include "agg_image_filters.h"
#include "agg_trans_bilinear.h"
#include "agg_span_allocator.h"
#include "agg_image_accessors.h"
#include "agg_span_image_filter_rgba.h"

// boost
#include <boost/utility.hpp>
#include <boost/make_shared.hpp>
#include <boost/math/special_functions/round.hpp>

// stl
#include <cmath>

// Shader
#include <mapnik/shader_program.hpp>

#include "setupGL.h"

namespace mapnik
{

template <typename T>
agg_renderer<T>::agg_renderer(Map const& m, T & pixmap, double scale_factor, unsigned offset_x, unsigned offset_y)
    : feature_style_processor<agg_renderer>(m, scale_factor),
      pixmap_(pixmap),
      internal_buffer_(),
      current_buffer_(&pixmap),
      style_level_compositing_(false),
      width_(pixmap_.width()),
      height_(pixmap_.height()),
      scale_factor_(scale_factor),
      t_(m.width(),m.height(),m.get_current_extent(),offset_x,offset_y),
      font_engine_(),
      font_manager_(font_engine_),
      detector_(boost::make_shared<label_collision_detector4>(box2d<double>(-m.buffer_size(), -m.buffer_size(), m.width() + m.buffer_size() ,m.height() + m.buffer_size()))),
      ras_ptr(new rasterizer),
      pathStorage_(),
      pathObject_(1),
      blendingModeLoaded_(35, false),
      blendingShader_(35),
      programs_(35),
      featureList_(NULL)
{
    setup(m);
    //setupOpenGL(m);
}

template <typename T>
agg_renderer<T>::agg_renderer(Map const& m, T & pixmap, boost::shared_ptr<label_collision_detector4> detector,
                              double scale_factor, unsigned offset_x, unsigned offset_y)
    : feature_style_processor<agg_renderer>(m, scale_factor),
      pixmap_(pixmap),
      internal_buffer_(),
      current_buffer_(&pixmap),
      style_level_compositing_(false),
      width_(pixmap_.width()),
      height_(pixmap_.height()),
      scale_factor_(scale_factor),
      t_(m.width(),m.height(),m.get_current_extent(),offset_x,offset_y),
      font_engine_(),
      font_manager_(font_engine_),
      detector_(detector),
      ras_ptr(new rasterizer),
      pathStorage_(),
      pathObject_(1),
      blendingModeLoaded_(35, false),
      blendingShader_(35),
      programs_(35),
      featureList_(NULL)
{
    setup(m);
    //setupOpenGL(m);
}

template <typename T>
void agg_renderer<T>::setup(Map const &m)
{
    boost::optional<color> const& bg = m.background();
    if (bg)
    {
        if (bg->alpha() < 255)
        {
            mapnik::color bg_color = *bg;
            bg_color.premultiply();
            pixmap_.set_background(bg_color);
        }
        else
        {
            pixmap_.set_background(*bg);
        }
    }

    boost::optional<std::string> const& image_filename = m.background_image();
    if (image_filename)
    {
        // NOTE: marker_cache returns premultiplied image, if needed
        boost::optional<mapnik::marker_ptr> bg_marker = mapnik::marker_cache::instance().find(*image_filename,true);
        if (bg_marker && (*bg_marker)->is_bitmap())
        {
            mapnik::image_ptr bg_image = *(*bg_marker)->get_bitmap_data();
            int w = bg_image->width();
            int h = bg_image->height();
            if ( w > 0 && h > 0)
            {
                // repeat background-image both vertically and horizontally
                unsigned x_steps = unsigned(std::ceil(width_/double(w)));
                unsigned y_steps = unsigned(std::ceil(height_/double(h)));
                for (unsigned x=0;x<x_steps;++x)
                {
                    for (unsigned y=0;y<y_steps;++y)
                    {
                        composite(pixmap_.data(),*bg_image, src_over, 1.0f, x*w, y*h, false);
                    }
                }
            }
        }
    }
    MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: Scale=" << m.scale();
}

template <typename T>
bool agg_renderer<T>::setupOpenGL(Map const &m) {

    // Start setting up OpenGL
    int argc = 0;
    char **argv = NULL;

    glutInit(&argc, argv);
    glutInitWindowSize(width_, height_);
    glutCreateWindow("map_rendering");
    glutHideWindow();
    glutInitDisplayMode(GLUT_RGBA | GLUT_MULTISAMPLE | GLUT_DEPTH | GLUT_STENCIL);

    int statusGlew = glewInit();

    if (statusGlew != GLEW_OK) {
        fprintf(stderr, "%s\n", "OpenGL Extension Wrangler (GLEW) failed to initialize");
        exit(1);
    }

    // Disable Vsync
    Display *dpy = glXGetCurrentDisplay();
    GLXDrawable drawable = glXGetCurrentDrawable();
    const int interval = 0;

    if (drawable) {
        glXSwapIntervalEXT(dpy, drawable, interval);
    }

    // Temporary disable
    // if (!glewIsSupported("GL_EXT_direct_state_access")) {
    //     fprintf(stderr, "%s\n", "OpenGL implementation doesn't support GL_EXT_direct_state_access (you should be using NVIDIA GPUs...)");
    //     exit(1);
    // }

    initializeNVPR("Mapnik GPU");
    if (!has_NV_path_rendering) {
        fprintf(stderr, "%s\n", "required NV_path_rendering OpenGL extension is not present");
        exit(1);
    }

    // Create initial shader program (srcOver)
    // currentProgram_ = createProgram(src_over);
    // currentShader_  = src_over;

    // glUseProgram(currentProgram_);

    // checkOpenGLError("Check error after set up initial program")

    // Initialize textures
    glGenTextures(4, textureArray_);

    for (unsigned int i = 0; i < 3; i++) {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureArray_[i]);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 16, GL_RGBA, width_, height_, GL_TRUE);
    }

    glBindTexture(GL_TEXTURE_2D, textureArray_[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    mainTexture_     = textureArray_[0];
    styleTexture_    = textureArray_[1];
    featureTexture_  = textureArray_[2];
    blankTexture_    = textureArray_[3];

    // For texture compositing
    listIndex_ = glGenLists(1);
    glNewList(listIndex_, GL_COMPILE);
    glBegin(GL_QUADS);
        glMultiTexCoord2f(GL_TEXTURE1, 0, 0); glMultiTexCoord2f(GL_TEXTURE2, 0, 0); glVertex2i(0, 0);
        glMultiTexCoord2f(GL_TEXTURE1, 1, 0); glMultiTexCoord2f(GL_TEXTURE2, 1, 0); glVertex2i(width_, 0);
        glMultiTexCoord2f(GL_TEXTURE1, 1, 1); glMultiTexCoord2f(GL_TEXTURE2, 1, 1); glVertex2i(width_, height_);
        glMultiTexCoord2f(GL_TEXTURE1, 0, 1); glMultiTexCoord2f(GL_TEXTURE2, 0, 1); glVertex2i(0, height_);
    glEnd();
    glEndList();

    // For texture cleaning
    rectDrawingIndex_ = glGenLists(1);
    glNewList(rectDrawingIndex_, GL_COMPILE);
    glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(width_, 0);
        glVertex2i(width_, height_);
        glVertex2i(0, height_);
    glEnd();
    glEndList();

    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    glMatrixOrthoEXT(GL_MODELVIEW, 0, width_, 0, height_, -1, 1);

    // Create frame buffer blit object
    glGenFramebuffers(1, &frameBufferBlit_);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferBlit_);

    // Create color buffer
    glGenRenderbuffers(1, &colorBufferBlit_);
    glBindRenderbuffer(GL_RENDERBUFFER, colorBufferBlit_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBufferBlit_);

    glGenRenderbuffers(1, &depthBufferBlit_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufferBlit_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferBlit_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBufferBlit_);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blankTexture_, 0);

    // -----------------------------------------------------------------------------------------------

    // Create frame buffer object
    glGenFramebuffers(1, &frameBuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_);

    // Create color buffer
    glGenRenderbuffers(1, &colorBuffer_);
    glBindRenderbuffer(GL_RENDERBUFFER, colorBuffer_);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 16, GL_RGBA8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuffer_);

    glGenRenderbuffers(1, &depthBuffer_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer_);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 16, GL_DEPTH24_STENCIL8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer_);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, mainTexture_, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (status == GL_FRAMEBUFFER_COMPLETE) {
        // You have succeeded!
        glClearStencil(0);
        glClearColor(0, 0, 0, 0); // White BG
        glStencilMask(~0);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
        glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    } else {
        // You are all about failure.
        switch(status){

            case GL_FRAMEBUFFER_UNDEFINED:
                 fprintf(stderr, "GL_FRAMEBUFFER_UNDEFINED\n");
                 break;

            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n");
                 break;

            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n");

                break;

            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n");
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n");
                break;

            case GL_FRAMEBUFFER_UNSUPPORTED:
            fprintf(stderr, "GL_FRAMEBUFFER_UNSUPPORTED\n");
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n");
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS\n");
                break;

            default:
            fprintf(stderr, "else\n");
            break;

        }


        exit(1);
    }

    // Set up background
    boost::optional<color> const& bg = m.background();
    if (bg) {
        cleanTexture(mainTexture_, bg->red(), bg->green(), bg->blue(), bg->alpha());
        //cleanTexture(featureTexture_, 255, 255, 255, 0);
    }

    // TODO: Background image
    return true;

}

template <typename T>
agg_renderer<T>::~agg_renderer() {}

template <typename T>
void agg_renderer<T>::start_map_processing(Map const& map)
{
    if(!setupOpenGLDone){
     setupOpenGL(map);
     setupOpenGLDone = true;
    }
     

    MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: Start map processing bbox=" << map.get_current_extent();
    ras_ptr->clip_box(0,0,width_,height_);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);


}

template <typename T>
void agg_renderer<T>::end_map_processing(Map const& )
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBuffer_);
    //glReadBuffer(GL_COLOR_ATTACHMENT0);
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, mainTexture_, 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferBlit_);
    //const GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    //glDrawBuffers(1, drawBuffers);
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blankTexture_, 0);

    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferBlit_);
    // Read buffer
    //glReadBuffer(GL_COLOR_ATTACHMENT0);

    glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, pixmap_.raw_data());

    // printf("accTime: %f\n",  accTime_);
    // printf("accTime2: %f\n",  accTime2_);
    // printf("accTime3: %f\n",  accTime3_);


    glDeleteLists(listIndex_, 1);
    glDeleteLists(rectDrawingIndex_, 1);

    glDeleteTextures(4, textureArray_);

    // Destroy frame buffer object
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &frameBuffer_);
    glDeleteRenderbuffers(1, &colorBuffer_);
    glDeleteRenderbuffers(1, &depthBuffer_);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &frameBufferBlit_);
    glDeleteRenderbuffers(1, &colorBufferBlit_);
    glDeleteRenderbuffers(1, &depthBufferBlit_);

    glDisable(GL_MULTISAMPLE);
    glDisable(GL_BLEND);

    agg::rendering_buffer buf(pixmap_.raw_data(),width_,height_, width_ * 4);
    agg::pixfmt_rgba32 pixf(buf);
    pixf.demultiply();
    MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: End map processing";

    // if(setupOpenGLDone){
    //     setupOpenGLDone = false;
    // }
}

template <typename T>
void agg_renderer<T>::start_layer_processing(layer const& lay, box2d<double> const& query_extent)
{
    MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: Start processing layer=" << lay.name();
    MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: -- datasource=" << lay.datasource().get();
    MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: -- query_extent=" << query_extent;

    if (lay.clear_label_cache())
    {
        detector_->clear();
    }

    query_extent_ = query_extent;
    int buffer_size = lay.buffer_size();
    if (buffer_size != 0 )
    {
        double padding = buffer_size * (double)(query_extent.width()/pixmap_.width());
        double x0 = query_extent_.minx();
        double y0 = query_extent_.miny();
        double x1 = query_extent_.maxx();
        double y1 = query_extent_.maxy();
        query_extent_.init(x0 - padding, y0 - padding, x1 + padding , y1 + padding);
    }

    boost::optional<box2d<double> > const& maximum_extent = lay.maximum_extent();
    if (maximum_extent)
    {
        query_extent_.clip(*maximum_extent);
    }
}

template <typename T>
void agg_renderer<T>::end_layer_processing(layer const&)
{
    MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: End layer processing";
}

template <typename T>
void agg_renderer<T>::start_style_processing(feature_type_style const& st)
{
    MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: Start processing style";
    if (st.comp_op() || st.image_filters().size() > 0 || st.get_opacity() < 1)
    {
        style_level_compositing_ = true;
    }
    else
    {
        style_level_compositing_ = false;
    }

    if (style_level_compositing_)
    {
        if (!internal_buffer_)
        {
            internal_buffer_ = boost::make_shared<buffer_type>(pixmap_.width(),pixmap_.height());
        }
        else
        {
            internal_buffer_->set_background(color(0,0,0,0)); // fill with transparent colour
        }
        current_buffer_ = internal_buffer_.get();

        // OpenGL
        //cleanTexture(styleTexture_, 255, 255, 255, 0);
        //cleanTexture(styleTexture_);
        //currentBackgroundTexture_ = styleTexture_;
    }
    else
    {
        current_buffer_ = &pixmap_;

        // OpenGL
        currentBackgroundTexture_ = mainTexture_;
    }
}

template <typename T>
void agg_renderer<T>::end_style_processing(feature_type_style const& st)
{
    if (style_level_compositing_)
    {
        bool blend_from = false;
        if (st.image_filters().size() > 0)
        {
            blend_from = true;
            mapnik::filter::filter_visitor<image_32> visitor(*current_buffer_);
            BOOST_FOREACH(mapnik::filter::filter_type const& filter_tag, st.image_filters())
            {
                boost::apply_visitor(visitor, filter_tag);
            }
        }

        if (st.comp_op())
        {
            composite(pixmap_.data(),current_buffer_->data(), *st.comp_op(), st.get_opacity(), 0, 0, false);

            //compositeTexture(styleTexture_, mainTexture_, static_cast<composite_mode_e>(*st.comp_op()));
        }
        else if (blend_from || st.get_opacity() < 1)
        {
            composite(pixmap_.data(),current_buffer_->data(), src_over, st.get_opacity(), 0, 0, false);
        }

        // apply any 'direct' image filters
        mapnik::filter::filter_visitor<image_32> visitor(pixmap_);
        BOOST_FOREACH(mapnik::filter::filter_type const& filter_tag, st.direct_image_filters())
        {
            boost::apply_visitor(visitor, filter_tag);
        }
    }

    MAPNIK_LOG_DEBUG(agg_renderer) << "agg_renderer: End processing style";
}

template <typename T>
void agg_renderer<T>::render_marker(pixel_position const& pos, marker const& marker, agg::trans_affine const& tr,
                                    double opacity, composite_mode_e comp_op)
{
    typedef agg::rgba8 color_type;
    typedef agg::order_rgba order_type;
    typedef agg::pixel32_type pixel_type;
    typedef agg::comp_op_adaptor_rgba<color_type, order_type> blender_type; // comp blender
    typedef agg::pixfmt_custom_blend_rgba<blender_type, agg::rendering_buffer> pixfmt_comp_type;
    typedef agg::renderer_base<pixfmt_comp_type> renderer_base;
    typedef agg::renderer_scanline_aa_solid<renderer_base> renderer_type;
    typedef agg::pod_bvector<mapnik::svg::path_attributes> svg_attribute_type;

    ras_ptr->reset();
    ras_ptr->gamma(agg::gamma_power());
    agg::scanline_u8 sl;
    agg::rendering_buffer buf(current_buffer_->raw_data(), width_, height_, width_ * 4);
    pixfmt_comp_type pixf(buf);
    pixf.comp_op(static_cast<agg::comp_op_e>(comp_op));
    renderer_base renb(pixf);

    if (marker.is_vector())
    {
        box2d<double> const& bbox = (*marker.get_vector_data())->bounding_box();
        coord<double,2> c = bbox.center();
        // center the svg marker on '0,0'
        agg::trans_affine mtx = agg::trans_affine_translation(-c.x,-c.y);
        // apply symbol transformation to get to map space
        mtx *= tr;
        mtx *= agg::trans_affine_scaling(scale_factor_);
        // render the marker at the center of the marker box
        mtx.translate(pos.x, pos.y);
        using namespace mapnik::svg;
        vertex_stl_adapter<svg_path_storage> stl_storage((*marker.get_vector_data())->source());
        svg_path_adapter svg_path(stl_storage);
        svg_renderer_agg<svg_path_adapter,
            svg_attribute_type,
            renderer_type,
            pixfmt_comp_type> svg_renderer(svg_path,
                                                   (*marker.get_vector_data())->attributes());

        svg_renderer.render(*ras_ptr, sl, renb, mtx, opacity, bbox);
    }
    else
    {
        double width = (*marker.get_bitmap_data())->width();
        double height = (*marker.get_bitmap_data())->height();
        double cx = 0.5 * width;
        double cy = 0.5 * height;

        if (std::fabs(1.0 - scale_factor_) < 0.001 && tr.is_identity())
        {
            composite(current_buffer_->data(), **marker.get_bitmap_data(),
                      comp_op, opacity,
                      boost::math::iround(pos.x - cx),
                      boost::math::iround(pos.y - cy),
                      false);
        }
        else
        {

            double p[8];
            double x0 = pos.x - 0.5 * width;
            double y0 = pos.y - 0.5 * height;
            p[0] = x0;         p[1] = y0;
            p[2] = x0 + width; p[3] = y0;
            p[4] = x0 + width; p[5] = y0 + height;
            p[6] = x0;         p[7] = y0 + height;

            agg::trans_affine marker_tr;

            marker_tr *= agg::trans_affine_translation(-pos.x,-pos.y);
            marker_tr *= tr;
            marker_tr *= agg::trans_affine_scaling(scale_factor_);
            marker_tr *= agg::trans_affine_translation(pos.x,pos.y);

            marker_tr.transform(&p[0], &p[1]);
            marker_tr.transform(&p[2], &p[3]);
            marker_tr.transform(&p[4], &p[5]);
            marker_tr.transform(&p[6], &p[7]);

            ras_ptr->move_to_d(p[0],p[1]);
            ras_ptr->line_to_d(p[2],p[3]);
            ras_ptr->line_to_d(p[4],p[5]);
            ras_ptr->line_to_d(p[6],p[7]);


            agg::span_allocator<color_type> sa;
            agg::image_filter_bilinear filter_kernel;
            agg::image_filter_lut filter(filter_kernel, false);

            image_data_32 const& src = **marker.get_bitmap_data();
            agg::rendering_buffer marker_buf((unsigned char *)src.getBytes(),
                                             src.width(),
                                             src.height(),
                                             src.width()*4);
            agg::pixfmt_rgba32_pre pixf(marker_buf);
            typedef agg::image_accessor_clone<agg::pixfmt_rgba32_pre> img_accessor_type;
            typedef agg::span_interpolator_linear<agg::trans_affine> interpolator_type;
            typedef agg::span_image_filter_rgba_2x2<img_accessor_type,
                                                    interpolator_type> span_gen_type;
            typedef agg::renderer_scanline_aa_alpha<renderer_base,
                        agg::span_allocator<agg::rgba8>,
                        span_gen_type> renderer_type;
            img_accessor_type ia(pixf);
            interpolator_type interpolator(agg::trans_affine(p, 0, 0, width, height) );
            span_gen_type sg(ia, interpolator, filter);
            renderer_type rp(renb,sa, sg, unsigned(opacity*255));
            agg::render_scanlines(*ras_ptr, sl, rp);
        }
    }
}

template <typename T>
void agg_renderer<T>::painted(bool painted)
{
    pixmap_.painted(painted);
}

template <typename T>
void agg_renderer<T>::debug_draw_box(box2d<double> const& box,
                                     double x, double y, double angle)
{
    agg::rendering_buffer buf(pixmap_.raw_data(), width_, height_, width_ * 4);
    debug_draw_box(buf, box, x, y, angle);
}

template <typename T> template <typename R>
void agg_renderer<T>::debug_draw_box(R& buf, box2d<double> const& box,
                                     double x, double y, double angle)
{
    typedef agg::pixfmt_rgba32 pixfmt;
    typedef agg::renderer_base<pixfmt> renderer_base;
    typedef agg::renderer_scanline_aa_solid<renderer_base> renderer_type;

    agg::scanline_p8 sl_line;
    pixfmt pixf(buf);
    renderer_base renb(pixf);
    renderer_type ren(renb);

    // compute tranformation matrix
    agg::trans_affine tr = agg::trans_affine_rotation(angle).translate(x, y);
    // prepare path
    agg::path_storage pbox;
    pbox.start_new_path();
    pbox.move_to(box.minx(), box.miny());
    pbox.line_to(box.maxx(), box.miny());
    pbox.line_to(box.maxx(), box.maxy());
    pbox.line_to(box.minx(), box.maxy());
    pbox.line_to(box.minx(), box.miny());

    // prepare stroke with applied transformation
    typedef agg::conv_transform<agg::path_storage> conv_transform;
    typedef agg::conv_stroke<conv_transform> conv_stroke;
    conv_transform tbox(pbox, tr);
    conv_stroke sbox(tbox);
    sbox.generator().width(1.0 * scale_factor_);

    // render the outline
    ras_ptr->reset();
    ras_ptr->add_path(sbox);
    ren.color(agg::rgba8(0x33, 0x33, 0xff, 0xcc)); // blue is fine
    agg::render_scanlines(*ras_ptr, sl_line, ren);
}

template <typename T>
void agg_renderer<T>::draw_geo_extent(box2d<double> const& extent, mapnik::color const& color)
{
    box2d<double> box = t_.forward(extent);
    double x0 = box.minx();
    double x1 = box.maxx();
    double y0 = box.miny();
    double y1 = box.maxy();
    unsigned rgba = color.rgba();
    for (double x=x0; x<x1; x++)
    {
        pixmap_.setPixel(x, y0, rgba);
        pixmap_.setPixel(x, y1, rgba);
    }
    for (double y=y0; y<y1; y++)
    {
        pixmap_.setPixel(x0, y, rgba);
        pixmap_.setPixel(x1, y, rgba);
    }
}

template <typename T>
void agg_renderer<T>::setCacheFeatures(std::list<feature_ptr> *featureList) {

    featureList_ = featureList;

}


template <typename T>
void agg_renderer<T>::render_text(int textSize, char text[], double posX, double posY){

 const GLsizei wordLen = textSize;

GLfloat xtranslate[wordLen+1];  // wordLen+1
const GLfloat emScale = 2048;  // match TrueType convention
const GLuint templatePathObject = ~0;
const int numChars = 256;
 GLuint glyphBase;

 glDeletePathsNV(glyphBase, numChars);
 glyphBase = glGenPathsNV(numChars);


    glPathGlyphRangeNV(glyphBase, 
                     GL_FILE_NAME_NV, "DejaVuSans.ttf", 0,
                     0, numChars,
                     GL_SKIP_MISSING_GLYPH_NV, templatePathObject, emScale);

    glPathGlyphRangeNV(glyphBase,
                     GL_STANDARD_FONT_NAME_NV, "Sans", GL_BOLD_BIT_NV,
                     0, numChars,
                     GL_SKIP_MISSING_GLYPH_NV, templatePathObject,
                     emScale);


      
  xtranslate[0] = 0;

  char repeatLast[wordLen+1];
  for(int i = 0;i < wordLen+1;i++){

      if(i < wordLen)
      repeatLast[i] = text[i];
      else repeatLast[i] = text[wordLen-1];

  }

  glGetPathSpacingNV(GL_ACCUM_ADJACENT_PAIRS_NV,
                     wordLen+1, GL_UNSIGNED_BYTE,
                     repeatLast, // repeat last
                                                     // letter twice
                     glyphBase,
                     1.0f, 1.0f,
                     GL_TRANSLATE_X_NV,
                     xtranslate+1);


 glMatrixLoadIdentityEXT(GL_PROJECTION);

 glMatrixOrthoEXT(GL_PROJECTION, 
                   0, width_, height_, 0,
                   -1, 1);


 double shiftX,shiftY;
 double scaleMulY = 196.0;
 double scaleMulX = 195.5;
 //0.006 scale 160

 shiftX = (posX / width_) * width_ * scaleMulX;
 shiftY = ((height_ - posY) / height_) * height_ * scaleMulY;

  glMatrixLoadIdentityEXT(GL_MODELVIEW);

    glMatrixScalefEXT(GL_MODELVIEW,0.005,0.005,0);
    glMatrixTranslatefEXT(GL_MODELVIEW, shiftX, shiftY, 0);



  glStencilFillPathInstancedNV(wordLen, GL_UNSIGNED_BYTE,
                               text,
                               glyphBase,
                               GL_PATH_FILL_MODE_NV, 0xFF,
                               GL_TRANSLATE_X_NV, xtranslate);


  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

   // agg::rgba8 color = agg::rgba8_pre(fill.red(), fill.green(), fill.blue(), int(fill.alpha() * sym.get_opacity()));
   //  glColor4ub(color.r, color.g, color.b, color.a);

      glColor3f(0,0,0);
      

  glCoverFillPathInstancedNV(wordLen, GL_UNSIGNED_BYTE,
                             text,
                             glyphBase,
                             GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                             GL_TRANSLATE_X_NV, xtranslate);


  // if(strokeEnable){
  //       glStencilStrokePathInstancedNV(wordLen,
  //     GL_UNSIGNED_BYTE, word, pathObject_,
  //     1, ~0,  /* Use all stencil bits */
  //     GL_TRANSLATE_X_NV, xtranslate);
  //    glColor4ub(red, green , blue, alpha);
  //   glCoverStrokePathInstancedNV(wordLen,
  //     GL_UNSIGNED_BYTE, word, pathObject_,
  //     GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
  //     GL_TRANSLATE_X_NV, xtranslate);
  // }



}

template <typename T>
void agg_renderer<T>::extractVerticesInCurrentPathStorage(GLfloat vertices[][2], unsigned int &numberOfVertices, GLubyte commands[], unsigned int &numberOfCommands
                                            , double &minX, double &maxX
                                            , double &minY, double &maxY){

        numberOfVertices = 0;
    numberOfCommands = 0;
    minX = 0.0;
    maxX = 0.0;
    minY = 0.0;
    maxY = 0.0;

    double x = 0, y = 0;
    double startX = 0, startY = 0;
    unsigned int currentStatus = 0;
    unsigned int cmd;
    bool initialMin = false;

    // Extract vertices and commands
    while (!agg::is_stop(cmd = pathStorage_.vertex(&x, &y))) {

        if(!initialMin){
            minX = (GLfloat)x;
            minY = (GLfloat)y;
            initialMin = true;
        }


        if (cmd == 79) {
            if (currentStatus == agg::path_cmd_line_to) {
                commands[numberOfCommands++] = GL_CLOSE_PATH_NV;
                currentStatus = cmd;
            }
        } else {
            switch (cmd) {
                case agg::path_cmd_move_to:
                    commands[numberOfCommands++] = GL_MOVE_TO_NV;
                    startX = x;
                    startY = y;
                    currentStatus = agg::path_cmd_move_to;
                    break;
                case agg::path_cmd_line_to:
                    commands[numberOfCommands++] = GL_LINE_TO_NV;
                    currentStatus = agg::path_cmd_line_to;
                    break;
                default:
                    break;
                    // To do for other types
            }

            vertices[numberOfVertices][0] = (GLfloat)x;
            vertices[numberOfVertices][1] = (GLfloat)y;
            numberOfVertices++;

            if(minX > (GLfloat)x){
                minX = (GLfloat)x;
            }
            if(minY > (GLfloat)y){
                minY = (GLfloat)y;
            }
            if(maxX < (GLfloat)x){
                maxX = (GLfloat)x;
            }
            if(maxY < (GLfloat)y){
                maxY = (GLfloat)y;
            }

        }

    }


}




template <typename T>
void agg_renderer<T>::extractVerticesInCurrentPathStorage(GLfloat vertices[][2], unsigned int &numberOfVertices, GLubyte commands[], unsigned int &numberOfCommands) {

    numberOfVertices = 0;
    numberOfCommands = 0;
    double x = 0, y = 0;
    double startX = 0, startY = 0;
    unsigned int currentStatus = 0;
    unsigned int cmd;

    // Extract vertices and commands
    while (!agg::is_stop(cmd = pathStorage_.vertex(&x, &y))) {


        if (cmd == 79) {
            if (currentStatus == agg::path_cmd_line_to) {
                commands[numberOfCommands++] = GL_CLOSE_PATH_NV;
                currentStatus = cmd;
            }
        } else {
            switch (cmd) {
                case agg::path_cmd_move_to:
                    commands[numberOfCommands++] = GL_MOVE_TO_NV;
                    startX = x;
                    startY = y;
                    currentStatus = agg::path_cmd_move_to;
                    break;
                case agg::path_cmd_line_to:
                    commands[numberOfCommands++] = GL_LINE_TO_NV;
                    currentStatus = agg::path_cmd_line_to;
                    break;
                default:
                    break;
                    // To do for other types
            }

            vertices[numberOfVertices][0] = (GLfloat)x;
            vertices[numberOfVertices][1] = (GLfloat)y;
            numberOfVertices++;

        }

    }

}

template <typename T>
GLuint agg_renderer<T>::createProgram(composite_mode_e comp_op) {

    GLint status;
    const char *shaderString = shaderTable[comp_op];

    if (!blendingModeLoaded_[comp_op]) {
        GLuint shaderObj = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(shaderObj, 1, &shaderString, NULL);
        glCompileShader(shaderObj);

        glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &status);

        if (status == GL_FALSE) {
            fprintf(stderr, "Cannot compile the blending shader: %d\n", comp_op);

            GLint logLength;
            glGetShaderiv(shaderObj, GL_INFO_LOG_LENGTH, &logLength);

            if (logLength > 0) {
                GLchar *logString = (GLchar *)malloc(logLength);
                glGetShaderInfoLog(shaderObj, logLength, &logLength, logString);
                fprintf(stderr, "Shader compile log: %s\n", logString);
                free(logString);
            }

            exit(1);
        }

        GLuint programObj = glCreateProgram();
        glAttachShader(programObj, shaderObj);
        glLinkProgram(programObj);

        glGetProgramiv(programObj, GL_LINK_STATUS, &status);

        if (status == GL_FALSE) {
            fprintf(stderr, "Cannot link a program with blending shader: %d\n", comp_op);
            exit(0);
        }

        blendingShader_[comp_op] = shaderObj;
        programs_[comp_op] = programObj;
        blendingModeLoaded_[comp_op] = true;
    }

    return programs_[comp_op];
}

template <typename T>
void agg_renderer<T>::compositeTexture(GLuint src, GLuint dst, composite_mode_e comp_op) {

    if (currentShader_ != comp_op) {
        currentProgram_ = createProgram(comp_op);
        currentShader_  = comp_op;
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, dst, 0);

    //glClearColor(0, 0, 0, 0);
    //glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // glEnable(GL_MULTISAMPLE);

    glDisable(GL_STENCIL_TEST);

    GLuint sampler = 1;
    glGenSamplers(1 , &sampler);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glUseProgram(currentProgram_);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, dst);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, src);

    glActiveTexture(GL_TEXTURE0);

    GLuint dstLocation = glGetUniformLocation(currentProgram_, "dstTexture");
    glUniform1i(dstLocation, 2);
    glBindSampler(2, sampler);

    GLuint srcLocation = glGetUniformLocation(currentProgram_, "srcTexture");
    glUniform1i(srcLocation, 1);
    glBindSampler(1, sampler);

    glCallList(listIndex_);

    glUseProgram(0);

    glDeleteSamplers(1, &sampler);

    glEnable(GL_STENCIL_TEST);

    // glDisable(GL_MULTISAMPLE);
}

template <typename T>
void agg_renderer<T>::cleanTexture(GLuint texture, GLubyte red /* = 0*/, GLubyte green /* = 0*/, GLubyte blue /* = 0*/, GLubyte alpha /* = 0*/) {
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glColor4ub(red, green, blue, alpha);
    glCallList(rectDrawingIndex_);

    glEnable(GL_STENCIL_TEST);
}

template <typename T>
void agg_renderer<T>::checkOpenGLError(const char *desc) {
    printf("%s\n", desc);
    GLenum error = glGetError();
    const GLubyte *errString;
    if (error != GL_NO_ERROR) {
        errString = gluErrorString(error);
        fprintf(stderr, "OpenGL Error: %s\n", errString);
        exit(1);
    }
}

template class agg_renderer<image_32>;
template void agg_renderer<image_32>::debug_draw_box<agg::rendering_buffer>(
                agg::rendering_buffer& buf,
                box2d<double> const& box,
                double x, double y, double angle);
}
