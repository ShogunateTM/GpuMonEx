//
//  statschart.hpp
//  GpuMonEx
//
//  Created by Aeroshogun on 4/17/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#ifndef statschart_hpp
#define statschart_hpp

#include <stdio.h>
#include <wx/wx.h>
#include <mathplot.h>


namespace gpumonex
{
    namespace wx
    {
        class wxGpuUsage : public mpFXYVector
        {
        public:
            wxGpuUsage( int usage ) : mpFXYVector( wxT( "Gpu Usage" ) ) 
            {
                
            }
            
            /*virtual double GetY( double x )
            {
                return 50;
            }*/

            void AddData( float x, float y, std::vector<double>& xs, std::vector<double>& ys )
            {
                // Check if the data vectora are of the same size
                if (xs.size() != ys.size()) {
                    wxLogError(_("wxMathPlot error: X and Y vector are not of the same length!"));
                    return;
                }

                //After a certain number of points implement a FIFO buffer
                //As plotting too many points can cause missing data
                if (x > 300)
                {
                    xs.erase(xs.begin());
                    ys.erase(ys.begin());
                }



                //Add new Data points at the end
                xs.push_back(x);
                ys.push_back(y);


                // Copy the data:
                m_xs = xs;
                m_ys = ys;

                // Update internal variables for the bounding box.
                if (xs.size()>0)
                {
                    m_minX  = xs[0];
                    m_maxX  = xs[0];
                    m_minY  = ys[0];
                    m_maxY  = ys[0];

                    std::vector<double>::const_iterator  it;

                    for (it=xs.begin();it!=xs.end();it++)
                    {
                        if (*it<m_minX) m_minX=*it;
                        if (*it>m_maxX) m_maxX=*it;
                    }
                    for (it=ys.begin();it!=ys.end();it++)
                    {
                        if (*it<m_minY) m_minY=*it;
                        if (*it>m_maxY) m_maxY=*it;
                    }
                    m_minX-=0.5f;
                    m_minY-=0.5f;
                    m_maxX+=0.5f;
                    m_maxY+=0.5f;
                }
                else
                {
                    m_minX  = -1;
                    m_maxX  = 1;
                    m_minY  = -1;
                    m_maxY  = 1;
                }
            }

        };
        
        class MySIN : public mpFX
        {
            double m_freq, m_amp;
        public:
            MySIN(double freq, double amp) : mpFX( wxT("f(x) = SIN(x)"), mpALIGN_LEFT) { m_freq=freq; m_amp=amp; m_drawOutsideMargins = false; }
            virtual double GetY( double x ) { return m_amp * sin(x/6.283185/m_freq); }
            virtual double GetMinY() { return -m_amp; }
            virtual double GetMaxY() { return  m_amp; }
        };
    }
}

#endif /* statschart_hpp */
