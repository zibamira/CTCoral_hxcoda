// STL

// ZIB
#include <hxspreadsheet/internal/HxSpreadSheet.h>
#include <hxspatialgraph/internal/HxSpatialGraph.h>
#include <hxfield/HxUniformLabelField3.h>

// Local
#include <hxcoda/HxCodaVertexSelection.h>
#include <hxcoda/internal/Coda.h>


HX_INIT_CLASS(HxCodaVertexSelection, HxCompModule)


HxCodaVertexSelection::HxCodaVertexSelection()
    : HxCompModule(HxSpreadSheet::getClassTypeId())
    , m_portDoIt(this, "action", tr("Action"), 1)
    , m_portCoda(this, "coda", tr("Coda"))
    , m_qtContext()
{
    // Attach tight to the "dummy" data object since it should 
    // not be of real other use than being a dummy.
    portData.setTightness(true);

    auto coda = coda::theCoda();
    QObject::connect(coda.get(), &coda::Coda::vertexSelectionChanged, &m_qtContext, [this](){
        this->compute();
    });
}


HxCodaVertexSelection::~HxCodaVertexSelection()
{}


void HxCodaVertexSelection::update()
{}


void HxCodaVertexSelection::compute()
{
    auto coda = coda::theCoda();

    const auto& vertexSelection = coda->vertexSelection();
    const int nrows = static_cast<int>(vertexSelection.size());

    // This module only outputs data. So we can just ignore all inputs.
    McHandle<HxSpreadSheet> filteredData = dynamic_cast<HxSpreadSheet*>(getResult());
    if(!filteredData)
    {
        filteredData = HxSpreadSheet::createInstance();
        filteredData->setLabel("CodaVertexSelection");
    }    
    filteredData->clear();
    filteredData->setNumRows(nrows);
    
    filteredData->addColumn("selected", HxSpreadSheet::Column::INT);
    const int icol = filteredData->findColumn("selected", HxSpreadSheet::Column::INT);

    // Populate the selection column.
    for(int irow = 0; irow < nrows; ++irow)
    {
        const float selected = vertexSelection[irow] ? 1.0 : 0.0;
        filteredData->column(icol)->setValue(irow, selected);
    }
    
    // Set the result.
    if(filteredData)
    {
        filteredData->touch();
        filteredData->fire();
    }
    setResult(filteredData);
}
