// STL
#include <iostream>
#include <iomanip>

// Qt
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QFileSystemWatcher>

// ZIB
#include <hxcolor/HxColormap.h>
#include <hxcolor/HxColormap256.h>
#include <hxcore/HxApplication.h>
#include <hxcore/HxObjectPool.h>
#include <hxcore/HxPort.h>
#include <hxcore/HxPortDoIt.h>
#include <hxcore/HxPortFilename.h>
#include <hxcore/HxResource.h>
#include <hxfield/HxUniformScalarField3.h>
#include <hxfield/HxUniformVectorField3.h>
#include <hxspreadsheet/internal/HxReadCSV.h>
#include <hxspatialgraphspreadsheet/sgtable.h>
#include <hxspatialgraphspreadsheet/ndtable.h>
#include <hxquant2/internal/HxLabelAnalysis.h>
#include <hxquant2custom/internal/HxConvertAnalysis.h>
#include <hxvolumeviz2/internal/HxVolumeRender2.h>

// Local
#include <hxcoda/internal/Coda.h>


// XXX: Needs to be included last because Inventor included
//      X11 headers that define constants which conflict 
//      with qdir.h enums.
#include <hxlineviewer/HxLineRaycast.h>


namespace coda
{
    

/**
 * Returns the template path for the shared directory with Coda.
 * The path name is based on the name of the current Amira project.
 */
static QString temporaryDirectoryTemplatePath()
{    
    const QString projectPath = theObjectPool->getNetworkName();
    const QString projectName = QFileInfo(projectPath).baseName();
    const QString dataDirectoryName = QString("amira_coda_%1_XXXXXX").arg(projectName);
    const QString templatePath = QDir::temp().absoluteFilePath(dataDirectoryName);  
    return templatePath;
}


/**
 * Saves the Amira field as Numpy ``*.npy`` or ``*.npz`` file at the given
 * path.
 * 
 * XXX: At time of writing this, Amira did not support the *.npy format
 *      directly.
 *      So I created the ``CodaLoadNumpy.pyscro`` and ``CodaSaveNumpy.pyscro``
 *      script objects which make use of the already existing Python bindings.
 *      The script objects are part of the package and used here to do the 
 *      actual lifting. 
 *      This has also the benefit, that I can also save and load .npy files 
 *      in Amira without having to enter the Python console everytime I need
 *      to. 
 *      As a reminder: When Amira finally supports loading and saving .npy
 *      files directly, you can remove this function with the proper 
 *      core io functions.
 */
static bool saveAsNpy(QString path, HxObject* data)
{
    // Lookup the Python script object.
    HxObjectInfo* info = HxResource::findObject("Coda Save Numpy");
    if(info == nullptr)
    {
        qWarning() << "Failed to locate the 'CodaSaveNumpy' Python script object.";
        return false;
    }

    // Create the Amira module.
    McHandle<HxObject> pyscro_object = HxResource::createObject(info);
    if(!pyscro_object)
    {
        qWarning() << "Failed to create an instance of the 'CodaSaveNumpy' Python script object.";
        return false;
    }

    McHandle<HxCompModule> pyscro = dynamic_cast<HxCompModule*>(pyscro_object.get());
    if(!pyscro)
    {
        qWarning() << "Failed to create an instance of the 'CodaSaveNumpy' Python script object.";
        return false;
    }

    // Connect the input array.
    HxConnection* portData = &pyscro->portData;
    portData->connect(data);
    
    // Set the output file path.
    HxPortFilename* portPath = dynamic_cast<HxPortFilename*>(pyscro->getPort("path"));
    if(!portPath)
    {
        qWarning() << "Failed to set the file path in the 'Coda Load Numpy' Python script object.";
        return false;
    }
    portPath->setFilename(path);
    
    // Hit the do it button.
    HxPortDoIt* portDoIt = dynamic_cast<HxPortDoIt*>(pyscro->getPort("apply"));
    if(!portDoIt)
    {
        qWarning() << "Failed to hit the 'apply' button in the 'Coda Load Numpy' Python script object.";
        return false;
    }
    portDoIt->hit();
    
    // Save the array.
    pyscro->compute();
    return true;
}


Coda::Coda(QObject* parent)
    : QObject(parent)
    , m_data_directory(temporaryDirectoryTemplatePath())
    , m_process(nullptr)
    , m_watcher(nullptr)
    , m_edge_data_to_path()
    , m_vertex_data_to_path()
    , m_path_to_data()
    , m_coda_vertex_selection()
    , m_coda_vertex_selection_csv(nullptr)
    , m_coda_vertex_selection_timer(nullptr)
    , m_coda_edge_selection()
    , m_coda_edge_selection_csv(nullptr)
    , m_coda_edge_selection_timer(nullptr)
    , m_coda_vertex_colormap(nullptr)
    , m_coda_edge_colormap(nullptr)
{
    m_process = new CodaProcess(m_data_directory.path());

    // Watch the "coda_selection*.csv" selection files, 
    // that is, if they already exist.
    //
    // Similarly, check if files "coda_colormap_*.csv" files
    // already exist.
    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(m_data_directory.path());

    if(QFileInfo(vertexSelectionPath()).exists())
    {
        readVertexSelection();
        m_watcher->addPath(vertexSelectionPath());
    }
    if(QFileInfo(edgeSelectionPath()).exists())
    {
        readEdgeSelection();
        m_watcher->addPath(edgeSelectionPath());
    }
    if(QFileInfo(vertexColormapPath()).exists())
    {
        readVertexColormap();
        m_watcher->addPath(vertexColormapPath());
    }
    if(QFileInfo(edgeColormapPath()).exists())
    {
        readEdgeColormap();
        m_watcher->addPath(edgeColormapPath());
    }

    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &Coda::on_watcher_fileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &Coda::on_watcher_directoryChanged);

    // Create the timer for delaying/throttling the reloading 
    // of the current coda selections and colormaps.
    m_coda_vertex_selection_timer = new QTimer(this);
    m_coda_edge_selection_timer = new QTimer(this);
    m_coda_vertex_colormap_timer = new QTimer(this);
    m_coda_edge_colormap_timer = new QTimer(this);

    connect(m_coda_vertex_selection_timer, &QTimer::timeout, this, &Coda::readVertexSelection);
    connect(m_coda_edge_selection_timer, &QTimer::timeout, this, &Coda::readEdgeSelection);
    connect(m_coda_vertex_colormap_timer, &QTimer::timeout, this, &Coda::readVertexColormap);
    connect(m_coda_edge_colormap_timer, &QTimer::timeout, this, &Coda::readEdgeColormap);
}


Coda::~Coda()
{
    delete m_process;
    delete m_watcher;
    delete m_coda_vertex_selection_timer;
    delete m_coda_edge_selection_timer;
}    


bool Coda::addVertexData(HxData* data)
{
    // The data object is already synchronized.
    if(m_vertex_data_to_path.contains(data))
    {
        return false;
    }

    // The data type is not supported.
    if(
        !dynamic_cast<HxSpreadSheet*>(data) 
        && !dynamic_cast<HxSpatialGraph*>(data)
        && !dynamic_cast<HxLabelAnalysis*>(data)
        && !dynamic_cast<HxUniformScalarField3*>(data)
        && !dynamic_cast<HxUniformVectorField3*>(data)
    ) {
        return false;
    }

    QString filename = QString("vertex_%1.csv").arg(data->getLabel());
    QString path = QDir(m_data_directory.path()).absoluteFilePath(filename);

    m_vertex_data_to_path[data] = path;
    m_path_to_data[path] = data;

    writeVertexData(data);
    return true;
}


void Coda::removeVertexData(HxData* data)
{
    // Nothing to do since the data is not synchronized.
    if(!m_vertex_data_to_path.remove(data))
    {
        return;
    }

    QString path = m_vertex_data_to_path.take(data);
    m_path_to_data.remove(path);

    QFile::remove(path);
    return;
}


void Coda::writeVertexData(HxData* data)
{
    // Nothing to do. The data object has not been added to Coda yet.
    if(!m_vertex_data_to_path.contains(data))
    {
        return;
    }

    const QString path = m_vertex_data_to_path[data];

    // spreadsheet?
    if(HxSpreadSheet* spreadsheet = dynamic_cast<HxSpreadSheet*>(data))
    {
        spreadsheet->saveCsv(path.toLocal8Bit());
    }
    // spatial graph nodes?
    else if(HxSpatialGraph* graph = dynamic_cast<HxSpatialGraph*>(data))
    {
        McHandle<HxSpreadSheet> spreadsheet = ndtable::createNodeSpreadSheet(graph);
        spreadsheet->saveCsv(path.toLocal8Bit());
    }
    // label or image analysis?
    else if(HxLabelAnalysis* analysis = dynamic_cast<HxLabelAnalysis*>(data))
    {
        McHandle<HxConvertAnalysis> convert_analysis = HxConvertAnalysis::createInstance();
        convert_analysis->portData.connect(analysis);
        convert_analysis->portDoIt.hit();
        convert_analysis->compute();

        McHandle<HxSpreadSheet> spreadsheet = dynamic_cast<HxSpreadSheet*>(convert_analysis->getResult());
        spreadsheet->saveCsv(path.toLocal8Bit());
    }    
    // uniform field?
    else if(dynamic_cast<HxUniformScalarField3*>(data) || dynamic_cast<HxUniformVectorField3*>(data))
    {
        saveAsNpy(path, data);
    }
}


bool Coda::addEdgeData(HxData* data)
{    
    // The data object is already synchronized.
    if(m_edge_data_to_path.contains(data))
    {
        return false;
    }

    // The data type is not supported.
    if(
        !dynamic_cast<HxSpreadSheet*>(data) 
        && !dynamic_cast<HxSpatialGraph*>(data)
        && !dynamic_cast<HxLabelAnalysis*>(data)
        && !dynamic_cast<HxUniformScalarField3*>(data)
        && !dynamic_cast<HxUniformVectorField3*>(data)
    ){
        return false;
    }

    QString filename = QString("edge_%1.csv").arg(data->getLabel());
    QString path = QDir(m_data_directory.path()).absoluteFilePath(filename);

    m_edge_data_to_path[data] = path;
    m_path_to_data[path] = data;

    writeEdgeData(data);
    return true;
}


void Coda::removeEdgeData(HxData* data)
{
    // Nothing to do since the data is not synchronized.
    if(!m_edge_data_to_path.remove(data))
    {
        return;
    }

    QString path = m_edge_data_to_path.take(data);
    m_path_to_data.remove(path);

    QFile::remove(path);
    return;
}


void Coda::writeEdgeData(HxData* data)
{
    // Nothing to do. The data object has not been added to Coda yet.
    if(!m_edge_data_to_path.contains(data))
    {
        return;
    }

    const QString path = m_edge_data_to_path[data];

    // spreadsheet?
    if(HxSpreadSheet* spreadsheet = dynamic_cast<HxSpreadSheet*>(data))
    {
        spreadsheet->saveCsv(path.toLocal8Bit());
    }
    // spatial graph nodes?
    else if(HxSpatialGraph* graph = dynamic_cast<HxSpatialGraph*>(data))
    {
        McHandle<HxSpreadSheet> spreadsheet = sgtable::mergedSegmentsNodes(graph);
        spreadsheet->saveCsv(path.toLocal8Bit());
    }
    // label or image analysis?
    else if(HxLabelAnalysis* analysis = dynamic_cast<HxLabelAnalysis*>(data))
    {
        McHandle<HxConvertAnalysis> convert_analysis = HxConvertAnalysis::createInstance();
        convert_analysis->portData.connect(analysis);
        convert_analysis->portDoIt.hit();
        convert_analysis->compute();

        McHandle<HxSpreadSheet> spreadsheet = dynamic_cast<HxSpreadSheet*>(convert_analysis->getResult());
        spreadsheet->saveCsv(path.toLocal8Bit());
    }    
    // uniform field?
    else if(dynamic_cast<HxUniformScalarField3*>(data) || dynamic_cast<HxUniformVectorField3*>(data))
    {
        saveAsNpy(path, data);
    }
}


QString Coda::vertexSelectionPath()
{
    return QDir(m_data_directory.path()).absoluteFilePath("coda_vertex_selection.csv");
}


void Coda::readVertexSelection()
{
    loadCodaSelection(
        m_coda_vertex_selection, m_coda_vertex_selection_csv, vertexSelectionPath()
    );
    emit vertexSelectionChanged();
}


const std::vector<bool>& Coda::vertexSelection() const
{
    return m_coda_vertex_selection;
}


QString Coda::edgeSelectionPath()
{
    return QDir(m_data_directory.path()).absoluteFilePath("coda_edge_selection.csv");
}


void Coda::readEdgeSelection()
{
    loadCodaSelection(
        m_coda_edge_selection, m_coda_edge_selection_csv, edgeSelectionPath()
    );
    emit edgeSelectionChanged();
}


const std::vector<bool>& Coda::edgeSelection() const
{
    return m_coda_edge_selection;
}


QString Coda::vertexColormapPath()
{
    return QDir(m_data_directory.path()).absoluteFilePath("coda_vertex_colormap.csv");
}


bool Coda::readVertexColormap()
{
    // Try to read the colormap.
    QString path = vertexColormapPath();
    McHandle<HxSpreadSheet> spreadsheet = HxSpreadSheet::createInstance();    
    if(!readCSVDataToSpreadSheet(path.toLocal8Bit(), spreadsheet))
    {
        return false;
    }

    // Check if a colormap has already been created.
    if(!m_coda_vertex_colormap)
    {
        m_coda_vertex_colormap = HxColormap256::createInstance();
        m_coda_vertex_colormap->setLabel("Coda_Vertex_Colormap");
        m_coda_vertex_colormap->setLabelField(true);

        const bool isHideNewModules = theObjectPool->isHideNewModules();
        theObjectPool->setHideNewModules(true);
        theObjectPool->addObject(m_coda_vertex_colormap, true);
        theObjectPool->setHideNewModules(isHideNewModules);
    }

    // Convert the spreadsheet into a colormap.
    if(!colormapFromSpreadSheet(m_coda_vertex_colormap, spreadsheet))
    {
        return false;
    }
    return true;
}


bool Coda::writeVertexColormap(HxConnection& connection)
{
    QString path = QDir(m_data_directory.path()).absoluteFilePath("amira_vertex_colormap.csv");

    McHandle<HxColormap> colormap = colormapVertices(connection);
    if(!colormap)
    {
        return false;
    }

    McHandle<HxSpreadSheet> spreadsheet = HxSpreadSheet::createInstance();
    colormapToSpreadSheet(spreadsheet, colormap);

    if(!spreadsheet->saveCsv(path.toLocal8Bit()))
    {
        return false;
    }
    return true;
}


QString Coda::edgeColormapPath()
{
    return QDir(m_data_directory.path()).absoluteFilePath("coda_edge_colormap.csv");
}


bool Coda::readEdgeColormap()
{
    // Try to read the colormap.
    QString path = edgeColormapPath();
    McHandle<HxSpreadSheet> spreadsheet = HxSpreadSheet::createInstance();    
    if(!readCSVDataToSpreadSheet(path.toLocal8Bit(), spreadsheet))
    {
        return false;
    }

    // Check if a colormap has already been created.
    if(!m_coda_edge_colormap)
    {
        m_coda_edge_colormap = HxColormap256::createInstance();
        m_coda_edge_colormap->setLabel("Coda_Edge_Colormap");
        m_coda_edge_colormap->setLabelField(true);        

        const bool isHideNewModules = theObjectPool->isHideNewModules();
        theObjectPool->setHideNewModules(true);
        theObjectPool->addObject(m_coda_edge_colormap, true);
        theObjectPool->setHideNewModules(isHideNewModules);
    }

    // Convert the spreadsheet into a colormap.
    if(!colormapFromSpreadSheet(m_coda_edge_colormap, spreadsheet))
    {
        return false;
    }
    return true;
}


bool Coda::writeEdgeColormap(HxConnection& connection)
{
    QString path = QDir(m_data_directory.path()).absoluteFilePath("amira_edge_colormap.csv");

    McHandle<HxColormap> colormap = colormapEdges(connection);
    if(!colormap)
    {
        return false;
    }

    McHandle<HxSpreadSheet> spreadsheet = HxSpreadSheet::createInstance();
    colormapToSpreadSheet(spreadsheet, colormap);

    if(!spreadsheet->saveCsv(path.toLocal8Bit()))
    {
        return false;
    }
    return true;
}


CodaProcess* Coda::process()
{
    return m_process;
}


QString Coda::dataDirectory()
{
    return m_data_directory.path();
}


void Coda::loadCodaSelection(
    std::vector<bool>& selection, 
    McHandle<HxSpreadSheet>& spreadsheet,
    const QString path
) {
    // Read the csv spreadsheet.
    if(!spreadsheet)
    {
        spreadsheet = HxSpreadSheet::createInstance();
    }
    readCSVDataToSpreadSheet(path.toLocal8Bit(), spreadsheet);

    // Find the column containing the selection mask.  
    const int icol = spreadsheet->findColumn("selected", HxSpreadSheet::Column::INT);
    const HxSpreadSheet::Column* column = icol >= 0 ? spreadsheet->column(icol) : nullptr;

    // Convert the selection to a vector.
    if(column != nullptr)
    {
        const int nrows = spreadsheet->nRows();
        selection.resize(nrows);
        for(int irow = 0; irow < nrows; ++irow)
        {
            selection[irow] = column->intValue(irow);
        }
    }
    else
    {
        selection.clear();
    }
}


void Coda::on_watcher_fileChanged(const QString& path)
{
    if(path == vertexSelectionPath())
    {
        rescheduleReadVertexSelection();
    }
    if(path == edgeSelectionPath())
    {
        rescheduleReadEdgeSelection();
    }
    if(path == vertexColormapPath())
    {
        rescheduleReadVertexColormap();
    }
    if(path == edgeColormapPath())
    {
        rescheduleReadEdgeColormap();
    }
}


void Coda::on_watcher_directoryChanged(const QString& path)
{
    // vertexSelectionPath()
    {
        const QString path = vertexSelectionPath();
        if(QFileInfo(path).exists() && !m_watcher->files().contains(path))
        {
            rescheduleReadVertexSelection();
            m_watcher->addPath(path);
        }
    }

    // edgeSelectionPath()
    {
        const QString path = edgeSelectionPath();
        if(QFileInfo(path).exists() && !m_watcher->files().contains(path))
        {
            rescheduleReadEdgeSelection();
            m_watcher->addPath(path);
        }
    }

    // vertexColormapPath()
    {
        const QString path = vertexColormapPath();
        if(QFileInfo(path).exists() && !m_watcher->files().contains(path))
        {
            rescheduleReadVertexColormap();
            m_watcher->addPath(path);
        }
    }

    // edgeColormapPath()
    {
        const QString path = edgeColormapPath();
        if(QFileInfo(path).exists() && !m_watcher->files().contains(path))
        {
            rescheduleReadEdgeColormap();
            m_watcher->addPath(path);
        }
    }
}


void Coda::rescheduleReadVertexSelection()
{
    m_coda_vertex_selection_timer->stop();
    m_coda_vertex_selection_timer->setSingleShot(true);
    m_coda_vertex_selection_timer->start(800);
}


void Coda::rescheduleReadEdgeSelection()
{
    m_coda_edge_selection_timer->stop();
    m_coda_edge_selection_timer->setSingleShot(true);
    m_coda_edge_selection_timer->start(800);
}


void Coda::rescheduleReadVertexColormap()
{
    m_coda_vertex_colormap_timer->stop();
    m_coda_vertex_colormap_timer->setSingleShot(true);
    m_coda_vertex_colormap_timer->start(800);    
}


void Coda::rescheduleReadEdgeColormap()
{
    m_coda_edge_colormap_timer->stop();
    m_coda_edge_colormap_timer->setSingleShot(true);
    m_coda_edge_colormap_timer->start(800);    
}


std::shared_ptr<Coda> theCoda()
{
    static std::shared_ptr<Coda> coda;
    if(!coda)
    {
        coda = std::make_shared<Coda>();
    }
    return coda;
}


void select(
    HxSpreadSheet* input, 
    const std::vector<bool>& selection
)
{
    const int ntables = input->nTables();
    const int nselection = static_cast<int>(selection.size());

    // Apply the selection to all tables in the spreadsheet.
    // Rows with indices exceeding the *selection* size cannot be selected
    // and are ignored.
    for(int itable = 0; itable < ntables; ++itable)
    {
        const int nrows = input->nRows();

        // Convert the selection to an index set.
        auto indices = McDArray<int>();
        for(int irow = 0; irow < nrows; ++irow)
        {
            if(irow < nselection && selection[irow])
            {
                indices.push_back(irow);
            }
        }

        // Select the rows in the current table.
        input->setRowSelection(indices, itable);
    }
}


void filterVertices(
    HxSpatialGraph* result, 
    HxSpatialGraph* input, 
    const std::vector<bool>& selection
) {
    const int nvertices = input->getNumVertices();
    const int nedges = input->getNumEdges();

    SpatialGraphSelection graph_selection(nvertices, nedges);

    // Show all vertices by default if no selection mask is given
    // or the size does not match the graph.
    if(nvertices != static_cast<int>(selection.size()))
    {
        graph_selection.selectAllVertices();
        graph_selection.selectAllEdges();
        graph_selection.selectAllPoints();
    }
    // Apply the selection.
    else
    {
        graph_selection.deselectAllVertices();
        graph_selection.deselectAllEdges();
        graph_selection.deselectAllPoints();
        
        // Select all vertices.
        for(int ivertex = 0; ivertex < nvertices; ++ivertex)
        {
            if(selection[ivertex])
            {
                graph_selection.selectVertex(ivertex);
            }
        }

        // Select all edges connected to vertices in the selection.
        for(int iedge = 0; iedge < nedges; ++iedge)
        {
            const int ivertex_source = input->getEdgeSource(iedge);
            if(ivertex_source >= nvertices || !selection[ivertex_source])
            {
                continue;
            }

            const int ivertex_target = input->getEdgeTarget(iedge);
            if(ivertex_target >= nvertices || !selection[ivertex_target])
            {
                continue;
            }

            graph_selection.selectEdge(iedge);
        }
    }

    // Filter the graph.
    McHandle<HxSpatialGraph> temporaryGraph = input->getSubgraph(graph_selection);
    result->copyFrom(temporaryGraph);
}


void filterEdges(
    HxSpatialGraph* result, 
    HxSpatialGraph* input, 
    const std::vector<bool>& selection
) {
    const int nvertices = input->getNumVertices();
    const int nedges = input->getNumEdges();

    SpatialGraphSelection graph_selection(nvertices, nedges);

    // Select all edges if the selection mask is empty. This is a convention also
    // used in Coda. *No selection* means all items are visibile/unmuted.
    if(nedges != static_cast<int>(selection.size()))
    {
        graph_selection.selectAllVertices();
        graph_selection.selectAllEdges();
        graph_selection.selectAllPoints();
    }
    // Apply the selection.
    else
    {
        graph_selection.deselectAllVertices();
        graph_selection.deselectAllEdges();
        graph_selection.deselectAllPoints();

        for(int iedge = 0; iedge < nedges; ++iedge)
        {
            if(!selection[iedge])
            {
                continue;
            }

            graph_selection.selectEdge(iedge);
            
            const int ivertex_source = input->getEdgeSource(iedge);
            if(ivertex_source < nvertices)
            {
                graph_selection.selectVertex(ivertex_source);
            }

            const int ivertex_target = input->getEdgeTarget(iedge);
            if(ivertex_target < nvertices)
            {
                graph_selection.selectVertex(ivertex_target);
            }
        }
    }
    
    // Filter the graph.
    McHandle<HxSpatialGraph> temporaryGraph = input->getSubgraph(graph_selection);
    result->copyFrom(temporaryGraph);
}


void filter(
    HxUniformLabelField3* result, 
    HxUniformLabelField3* input, 
    const std::vector<bool>& selection
)
{    
    // Configure the result field to match the input field.
    const auto dims = input->lattice().getDims();
    result->lattice().setPrimType(input->primType());
    result->lattice().resize(dims);
    result->lattice().setBoundingBox(input->getBoundingBox());

    // Apply no filter if the selection mask is empty.
    if(selection.empty())
    {
        result->copyData(*input);
    }
    // Filter based on the input field.
    else
    {
        const int nrows = static_cast<int>(selection.size());
        const float bg_value = 0.0f;
        
        for(int iz = 0; iz < dims.nz; ++iz)
        {
            for(int iy = 0; iy < dims.ny; ++iy)
            {
                for(int ix = 0; ix < dims.nx; ++ix)
                {
                    const float value = input->evalReg(ix, iy, iz);
                    float result_value = bg_value;
                    const int label = static_cast<int>(value);

                    // Note the offset: The first foreground label has the value 1
                    // while the first row has index 0.
                    if(0 < label && label <= nrows && selection[label - 1])
                    {
                        result_value = value;
                    }
                    
                    result->lattice().set(ix, iy, iz, &result_value);
                }
            }
        }
    }

    result->touchMinMax();
}


/**
 * Internal method used to obtain a colormap from an HxColormapPort.
 */
static McHandle<HxColormap> colormapFromPort(HxPortColormap& port)
{
    if(port.getColormap())
    {
        return port.getColormap();
    }
    return port.getDefaultColormap();
}


/**
 * Internal helper method used by
 * 
 *      colormapVertices() 
 *      and colormapEdges().
 */
static McHandle<HxColormap> colormap(
    HxConnection& connection, 
    bool vertexData
) {
    if(HxColormap* colormap = hxconnection_cast<HxColormap>(connection))
    {
        return colormap;
    }
    else if(HxScalarField3* field = hxconnection_cast<HxScalarField3>(connection))
    {
        return colormapFromPort(field->portSharedColormap);
    }
    else if(HxVolumeRender2* renderer = hxconnection_cast<HxVolumeRender2>(connection))
    {
        return colormapFromPort(renderer->portColormap);
    }
    else if(HxLineRaycast* lineraycast = hxconnection_cast<HxLineRaycast>(connection))
    {
        if(vertexData)
        {
            return colormapFromPort(lineraycast->mPortNodeColorMap);
        }
        else
        {
            return colormapFromPort(lineraycast->mPortLineColorMap);
        }
    }
    return nullptr;
}


McHandle<HxColormap> colormapVertices(HxConnection& connection)
{
    return colormap(connection, true);
}


McHandle<HxColormap> colormapEdges(HxConnection& connection)
{
    return colormap(connection, false);
}


/**
 * Helper function to convert a float RGBA color into a hex-coded
 * string representation.
 */
static std::string rgbaToHex(const float rgba[4])
{
    std::ostringstream ss;
    ss 
        << "#"
        << std::hex << std::max(0, std::min(255, static_cast<int>(255.0f*rgba[0])))
        << std::hex << std::max(0, std::min(255, static_cast<int>(255.0f*rgba[1])))
        << std::hex << std::max(0, std::min(255, static_cast<int>(255.0f*rgba[2])))
        << std::hex << std::max(0, std::min(255, static_cast<int>(255.0f*rgba[3])));
    return ss.str();
}


/**
 * Extract a color from a hex coded RGBA value.
 */
static bool rgbaFromHex(float rgba[4], std::string hex)
{
    rgba[0] = 1.0f;
    rgba[1] = 1.0f;
    rgba[2] = 1.0f;
    rgba[3] = 1.0f;

    if(hex.size() < 1 || hex[0] != '#')
    {
        return false;
    }

    // red
    if(hex.size() >= 3)
    {
        std::string svalue = hex.substr(1, 2);
        std::stringstream ss(svalue);

        int ivalue = 0;
        ss >> std::hex >> ivalue;

        rgba[0] = static_cast<float>(ivalue)/255.0f;
    }

    // green
    if(hex.size() >= 5)
    {
        std::string svalue = hex.substr(3, 2);
        std::stringstream ss(svalue);

        int ivalue = 0;
        ss >> std::hex >> ivalue;

        rgba[1] = static_cast<float>(ivalue)/255.0f;
    }

    // blue 
    if(hex.size() >= 7)
    {
        std::string svalue = hex.substr(5, 2);
        std::stringstream ss(svalue);

        int ivalue = 0;
        ss >> std::hex >> ivalue;

        rgba[2] = static_cast<float>(ivalue)/255.0f;
    }

    // alpha
    if(hex.size() >= 9)
    {
        std::string svalue = hex.substr(7, 2);
        std::stringstream ss(svalue);

        int ivalue = 0;
        ss >> std::hex >> ivalue;

        rgba[3] = static_cast<float>(ivalue)/255.0f;
    }
    return true;
}


void colormapToSpreadSheet(
    HxSpreadSheet* spreadsheet, 
    HxColormap* colormap
) {
    // Prepare the spreadsheet.
    spreadsheet->clear();

    spreadsheet->addColumn("value", HxSpreadSheet::Column::FLOAT);
    spreadsheet->addColumn("rgba", HxSpreadSheet::Column::STRING);

    const int icol_value = spreadsheet->findColumn("value", HxSpreadSheet::Column::FLOAT);
    const int icol_rgba = spreadsheet->findColumn("rgba", HxSpreadSheet::Column::STRING);
    
    HxSpreadSheet::Column* col_value = spreadsheet->column(icol_value);
    HxSpreadSheet::Column* col_rgba = spreadsheet->column(icol_rgba);

    // Convert exactly if it's a 256 colormap.
    //
    // This prevents smaller colormaps from being sampled at the wrong
    // intervals. It's also better for discrete (label) colormaps.
    //
    HxColormap256* colormap256 = dynamic_cast<HxColormap256*>(colormap);
    if(colormap256)
    {
        const int ncolors = colormap256->getSize();
        spreadsheet->setNumRows(ncolors);

        for(int icolor = 0; icolor < ncolors; ++icolor)
        {
            float rgba[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            colormap256->getRGBA(icolor, rgba);

            std::string hex_rgba = rgbaToHex(rgba);

            col_value->setValue(icolor, static_cast<float>(icolor));
            col_rgba->setValue(icolor, hex_rgba.c_str());
        }
    }
    // In this case, we can only assume at continuous colormap
    // with sampling between 0 and 1. So just sample at 256 values
    // for now.
    //
    else
    {
        const int nsamples = 256;
        const float xmin = colormap->minCoord();
        const float xmax = colormap->maxCoord();
        const float xptp = xmax - xmin;
        const float xstep = xptp/static_cast<float>(nsamples - 1);

        spreadsheet->setNumRows(nsamples);
        
        for(int isample = 0; isample < nsamples; ++isample)
        {
            float x = xmin + xstep*static_cast<float>(isample);

            float rgba[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            colormap->getRGBA(x, rgba);

            std::string hex_rgba = rgbaToHex(rgba);

            col_value->setValue(isample, x);
            col_rgba->setValue(isample, hex_rgba.c_str());
        }
    }
}


bool colormapFromSpreadSheet(
    HxColormap256* colormap,
    HxSpreadSheet* spreadsheet
) {
    // Coda outputs the color for each row in the dataframe.
    // So no additional mapping is actually required.
    
    const int ncolors = spreadsheet->nRows();

    const int icol_rgba = spreadsheet->findColumn("rgba", HxSpreadSheet::Column::STRING);
    if(icol_rgba < 0)
    {
        return false;
    }

    const HxSpreadSheet::Column* col_rgba = spreadsheet->column(icol_rgba);    
    if(col_rgba == nullptr)
    {
        return false;
    }
    
    colormap->resize(ncolors);
    colormap->setInterpolate(false);
    colormap->setOutOfBoundsBehavior(HxColormap256::DEFAULT_CLAMP);

    for(int icolor = 0; icolor < ncolors; ++icolor)
    {            
        std::string hex_color = col_rgba->stringValue(icolor).dataPtr();
        float rgba[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        rgbaFromHex(rgba, hex_color);
        colormap->setRGBA(icolor, rgba);
    }

    colormap->setMinMax(0.0f, static_cast<float>(ncolors) - 1.0f);
    return true;
}


} // namespace coda