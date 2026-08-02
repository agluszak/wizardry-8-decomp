package wiz8.recovery;

/** ABI adjustments performed by one compiler thunk. */
record ThunkInfo(ThunkKind kind, long thisAdjustment, long returnAdjustment) {
	static final ThunkInfo NONE = new ThunkInfo(ThunkKind.NONE, 0, 0);

	enum ThunkKind {
		NONE, IMPORT, THIS_ADJUSTOR, RETURN_ADJUSTOR,
		THIS_AND_RETURN_ADJUSTOR, UNKNOWN
	}
}
