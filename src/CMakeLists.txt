set (SOURCES
	main.cpp
	SynacorVM.cpp
	VMDebugger.cpp
)

set (HEADERS
	SynacorVM.hpp
	VMDebugger.hpp
)

add_executable(synacorvm ${SOURCES} ${HEADERS})

install(TARGETS synacorvm DESTINATION bin)