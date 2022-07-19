project(nft)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( nft nft nft.cpp json11.cpp )
target_include_directories( nft PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( nft ${CMAKE_SOURCE_DIR}/../ricardian )