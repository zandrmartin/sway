include_directories(
	${PROTOCOLS_INCLUDE_DIRS}
	${CAIRO_INCLUDE_DIRS}
	${PANGO_INCLUDE_DIRS}
	${JSONC_INCLUDE_DIRS}
	${WAYLAND_INCLUDE_DIR}
	${WLC_INCLUDE_DIRS}
	${XKBCOMMON_INCLUDE_DIRS}
)

add_executable(swaygrab
	main.c
	json.c
	interactive.c
)

target_link_libraries(swaygrab
	sway-common
	sway-wayland
	${WLC_LIBRARIES}
	${JSONC_LIBRARIES}
	${WAYLAND_CLIENT_LIBRARIES}
	${WAYLAND_CURSOR_LIBRARIES}
	rt
	m
)

if (WITH_GDK_PIXBUF)
	include_directories(
		${GDK_PIXBUF_INCLUDE_DIRS}
	)
	target_link_libraries(swaygrab
		${GDK_PIXBUF_LIBRARIES}
	)
endif()

install(
	TARGETS swaygrab
	RUNTIME
	DESTINATION bin
	COMPONENT runtime
)

add_manpage(swaygrab 1)
