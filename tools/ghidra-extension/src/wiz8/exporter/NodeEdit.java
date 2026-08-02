package wiz8.exporter;

import ghidra.app.decompiler.ClangNode;

/** One renderer edit proposed against a verified markup node. */
sealed interface NodeEdit {
	ClangNode node();
	record Drop(ClangNode node) implements NodeEdit { }
	record Replace(ClangNode node, String text) implements NodeEdit { }
}
