add_executable(r7complexity main.cpp)

if (MSVC)
	set_target_properties(r7complexity PROPERTIES LINK_FLAGS "/STACK:104857600")
endif()

install(TARGETS r7complexity DESTINATION tools)