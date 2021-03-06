function(query_qmake var)
  get_target_property(QMAKE_EXECUTABLE Qt5::qmake LOCATION)
  exec_program(${QMAKE_EXECUTABLE} ARGS "-query ${var}" RETURN_VALUE return_code OUTPUT_VARIABLE output )

  if(return_code)
    message(WARNING "failed to query ${var}: ${return_code}")
  else()
    file(TO_CMAKE_PATH "${output}" output)
    set(${var} ${output} PARENT_SCOPE)
  endif()
endfunction()
