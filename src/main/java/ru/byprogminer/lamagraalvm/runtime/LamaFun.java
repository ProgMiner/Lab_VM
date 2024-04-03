package ru.byprogminer.lamagraalvm.runtime;

import com.oracle.truffle.api.CompilerDirectives.TruffleBoundary;
import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.dsl.Fallback;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.frame.MaterializedFrame;
import com.oracle.truffle.api.interop.InteropLibrary;
import com.oracle.truffle.api.interop.TruffleObject;
import com.oracle.truffle.api.library.ExportLibrary;
import com.oracle.truffle.api.library.ExportMessage;
import com.oracle.truffle.api.source.SourceSection;
import com.oracle.truffle.api.utilities.TriState;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.nodes.builtin.StringBuiltin;

@ExportLibrary(InteropLibrary.class)
public record LamaFun(RootCallTarget callTarget, MaterializedFrame parentScope) implements TruffleObject {

    @ExportMessage
    public boolean isExecutable() {
        return true;
    }

    @ExportMessage
    public Object execute(Object... arguments) {
        return callTarget.call(arguments);
    }

    @ExportMessage
    public boolean hasExecutableName() {
        return callTarget.getRootNode().getName() != null;
    }

    @ExportMessage
    public String getExecutableName() {
        return callTarget.getRootNode().getName();
    }

    @ExportMessage
    public boolean hasSourceLocation() {
        return callTarget.getRootNode().getSourceSection() != null;
    }

    @ExportMessage
    public SourceSection getSourceLocation() {
        return callTarget.getRootNode().getSourceSection();
    }

    @ExportMessage
    public boolean hasLanguage() {
        return true;
    }

    @ExportMessage
    public Class<LamaLanguage> getLanguage() {
        return LamaLanguage.class;
    }

    @ExportMessage
    public String toDisplayString(@SuppressWarnings("unused") boolean allowSideEffects) {
        final StringBuilder sb = new StringBuilder();

        StringBuiltin.string(this, sb);
        return sb.toString();
    }

    @ExportMessage
    @TruffleBoundary
    public int identityHashCode() {
        return System.identityHashCode(this);
    }

    @ExportMessage
    static final class IsIdenticalOrUndefined {

        @Specialization
        static TriState isIdentical(LamaFun a, LamaFun b) {
            return TriState.valueOf(a == b);
        }

        @Fallback
        static TriState isIdentical(LamaFun a, Object b) {
            return TriState.UNDEFINED;
        }
    }
}
