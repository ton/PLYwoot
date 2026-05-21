add_executable(plywoot-reply)

target_sources(plywoot-reply
  PRIVATE
    reply.cpp
)

set_target_properties(plywoot-reply
  PROPERTIES
    OUTPUT_NAME reply
    CXX_STANDARD 17
)

target_link_libraries(plywoot-reply
  PRIVATE
    plywoot
    plywoot-compiler-flags
)
