// STL

// ZIB
#include <hxfield/HxUniformScalarField3.h>
#include <hxfield/HxUniformVectorField3.h>
#include <hxspreadsheet/internal/HxSpreadSheet.h>
#include <hxspatialgraph/internal/HxSpatialGraph.h>
#include <hxquant2/internal/HxLabelAnalysis.h>

// Local
#include <hxcoda/HxCodaEdge.h>
#include <hxcoda/internal/Coda.h>


HX_INIT_CLASS(HxCodaEdge, HxCompModule)


HxCodaEdge::HxCodaEdge()
    : HxCompModule(HxSpreadSheet::getClassTypeId())
    , m_portDoIt(this, "action", tr("Action"), 1)
    , m_portCoda(this, "coda", tr("Coda"))
    , m_lastData()
{
    portData.addType(HxSpatialGraph::getClassTypeId());
    portData.addType(HxLabelAnalysis::getClassTypeId());
    portData.addType(HxUniformScalarField3::getClassTypeId());
    portData.addType(HxUniformVectorField3::getClassTypeId());
    portData.setTightness(true);
}


HxCodaEdge::~HxCodaEdge()
{}


void HxCodaEdge::update()
{
    auto coda = coda::theCoda();

    // Check if the module has been connected to another data object.
    // If so, detach from the old one.
    McHandle<HxData> currentData = hxconnection_cast<HxData>(portData);
    if(m_lastData && currentData != m_lastData)
    {
        coda->removeEdgeData(m_lastData);
        m_lastData.release();
    }    

    // Check if the module has been attached to a new data object.
    // If so, attach to the new one.
    if(currentData && currentData != m_lastData)
    {
        if(coda->addEdgeData(currentData))
        {
            m_lastData = currentData;
        }
        else
        {
            m_lastData.release();
            portData.disconnect(true);
        }
    }
}


void HxCodaEdge::compute()
{
    if(m_lastData)
    {
        auto coda = coda::theCoda();
        coda->writeEdgeData(m_lastData.get());
    }
}
