// STL

// ZIB
#include <hxspreadsheet/internal/HxSpreadSheet.h>
#include <hxspatialgraph/internal/HxSpatialGraph.h>
#include <hxfield/HxUniformLabelField3.h>

// Local
#include <hxcoda/HxCodaEdgeFilter.h>
#include <hxcoda/internal/Coda.h>


HX_INIT_CLASS(HxCodaEdgeFilter, HxCompModule)


HxCodaEdgeFilter::HxCodaEdgeFilter()
    : HxCompModule(HxSpreadSheet::getClassTypeId())
    , m_portDoIt(this, "action", tr("Action"), 1)
    , m_portCoda(this, "coda", tr("Coda"))
    , m_qtContext()
{
    portData.addType(HxSpatialGraph::getClassTypeId());
    portData.addType(HxUniformLabelField3::getClassTypeId());

    auto coda = coda::theCoda();
    QObject::connect(coda.get(), &coda::Coda::edgeSelectionChanged, &m_qtContext, [this](){
        this->compute();
    });
}


HxCodaEdgeFilter::~HxCodaEdgeFilter()
{}


void HxCodaEdgeFilter::update()
{
    // Attach tight to spreadsheets since we only change the selection
    // in the spreadsheet and don't create a new result.
    portData.setTightness(!!hxconnection_cast<HxSpreadSheet>(portData));
}


void HxCodaEdgeFilter::compute()
{
    auto coda = coda::theCoda();

    if(!m_portDoIt.wasHit())
    {
        return;
    }

    McHandle<HxData> filteredData;

    // Filter an attached spreadsheet.
    if(auto input = McHandle<HxSpreadSheet>(hxconnection_cast<HxSpreadSheet>(portData)))
    {
        coda::select(input, coda->edgeSelection());
        filteredData.release();
    }

    // Filter an attached spatialgraph.
    if(auto input = McHandle<HxSpatialGraph>(hxconnection_cast<HxSpatialGraph>(portData)))
    {
        auto filtered = McHandle<HxSpatialGraph>(dynamic_cast<HxSpatialGraph*>(getResult()));
        if(!filtered)
        {
            filtered = HxSpatialGraph::createInstance();
            filtered->composeLabel(input->getLabel(), "coda_filtered");
        }
        coda::filterEdges(filtered, input, coda->edgeSelection());
        filteredData = filtered;
    }

    // Filter an attached label field.
    if(auto input = McHandle<HxUniformLabelField3>(hxconnection_cast<HxUniformLabelField3>(portData)))
    {
        auto filtered = McHandle<HxUniformLabelField3>(dynamic_cast<HxUniformLabelField3*>(getResult()));
        if(!filtered)
        {
            filtered = HxUniformLabelField3::createInstance();
            filtered->lattice().setPrimType(McPrimType::MC_INT32);
            filtered->composeLabel(input->getLabel(), "coda_filtered");
        }
        coda::filter(filtered, input, coda->edgeSelection());
        filteredData = filtered;
    }

    // Set the result.    
    if(filteredData)
    {
        filteredData->touch();
        filteredData->fire();
    }
    setResult(filteredData);
}
