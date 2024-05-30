#pragma once

// ZIB
#include <hxcore/HxCompModule.h>
#include <hxcore/HxPortDoIt.h>

// Local
#include <hxcoda/api.h>
#include <hxcoda/internal/PortCoda.h>


/**
 * @brief HxCodaEdge
 * 
 * This module makes the content of a spreadsheet 
 * or the edges of a spatialgraph available in Coda 
 * as edge dataframe.
 */
class HXCODA_API HxCodaEdge : public HxCompModule
{
HX_HEADER(HxCodaEdge);

public:

    virtual void update() override;
    virtual void compute() override;

    HxPortDoIt m_portDoIt;
    PortCoda m_portCoda;
    McHandle<HxData> m_lastData;
};