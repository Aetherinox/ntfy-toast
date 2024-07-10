add_library(NtfyToastActions INTERFACE)
target_include_directories(NtfyToastActions INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/ntfytoast>
)
add_library(NtfyToast::NtfyToastActions ALIAS NtfyToastActions)

configure_file(config.h.in config.h @ONLY)
add_library(libntfytoast STATIC ntfytoasts.cpp toasteventhandler.cpp linkhelper.cpp utils.cpp)
target_link_libraries(libntfytoast PUBLIC runtimeobject shlwapi NtfyToast::NtfyToastActions)
target_compile_definitions(libntfytoast PRIVATE UNICODE _UNICODE __WRL_CLASSIC_COM_STRICT__ WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_definitions(libntfytoast PUBLIC __WRL_CLASSIC_COM_STRICT__)
target_include_directories(libntfytoast PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties(libntfytoast PROPERTIES EXPORT_NAME LibNtfyToast)
add_library(NtfyToast::LibNtfyToast ALIAS libntfytoast)
generate_export_header(libntfytoast)

create_icon_rc(${PROJECT_SOURCE_DIR}/data/ntfytoast.ico TOAST_ICON)
add_executable(ntfytoast WIN32 main.cpp ${TOAST_ICON})
target_link_libraries(ntfytoast PRIVATE NtfyToast::LibNtfyToast ntfyretoastsources)
target_compile_definitions(ntfytoast PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
add_executable(NtfyToast::NtfyToast ALIAS ntfytoast)

install(TARGETS ntfytoast NtfyToastActions EXPORT LibNtfyToastConfig RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES ntfytoastactions.h ${CMAKE_CURRENT_BINARY_DIR}/config.h DESTINATION include/ntfytoast)
install(EXPORT LibNtfyToastConfig DESTINATION lib/cmake/libntfytoast NAMESPACE NtfyToast::)
