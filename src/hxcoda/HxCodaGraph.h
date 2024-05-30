#pragma once

// ZIB
#include <hxcore/HxCompModule.h>
#include <hxcore/HxPortDoIt.h>

// Local
#include <hxcoda/api.h>
#include <hxcoda/internal/PortCoda.h>


/**
 * @brief HxCodaGraph
 * 
 * This module can be attached to a spatialgraph and makes
 * its vertices and edges available in Coda.
 */
class HXCODA_API HxCodaGraph : public HxCompModule
{
HX_HEADER(HxCodaGraph);

public:

    virtual void update() override;
    virtual void compute() override;

    HxPortDoIt m_portDoIt;
    PortCoda m_portCoda;
    McHandle<HxData> m_lastData;
};

