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
        class wxGpuUsage : public mpFX
        {
        public:
            wxGpuUsage( int usage ) : mpFX( wxT( "Gpu Usage" ) ) {}
            
            virtual double GetY( double x )
            {
                return 50;
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
