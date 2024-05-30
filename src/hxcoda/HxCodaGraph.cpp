// STL

// ZIB
#include <hxspatialgraph/internal/HxSpatialGraph.h>

// Local
#include <hxcoda/HxCodaGraph.h>
#include <hxcoda/internal/Coda.h>


HX_INIT_CLASS(HxCodaGraph, HxCompModule)


HxCodaGraph::HxCodaGraph()
    : HxCompModule(HxSpatialGraph::getClassTypeId())
    , m_portDoIt(this, "action", tr("Action"), 1)
    , m_portCoda(this, "coda", tr("Coda"))
    , m_lastData()
{
    portData.setTightness(true);
}


HxCodaGraph::~HxCodaGraph()
{}


void HxCodaGraph::update()
{
    auto coda = coda::theCoda();

    // Check if the module has been connected to another data object.
    // If so, detach from the old one.
    McHandle<HxData> currentData = hxconnection_cast<HxData>(portData);
    if(m_lastData && currentData != m_lastData)
    {
        coda->removeVertexData(m_lastData);
        coda->removeEdgeData(m_lastData);
        m_lastData.release();
    }    

    // Check if the module has been attached to a new data object.
    // If so, attach to the new one.
    if(currentData && currentData != m_lastData)
    {
        if(coda->addVertexData(currentData) && coda->addEdgeData(currentData))
        {
            m_lastData = currentData;
        }
        else
        {
            coda->removeVertexData(m_lastData);
            coda->removeEdgeData(m_lastData);
            m_lastData.release();
            portData.disconnect(true);
        }
    }
}


void HxCodaGraph::compute()
{    
    if(m_lastData)
    {
        auto coda = coda::theCoda();
        coda->writeVertexData(m_lastData.get());
        coda->writeEdgeData(m_lastData.get());
    }
}
