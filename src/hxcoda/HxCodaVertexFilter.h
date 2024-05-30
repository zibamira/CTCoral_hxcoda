#pragma once

// Qt
#include <QScopedPointer>
#include <QObject>

// ZIB
#include <hxcore/HxCompModule.h>
#include <hxcore/HxPortDoIt.h>

// Local
#include <hxcoda/api.h>
#include <hxcoda/internal/PortCoda.h>


/**
 * @brief HxCodaVertexFilter
 * 
 * This module applies the current edge selection in Coda
 * as a filter to the attached spatialgraph or the rows
 * of a spreadsheet.
 */
class HXCODA_API HxCodaVertexFilter : public HxCompModule
{
HX_HEADER(HxCodaVertexFilter);

public:

    virtual void update() override;
    virtual void compute() override;

    HxPortDoIt m_portDoIt;
    PortCoda m_portCoda;
    QObject m_qtContext;
};

