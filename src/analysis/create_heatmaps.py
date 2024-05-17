from qgis.core import *
from qgis.utils import *

import pandas as pd

vlayer = QgsVectorLayer("/Users/ahmadh/Downloads/WijkBuurtkaart_2018_v3/gemeente_2018_v3.shp", "gemeente_2018_v3", "ogr")
vlayer.isValid()
QgsProject.instance().addMapLayer(vlayer)

# Contains list of municipality names in alphabetical order (column: Regio's
municipalities_info = pd.read_csv("/Users/ahmadh/cbs-covid/src/data/inwoners_gemeente_2018.csv")

freq = 1
experiment = 'experiments_2023-08-22_22-07-08/0e45e32e-4128-11ee-a9c4-a63f12acbeef'
options = ['total_per_municipality', 'total_infected_per_municipality']
query = 1
totalT = 2280

max_value = 50

for t in range(2280):
    if (t % freq) != 0:
        continue
    # Load the data file that we need to link the municipality names to
    data = pd.read_csv(f"/Users/ahmadh/snellius/cbs-covid/build/output/{experiment}/{options[query]}_t{t}.csv")

    column_to_replace = 'x'
    replacement_column = "Regio's"

    data[column_to_replace] = municipalities_info[replacement_column]
    data.to_csv("/tmp/qgis_infected.csv", index=False)

    csv_layer = QgsVectorLayer("file:///tmp/qgis_infected.csv?type=csv&maxFields=20000&detectTypes=yes&geomType=none&subsetIndex=no&watchFile=no", "data", "delimitedtext")
    csv_layer.isValid()
    QgsProject.instance().addMapLayer(csv_layer)

    # Create the join object
    join_info = QgsVectorLayerJoinInfo()

    # Set the join layer
    join_info.setJoinLayer(csv_layer)

    # Set the join field and target field
    join_info.setJoinFieldName("x")
    join_info.setTargetFieldName("GM_NAAM")

    # Add the join to the shapefile layer
    vlayer.addJoin(join_info)

    default_style = QgsStyle().defaultStyle()
    color_ramp = default_style.colorRamp('Reds')

    num_classes = 8
    interval = max_value / num_classes
    graduated_renderer = QgsGraduatedSymbolRenderer()
    graduated_renderer.setClassAttribute("data_y")
    graduated_renderer.setClassificationMethod(QgsClassificationEqualInterval())
    graduated_renderer.updateClasses(vlayer, num_classes)
    graduated_renderer.updateColorRamp(color_ramp)
    lower = 0
    upper = interval
    for r in range(num_classes):
        graduated_renderer.updateRangeLowerValue(r, lower)
        graduated_renderer.updateRangeUpperValue(r, upper)
        lower = upper
        upper += interval
        r += 1
    vlayer.setRenderer(graduated_renderer)
    vlayer.triggerRepaint()

    output_size = QSize(720, 480)
    output_dpi = 48
    output_format = QImage.Format_ARGB32_Premultiplied

    # Create a new map settings object
    map_settings = QgsMapSettings()

    # Add the shapefile layer to the map and set the renderer to a graduated symbol renderer
    map_settings.setLayers([vlayer])
    vlayer.setRenderer(graduated_renderer)

    # Set the extent of the map to the extent of the shapefile layer
    map_settings.setExtent(vlayer.extent())

    # Set the output size and resolution of the map
    map_settings.setOutputSize(output_size)
    map_settings.setOutputDpi(output_dpi)
    map_settings.setBackgroundColor(QColor(0, 0, 0, 0))

    image = QImage(output_size, output_format)
    painter = QPainter()
    painter.begin(image)
    job = QgsMapRendererCustomPainterJob(map_settings, painter)
    job.renderSynchronously()
    painter.end()
    t_formatted = str(t).zfill(4)
    image.save(f"/Users/ahmadh/Downloads/infected_images/{options[query]}_{t_formatted}.png", "PNG")

    QgsProject.instance().removeMapLayer(csv_layer)
