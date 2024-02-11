add_executable(reply)

target_sources(reply
  PRIVATE
    reply.cpp
)

set_property(TARGET reply PROPERTY CXX_STANDARD 17)

target_link_libraries(reply
  PRIVATE
    plywoot
)
