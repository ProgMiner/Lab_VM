package ru.byprogminer.lamagraalvm.runtime;

import com.oracle.truffle.api.CompilerDirectives.TruffleBoundary;
import com.oracle.truffle.api.dsl.Fallback;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.interop.InteropLibrary;
import com.oracle.truffle.api.interop.InvalidArrayIndexException;
import com.oracle.truffle.api.interop.TruffleObject;
import com.oracle.truffle.api.library.ExportLibrary;
import com.oracle.truffle.api.library.ExportMessage;
import com.oracle.truffle.api.utilities.TriState;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.nodes.builtin.StringBuiltin;

@ExportLibrary(InteropLibrary.class)
public record LamaSexp(String tag, Object[] values) implements TruffleObject {

    @ExportMessage
    public boolean hasArrayElements() {
        return true;
    }

    @ExportMessage
    public long getArraySize() {
        return values.length;
    }

    @ExportMessage
    public boolean isArrayElementReadable(long index) {
        return 0 <= index && index < values.length;
    }

    @ExportMessage
    public boolean isArrayElementModifiable(long index) {
        return isArrayElementReadable(index);
    }

    @ExportMessage
    public boolean isArrayElementInsertable(@SuppressWarnings("unused") long index) {
        return false;
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
    static final class ReadArrayElement {

        @Specialization(rewriteOn = {IndexOutOfBoundsException.class, ArithmeticException.class})
        static Object readArrayElementInBounds(LamaSexp value, long index) {
            return value.values[Math.toIntExact(index)];
        }

        @Specialization(replaces = "readArrayElementInBounds")
        static Object readArrayElement(
                @SuppressWarnings("unused") LamaSexp value,
                long index
        ) throws InvalidArrayIndexException {
            throw InvalidArrayIndexException.create(index);
        }
    }

    @ExportMessage
    static final class WriteArrayElement {

        @Specialization(rewriteOn = {IndexOutOfBoundsException.class, ArithmeticException.class})
        static void writeArrayElementInBounds(LamaSexp values, long index, Object value) {
            values.values[Math.toIntExact(index)] = value;
        }

        @Specialization(replaces = "writeArrayElementInBounds")
        static void writeArrayElement(
                @SuppressWarnings("unused") LamaSexp values,
                long index,
                @SuppressWarnings("unused") Object value
        ) throws InvalidArrayIndexException {
            throw InvalidArrayIndexException.create(index);
        }
    }

    @ExportMessage
    static final class IsIdenticalOrUndefined {

        @Specialization
        static TriState isIdentical(LamaSexp a, LamaSexp b) {
            return TriState.valueOf(a == b);
        }

        @Fallback
        static TriState isIdentical(LamaSexp a, Object b) {
            return TriState.UNDEFINED;
        }
    }
}
