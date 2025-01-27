#
# This file controls the installation of the header files.
#

# Header installation
install(
  FILES quaternion.h tools.h BezCoord.h BezCurve.h BezCurvePath.h ReadCurves.h AllocAndRead.h MorphDbg.h mathconst.h MathAlgo.h MathImpl.h geometry.h number_type.h Hex.h HexGrid.h hexyhisto.h CartGrid.h histo.h keys.h Grid.h HdfData.h Process.h RD_Base.h DirichVtx.h DirichDom.h ShapeAnalysis.h NM_Simplex.h Rect.h Anneal.h Config.h vec.h vvec.h mat22.h mat33.h mat44.h colour.h ColourMap.h ColourMap_Lists.h lenthe_colormap.hpp colourmaps_cet.h colourmaps_crameri.h scale.h Random.h rngd.h rng.h rngs.h RecurrentNetworkTools.h RecurrentNetwork.h range.h Winder.h trait_tests.h base64.h unicode.h Mnist.h bootstrap.h rapidxml.hpp rapidxml_iterators.hpp rapidxml_print.hpp rapidxml_utils.hpp flags.h version.h
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/morph
  )
# There are also headers in sub directories
add_subdirectory(nn) # 'nn' for neural network code
add_subdirectory(bn) # 'bn' for boolean net code
add_subdirectory(gl) # GL common and compute code
add_subdirectory(linuxos) # One Linux-specific file
add_subdirectory(healpix) # Public, but slightly modified HEALPix headers

# And the nlohmann/json.hpp header
add_subdirectory(nlohmann)

# And the jcvoronoi code
add_subdirectory(jcvoronoi)

# Graphics headers
install(
  FILES VisualCommon.h Visual.h lodepng.h loadpng.h VisualModel.h VisualDataModel.h VisualTextModel.h TextGeometry.h TextFeatures.h VisualResources.h VisualFace.h CoordArrows.h HexGridVisual.h CartGridVisual.h GridVisual.h GridctVisual.h QuadsVisual.h QuadsMeshVisual.h graphstyles.h DatasetStyle.h GraphVisual.h PointRowsVisual.h PointRowsMeshVisual.h ScatterVisual.h QuiverVisual.h RodVisual.h SphereVisual.h PolygonVisual.h VisualDefaultShaders.h RecurrentNetworkModel.h ColourBarVisual.h CurvyTellyVisual.h HSVWheelVisual.h RhomboVisual.h TriaxesVisual.h TriFrameVisual.h TxtVisual.h VectorVisual.h ConfigVisual.h RectangleVisual.h TriangleVisual.h ConeVisual.h IcosaVisual.h GeodesicVisual.h GratingVisual.h HealpixVisual.h VoronoiVisual.h LengthscaleVisual.h
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/morph
  )
# The Visual-in-a-Qt-Widget code
add_subdirectory(qt)
# WxWidgets code
add_subdirectory(wx)

# Install the EXPORT so that morphologica has its own .cmake file and find_package(morphologica) should work
install(FILES morphologica-config.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/morphologica)
#install(EXPORT morphologica DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/morphologica)
