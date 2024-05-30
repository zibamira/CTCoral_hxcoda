// STL

// ZIB
#include <hxcolor/HxColormap.h>
#include <hxcolor/HxColormap256.h>
#include <hxfield/HxUniformScalarField3.h>
#include <hxfield/HxUniformVectorField3.h>
#include <hxspatialgraph/internal/HxSpatialGraph.h>
#include <hxspreadsheet/internal/HxSpreadSheet.h>
#include <hxquant2/internal/HxLabelAnalysis.h>
#include <hxvolumeviz2/internal/HxVolumeRender2.h>

// Local
#include <hxcoda/HxCodaVertexColormap.h>
#include <hxcoda/internal/Coda.h>


// XXX: Needs to be included last because Inventor included
//      X11 headers that define constants which conflict 
//      with qdir.h enums.
#include <hxlineviewer/HxLineRaycast.h>


HX_INIT_CLASS(HxCodaVertexColormap, HxCompModule)


HxCodaVertexColormap::HxCodaVertexColormap()
    : HxCompModule(HxColormap::getClassTypeId())
    , m_portDoIt(this, "action", tr("Action"), 1)
    , m_portCoda(this, "coda", tr("Coda"))
    , m_lastData()
{
    portData.addType(HxSpatialGraph::getClassTypeId());
    portData.addType(HxUniformScalarField3::getClassTypeId());
    portData.addType(HxUniformVectorField3::getClassTypeId());
    portData.addType(HxLineRaycast::getClassTypeId());
    portData.addType(HxVolumeRender2::getClassTypeId());
    portData.setTightness(true);
}


HxCodaVertexColormap::~HxCodaVertexColormap()
{}


void HxCodaVertexColormap::update()
{}


void HxCodaVertexColormap::compute()
{
    auto coda = coda::theCoda();
    coda->writeVertexColormap(portData);
}
