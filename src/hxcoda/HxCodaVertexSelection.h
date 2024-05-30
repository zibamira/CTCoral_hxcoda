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
 * @brief HxCodaVertexSelection
 * 
 * This module makes the current Coda vertex selection (indices)
 * available in Amira as a spreadsheet. The spreadsheet can 
 * for example be used in the Codal segmenter module to mask
 * out other labels.
 * 
 * TODO: It would be cool to be able to create this module without
 *       a dummy data object.
 */
class HXCODA_API HxCodaVertexSelection : public HxCompModule
{
HX_HEADER(HxCodaVertexSelection);

public:

    virtual void update() override;
    virtual void compute() override;

    HxPortDoIt m_portDoIt;
    PortCoda m_portCoda;
    QObject m_qtContext;
};

