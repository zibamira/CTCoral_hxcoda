#pragma once

// STL
#include <memory>
#include <vector>

// Qt
#include <QDir>
#include <QObject>
#include <QString>
#include <QMap>
#include <QSharedPointer>
#include <QFileSystemWatcher>
#include <QTemporaryDir>
#include <QTimer>

// ZIB
#include <hxcolor/HxColormap.h>
#include <hxcolor/HxColormap256.h>
#include <hxfield/HxUniformLabelField3.h>
#include <hxspreadsheet/internal/HxSpreadSheet.h>
#include <hxspatialgraph/internal/HxSpatialGraph.h>

// Local
#include <hxcoda/internal/CodaProcess.h>


namespace coda
{


/**
 * @brief The Coda class
 *
 * This module implements a simple file based interface to the visual analytics
 * tool *Coda - The (Codal) Explorer*.
 * 
 * The attached Amira spreadsheets are stored in a shared directory with Coda.convert_analysis
 * When Coda detects a new spreadsheet in this directory, it is automatically 
 * made available in Coda.
 * 
 * Similarly, if Coda updates the current edge or vertex selection spreadsheet,
 * this class will detect the file change and reload it and making the selection
 * available in Amira.
 * 
 * If possible, the files are stored in-memory, e.g. in ``/dev/shm/``.
 */
class Coda : public QObject
{
    Q_OBJECT

public:

    explicit Coda(QObject* parent = nullptr);
    virtual ~Coda();

    bool addVertexData(HxData* data);
    void removeVertexData(HxData* data);
    void writeVertexData(HxData* data);

    bool addEdgeData(HxData* data);
    void removeEdgeData(HxData* data);
    void writeEdgeData(HxData* data);
     
    QString vertexSelectionPath();
    void readVertexSelection();
    const std::vector<bool>& vertexSelection() const;

    QString edgeSelectionPath();
    void readEdgeSelection();
    const std::vector<bool>& edgeSelection() const;

    QString vertexColormapPath();
    bool readVertexColormap();
    bool writeVertexColormap(HxConnection& connection);

    QString edgeColormapPath();
    bool readEdgeColormap();
    bool writeEdgeColormap(HxConnection& connection);

    CodaProcess* process();
    QString dataDirectory();

protected:

    void updateSelectionWatch();

    void loadCodaSelection(
        std::vector<bool>& selection, 
        McHandle<HxSpreadSheet>& spreadsheet,
        const QString path
    );

protected slots:

    void on_watcher_fileChanged(const QString& path);
    void on_watcher_directoryChanged(const QString& path);

protected:

    void rescheduleReadVertexSelection();
    void rescheduleReadEdgeSelection();
    void rescheduleReadVertexColormap();
    void rescheduleReadEdgeColormap();

signals:

    void edgeSelectionChanged();
    void vertexSelectionChanged();

private:

    /// The path to the shared directory with Coda. All files
    /// are placed inside this directory.
    // QTemporaryDir m_data_directory;
    QTemporaryDir m_data_directory;

    /// Manage a dedicated Coda process for this Amira instance.
    CodaProcess* m_process;

    /// The filesystem watcher used to watch changes to the edge
    /// and vertex selections.
    QFileSystemWatcher* m_watcher;

    /// Maps a data object in Amira to the filepath where 
    /// the edge attributes are stored.
    QMap<HxData*, QString> m_edge_data_to_path;

    /// Maps a data object in Amira to the filepath where 
    /// the edge attributes are stored.
    QMap<HxData*, QString> m_vertex_data_to_path;

    /// Maps a path to the associated Amira data object.
    QMap<QString, McHandle<HxData>> m_path_to_data;

    /// The current vertex selection in Coda.
    std::vector<bool> m_coda_vertex_selection;
    McHandle<HxSpreadSheet> m_coda_vertex_selection_csv;
    QTimer* m_coda_vertex_selection_timer;

    /// The current edge selection in Coda.
    std::vector<bool> m_coda_edge_selection;
    McHandle<HxSpreadSheet> m_coda_edge_selection_csv;
    QTimer* m_coda_edge_selection_timer;

    /// The current vertex colormap used in Coda.
    McHandle<HxColormap256> m_coda_vertex_colormap;
    QTimer* m_coda_vertex_colormap_timer;

    /// The current edge colormap used in Coda.
    McHandle<HxColormap256> m_coda_edge_colormap;
    QTimer* m_coda_edge_colormap_timer;
};


/**f
 * Returns a pointer to the Coda *instance* associated with 
 * the current Amira project.
 */
std::shared_ptr<Coda> theCoda();


/**
 * Select all rows given in the selection mask.
 */
void select(
    HxSpreadSheet* input, 
    const std::vector<bool>& selection
);


/**
 * Select all vertices and adjacent edges in the spatialgraph
 * given the vertex selection mask *selection*.
 * 
 * This is the partial subgraph induced by a vertex set.
 */
void filterVertices(
    HxSpatialGraph* result,
    HxSpatialGraph* input, 
    const std::vector<bool>& selection
);


/**
 * Select all edges and adjacent vertices in the spatialgraph
 * given the edge selection mask *selection*.
 * 
 * This is the partial subgraph induced by the edge set.
 */
void filterEdges(
    HxSpatialGraph* result,
    HxSpatialGraph* input, 
    const std::vector<bool>& selection
);


/**
 * Filter a regular field given a selection mask.
 */
void filter(
    HxUniformLabelField3* result, 
    HxUniformLabelField3* input, 
    const std::vector<bool>& selection
);


/**
 * Returns the colormap attached to the vertex data of the
 * HxConnection object.
 * 
 * See also: colormapEdges().
 */
McHandle<HxColormap> colormapVertices(HxConnection& connection);


/**
 * Returns the colormap attached to the edge data of the
 * HxConnection object.
 * 
 * See also: colormapVertices().
 */
McHandle<HxColormap> colormapEdges(HxConnection& connection);


/**
 * Samples the colormap into a spreadsheet which can be easily read
 * by Coda.
 */
void colormapToSpreadSheet(
    HxSpreadSheet* spreadsheet, 
    HxColormap* colormap
);


/**
 * Reads a colormap from a spreadsheet. 
 */
bool colormapFromSpreadSheet(
    HxColormap256* colormap,
    HxSpreadSheet* spreadsheet
);


} // namespace coda