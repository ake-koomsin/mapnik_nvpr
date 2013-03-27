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
#include <mapnik/symbolizer_helpers.hpp>

// boost
#include <boost/make_shared.hpp>

namespace mapnik {

template <typename T>
void  agg_renderer<T>::process(shield_symbolizer const& sym,
                               mapnik::feature_impl & feature,
                               proj_transform const& prj_trans)
{


    for (std::list<feature_ptr>::iterator f = featureList_->begin(); f != featureList_->end(); f++) {
        feature_ptr featurePtr = *f;

    shield_symbolizer_helper<face_manager<freetype_engine>,
        label_collision_detector4> helper(
            sym, *featurePtr, prj_trans,
            width_, height_,
            scale_factor_,
            t_, font_manager_, *detector_,
            query_extent_);

    text_renderer<T> ren(*current_buffer_,
                         font_manager_,
                         *(font_manager_.get_stroker()),
                         sym.comp_op(),
                         scale_factor_);

    while (helper.next())
    {
        placements_type const& placements = helper.placements();
        for (unsigned int ii = 0; ii < placements.size(); ++ii)
        {
            // get_marker_position returns (minx,miny) corner position,
            // while (currently only) agg_renderer::render_marker newly
            // expects center position;
            // until all renderers and shield_symbolizer_helper are
            // modified accordingly, we must adjust the position here
             pixel_position pos = helper.get_marker_position(placements[ii]);
            // pos.x += 0.5 * helper.get_marker_width();
            // pos.y += 0.5 * helper.get_marker_height();
            // render_marker(pos,
            //               helper.get_marker(),
            //               helper.get_image_transform(),
            //               sym.get_opacity(),
            //               sym.comp_op());


          marker const& marker_ = helper.get_marker();

          image_data_32 const& src = **marker_.get_bitmap_data();
          double width =  src.width();
          double height =  src.height();

          const double shifted = 0;
          double markerX = pos.x + shifted, markerY = height_ - pos.y - shifted - height;



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



  // glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
  // glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);


  //makeFaceTexture(TEXTURE_FACE);
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

  //glEnable(GL_STENCIL_TEST);
  
    //glColor3f(1,1,1);
    glEnable(GL_TEXTURE_2D);
    glPathTexGenNV(GL_TEXTURE0, GL_PATH_OBJECT_BOUNDING_BOX_NV, 2, &data[0][0]);
    glStencilFillPathNV(pathObject_, GL_COUNT_UP_NV, 0x1F);
    glCoverFillPathNV(pathObject_, GL_BOUNDING_BOX_NV);
    glDisable(GL_TEXTURE_2D);

    pathObject_++;


            //Text rendering
            //ren.prepare_glyphs(placements[ii]);
            //ren.render(placements[ii].center);

             const int size = placements[ii].num_nodes();
            int strokeEnable = 0;
            char word[size]; 
            unsigned red,green,blue,alpha;
            double textWidth = 0;
            double textHeight = 0;
            color textColor,strokeColor;
            double opacity = 0.0;


             for (int i = 0; i < placements[ii].num_nodes(); i++)
            {
              char_info_ptr c;
              double x, y, angle;

             placements[ii].vertex(&c, &x, &y, &angle);
             word[i] = (char)c->c;
              textWidth += c->width;
            textHeight = c->height();

              double halo_radius = c->format->halo_radius;

                if (halo_radius > 0.0 && halo_radius < 1024.0){
                  strokeEnable = 1;
                  // red = c->format->halo_fill.red();
                  // green = c->format->halo_fill.green();
                  // blue = c->format->halo_fill.blue();
                  // alpha = c->format->halo_fill.alpha();
                  strokeColor = c->format->halo_fill;

                } 

               textColor = c->format->fill;
               opacity = c->format->text_opacity;
            }

           double posX = placements[ii].center.x, posY = placements[ii].center.y;

           int centerText = textWidth / 2;
           double posTextY = height_ - markerY - (height)/2;
           double posTextX = posX - centerText;

           if(posTextY <= height_ / 2) posTextY -= 5;
           if(posTextX >= width_ / 2) posTextX += 5;


           if(!strokeEnable){
            render_text(size, word, posTextX, posTextY,textColor, opacity);  
           }

           if(strokeEnable){
            render_text(size, word, posTextX, posTextY,textColor,strokeColor,opacity);  
           }




        }
    }

  }
}


template void agg_renderer<image_32>::process(shield_symbolizer const&,
                                              mapnik::feature_impl &,
                                              proj_transform const&);

}
