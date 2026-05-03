add_custom_target(GeneratedFiles ALL)

function(add_generator out script)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${out}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/tools/codegen/${script}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/tools/codegen/${script}
    )
    add_custom_target(gen_${out} DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${out})
    add_dependencies(GeneratedFiles gen_${out})
endfunction()

add_generator(GeneratedErrorMap.h GenerateErrorMap.py)
add_generator(GeneratedITCHMessages_BIST.h GenerateITCHMessages_BIST.py)
add_generator(GeneratedITCHMessages_NASDAQ.h GenerateITCHMessages_NASDAQ.py)
add_generator(GeneratedOUCHMessages_BIST.h GenerateOUCHMessages_BIST.py)
add_generator(GeneratedOUCHMessages_NASDAQ.h GenerateOUCHMessages_NASDAQ.py)
add_generator(GeneratedSymbolTables.h GenerateSymbolTables.py)
