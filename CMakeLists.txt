CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)
CMAKE_POLICY(SET CMP0003 NEW)

PROJECT(picoquic)

# FIND_PACKAGE(PkgConfig REQUIRED)

SET(CMAKE_C_FLAGS "-std=c99 -Wall -O2 -g ${CC_WARNING_FLAGS} ${CMAKE_C_FLAGS}")

INCLUDE_DIRECTORIES(picoquic picoquictest ../picotls/include
    ${PICOTLS_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR})

SET(PICOQUIC_LIBRARY_FILES
    picoquic/fnv1a.c
    picoquic/frames.c
    picoquic/http0dot9.c
    picoquic/intformat.c
    picoquic/logger.c
    picoquic/newreno.c
    picoquic/packet.c
    picoquic/picohash.c
    picoquic/quicctx.c
    picoquic/sacks.c
    picoquic/sender.c
    picoquic/tls_api.c
    picoquic/transport.c
    picoquic/util.c
)

SET(PICOQUIC_TEST_LIBRARY_FILES
    picoquictest/cnx_creation_test.c
    picoquictest/float16test.c
    picoquictest/fnv1atest.c
    picoquictest/hashtest.c
    picoquictest/http0dot9test.c
    picoquictest/intformattest.c
    picoquictest/parseheadertest.c
    picoquictest/pn2pn64test.c
    picoquictest/sacktest.c
    picoquictest/sim_link.c
    picoquictest/stream0_frame_test.c
    picoquictest/tls_api_test.c
    picoquictest/transport_param_test.c
)

FIND_LIBRARY(PTLS_CORE picotls-core PATH ../picotls)
MESSAGE(STATUS "Found picotls-core at : ${PTLS_CORE} " )

FIND_LIBRARY(PTLS_MINICRYPTO picotls-minicrypto PATH ../picotls)
MESSAGE(STATUS "Found picotls-crypto at : ${PTLS_MINICRYPTO} " )

FIND_LIBRARY(PTLS_OPENSSL picotls-openssl PATH ../picotls)
MESSAGE(STATUS "Found picotls-openssl at : ${PTLS_OPENSSL} " )

FIND_PACKAGE(OpenSSL )
MESSAGE("root: ${OPENSSL_ROOT_DIR}")
MESSAGE("OpenSSL_VERSION: ${OPENSSL_VERSION}")
MESSAGE("OpenSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
MESSAGE("OpenSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")

ADD_LIBRARY(picoquic-core
    ${PICOQUIC_LIBRARY_FILES}
)

ADD_EXECUTABLE(picoquicdemo picoquicfirst/picoquicdemo.c
                            picoquicfirst/getopt.c )
TARGET_LINK_LIBRARIES(picoquicdemo picoquic-core
    ${PTLS_CORE}
    ${PTLS_OPENSSL}    
    ${PTLS_MINICRYPTO}
    ${OPENSSL_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

ADD_EXECUTABLE(picoquic_ct picoquic_t/picoquic_t.c
 ${PICOQUIC_TEST_LIBRARY_FILES} )
TARGET_LINK_LIBRARIES(picoquic_ct picoquic-core
    ${PTLS_CORE}
    ${PTLS_OPENSSL}    
    ${PTLS_MINICRYPTO}
    ${PTLS_OPENSSL}
    ${PTLS_CORE}
    ${OPENSSL_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

SET(TEST_EXES picoquic_ct)
