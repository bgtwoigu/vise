add_library( SearchEngine SearchEngine.cc)
target_link_libraries( SearchEngine
  ViseMessageQueue
  train_descs
  train_assign
  train_hamming
  feat_standard
  hamming_embedder
  build_index
  ${Boost_LIBRARIES} 
  ${ImageMagick_LIBRARIES})

add_library( ViseMessageQueue ViseMessageQueue.cc)
target_link_libraries( ViseMessageQueue ${Boost_LIBRARIES} )

add_library( ViseServer ViseServer.cc Resources.cc Connection.cc ViseUtils.cc)
target_link_libraries( ViseServer 
  ViseMessageQueue
  SearchEngine
  clst_centres
  dataset_v2
  feat_standard
  hamming
  hamming_embedder
  mq_filter_outliers
  proto_db
  proto_db_file
  proto_index
  slow_construction
  spatial_api
  spatial_verif_v2
  tfidf_v2
  api_v2
  ${Boost_LIBRARIES} 
  ${ImageMagick_LIBRARIES}
  ${fastann_LIBRARIES})

