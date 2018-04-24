project(FILEINDEX)

link_libraries(libmagic.so)
link_libraries(libhiredis.so)

add_executable(search FileIndexer.cc)
