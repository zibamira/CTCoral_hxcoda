#pragma once

// ZIB
#include <hxcore/HxCompModule.h>
#include <hxcore/HxPortDoIt.h>

// Local
#include <hxcoda/api.h>
#include <hxcoda/internal/PortCoda.h>


/**
 * @brief HxCodaEdgeColormap
 * 
 * This module makes the colormap of the attached edge data
 * (field, graph, colormap or renderer) available in Coda as 
 * edge colormap.
 */
class HXCODA_API HxCodaEdgeColormap : public HxCompModule
{
HX_HEADER(HxCodaEdgeColormap);

public:

    virtual void update() override;
    virtual void compute() override;

    HxPortDoIt m_portDoIt;
    PortCoda m_portCoda;
    McHandle<HxData> m_lastData;
};