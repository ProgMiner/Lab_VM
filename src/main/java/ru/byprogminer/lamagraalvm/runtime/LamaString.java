package ru.byprogminer.lamagraalvm.runtime;

import com.oracle.truffle.api.CompilerDirectives.TruffleBoundary;
import com.oracle.truffle.api.dsl.Fallback;
import com.oracle.truffle.api.dsl.Specialization;
import com.oracle.truffle.api.interop.InteropLibrary;
import com.oracle.truffle.api.interop.InvalidArrayIndexException;
import com.oracle.truffle.api.interop.TruffleObject;
import com.oracle.truffle.api.interop.UnsupportedTypeException;
import com.oracle.truffle.api.library.ExportLibrary;
import com.oracle.truffle.api.library.ExportMessage;
import com.oracle.truffle.api.strings.TruffleString;
import com.oracle.truffle.api.utilities.TriState;
import ru.byprogminer.lamagraalvm.LamaLanguage;
import ru.byprogminer.lamagraalvm.nodes.builtin.StringBuiltin;

import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;

@ExportLibrary(InteropLibrary.class)
@SuppressWarnings("MethodDoesntCallSuperMethod")
public record LamaString(byte[] value) implements TruffleObject, Cloneable {

    public static final Charset CHARSET = StandardCharsets.UTF_8;

    @Override
    public Object clone() {
        return new LamaString(value.clone());
    }

    @Override
    public String toString() {
        return new String(value, CHARSET);
    }

    public static LamaString ofString(String str) {
        return new LamaString(str.getBytes(CHARSET));
    }

    @ExportMessage
    public boolean isString() {
        return true;
    }

    @ExportMessage
    public String asString() {
        return toString();
    }

    @ExportMessage
    public TruffleString asTruffleString() {
        return TruffleString.fromByteArrayUncached(value, TruffleString.Encoding.UTF_8);
    }

    @ExportMessage
    public boolean hasArrayElements() {
        return true;
    }

    @ExportMessage
    public long getArraySize() {
        return value.length;
    }

    @ExportMessage
    public boolean isArrayElementReadable(long index) {
        return 0 <= index && index < value.length;
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
        static byte readArrayElementInBounds(LamaString value, long index) {
            return value.value[Math.toIntExact(index)];
        }

        @Specialization(replaces = "readArrayElementInBounds")
        static Object readArrayElement(
                LamaString value,
                long index
        ) throws InvalidArrayIndexException {
            throw InvalidArrayIndexException.create(index);
        }
    }

    @ExportMessage
    static final class WriteArrayElement {

        @Specialization(rewriteOn = {IndexOutOfBoundsException.class, ArithmeticException.class})
        static void writeArrayElementInBounds(LamaString values, long index, byte value) {
            values.value[Math.toIntExact(index)] = value;
        }

        @Specialization(replaces = "writeArrayElementInBounds")
        static void writeArrayElement(
                @SuppressWarnings("unused") LamaString values,
                long index,
                @SuppressWarnings("unused") byte value
        ) throws InvalidArrayIndexException {
            throw InvalidArrayIndexException.create(index);
        }

        @Fallback
        static void writeArrayElement(
                @SuppressWarnings("unused") LamaString values,
                long index,
                @SuppressWarnings("unused") Object value
        ) throws UnsupportedTypeException {
            throw UnsupportedTypeException.create(new Object[]{ value });
        }
    }

    @ExportMessage
    static final class IsIdenticalOrUndefined {

        @Specialization
        static TriState isIdentical(LamaString a, LamaString b) {
            return TriState.valueOf(a == b);
        }

        @Fallback
        static TriState isIdentical(LamaString a, Object b) {
            return TriState.UNDEFINED;
        }
    }
}
