set(SB_SRCS sph_shabal.cpp)

add_library(shabal STATIC ${SB_SRCS})
install(TARGETS shabal DESTINATION lib)