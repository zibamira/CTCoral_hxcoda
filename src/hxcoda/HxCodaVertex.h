#pragma once

// ZIB
#include <hxcore/HxCompModule.h>
#include <hxcore/HxPortDoIt.h>

// Local
#include <hxcoda/api.h>
#include <hxcoda/internal/PortCoda.h>


/**
 * @brief HxCodaVertex
 * 
 * This module makes the content of a spreadsheet 
 * or the vertices of a spatialgraph available in Coda 
 * as vertex dataframe.
 */
class HXCODA_API HxCodaVertex : public HxCompModule
{
HX_HEADER(HxCodaVertex);

public:

    virtual void update() override;
    virtual void compute() override;

    HxPortDoIt m_portDoIt;
    PortCoda m_portCoda;
    McHandle<HxData> m_lastData;
};