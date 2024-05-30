#pragma once

// ZIB
#include <hxcore/HxCompModule.h>
#include <hxcore/HxPortDoIt.h>

// Local
#include <hxcoda/api.h>
#include <hxcoda/internal/PortCoda.h>


/**
 * @brief HxCodaVertexColormap
 * 
 * This module makes the colormap of the attached vertex data
 * (field, graph, colormap or renderer) available in Coda as 
 * vertex colormap.
 */
class HXCODA_API HxCodaVertexColormap : public HxCompModule
{
HX_HEADER(HxCodaVertexColormap);

public:

    virtual void update() override;
    virtual void compute() override;

    HxPortDoIt m_portDoIt;
    PortCoda m_portCoda;
    McHandle<HxData> m_lastData;
};