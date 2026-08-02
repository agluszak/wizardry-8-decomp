package wiz8.exporter;

/** Reviewed or structurally inferred ownership of one binary emission. */
enum Origin {
	FIRST_PARTY, SURRENDER, SGP, MSVC_RUNTIME, PLATFORM,
	ZLIB, JPEG, INFO_ZIP, IMPORT, LIBRARY, UNKNOWN
}
