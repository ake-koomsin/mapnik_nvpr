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
#include <mapnik/symbolizer_helpers.hpp>
#include <mapnik/graphics.hpp>

namespace mapnik {

template <typename T>
void agg_renderer<T>::process(text_symbolizer const& sym,
                              mapnik::feature_impl & feature,
                              proj_transform const& prj_trans)
{

    for (std::list<feature_ptr>::iterator f = featureList_->begin(); f != featureList_->end(); f++) {
        feature_ptr featurePtr = *f;


    text_symbolizer_helper<face_manager<freetype_engine>,
        label_collision_detector4> helper(
            sym, *featurePtr, prj_trans,
            width_,height_,
            scale_factor_,
            t_, font_manager_, *detector_,
            query_extent_);


    text_renderer<T> ren(*current_buffer_,
                         font_manager_,
                         *(font_manager_.get_stroker()),
                         sym.comp_op(),
                         scale_factor_);

    double prevX = 0.0, prevY = 0.0;
    double prevWidth = 0.0, prevHeight = 0.0;


    while (helper.next()) 
    {
        placements_type const& placements = helper.placements();
        for (unsigned int ii = 0; ii < placements.size(); ++ii)
        {
            //ren.prepare_glyphs(placements[ii]);
            //ren.render(placements[ii].center);
           const int size = placements[ii].num_nodes();
            int strokeEnable = 0;
            char word[size]; 
            // unsigned red,green,blue,alpha;
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
           double posTextY = posY;
           double posTextX = posX - centerText;

           bool collide = false;
           if(posX >= prevX && posX <= prevX+prevWidth && posY >= prevY && posY <= prevY+prevWidth){
            collide = true;
            }
            
           if(posTextY <= height_ / 2) posTextY -= 5;
           if(posTextX >= width_ / 2) posTextX += 5;

          if(!collide && !strokeEnable){

            render_text(size, word, posTextX, posTextY,textColor, opacity);  
          }

          if(!collide && strokeEnable){

            render_text(size, word, posTextX, posTextY,textColor,strokeColor,opacity);  
          }

  


           prevX = posX;
           prevY = posY;
           prevWidth = textWidth;
           prevHeight = textHeight;
            
        }
    }



    }

    
}

template void agg_renderer<image_32>::process(text_symbolizer const&,
                                              mapnik::feature_impl &,
                                              proj_transform const&);

}
